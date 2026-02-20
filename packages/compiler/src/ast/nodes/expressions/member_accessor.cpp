#include <llvm/IR/Module.h>

#include "formatting.h"
#include "ast/nodes/expression.h"

using namespace stride::ast;

bool stride::ast::is_member_accessor(AstExpression* lhs, const TokenSet& set)
{
    // We assume the expression and subsequent tokens are "member accessors"
    // if the LHS is an identifier, and it's followed by `.<identifier>`
    // E.g., `struct_var.member`
    if (dynamic_cast<AstIdentifier*>(lhs))
    {
        return set.peek_eq(TokenType::DOT, 0) && set.peek_eq(TokenType::IDENTIFIER, 1);
    }
    return false;
}

/// This parses both function call chaining, and struct member access
/// e.g.,
/// foo().bar.baz()
/// or
/// struct_var.member.member2 ...
std::unique_ptr<AstExpression> stride::ast::parse_chained_member_access(
    const std::shared_ptr<ParsingContext>& context,
    TokenSet& set,
    const std::unique_ptr<AstExpression>& lhs
)
{
    std::vector<std::unique_ptr<AstIdentifier>> chained_accessors = {};

    // Initial accessors
    const auto reference_token = set.peek_next();

    while (set.peek_next_eq(TokenType::DOT))
    {
        set.expect(TokenType::DOT, "Expected '.' after identifier in member access");

        const auto accessor_iden_tok = set.expect(
            TokenType::IDENTIFIER,
            "Expected identifier after '.' in member access"
        );

        auto symbol = Symbol(accessor_iden_tok.get_source_position(), accessor_iden_tok.get_lexeme());

        chained_accessors.push_back(std::make_unique<AstIdentifier>(context, symbol));
    }

    const auto lhs_source_pos = lhs->get_source_position();

    // TODO: Allow function calls to be the last element as well.
    const auto lhs_identifier = dynamic_cast<AstIdentifier*>(lhs.get());
    if (!lhs_identifier)
    {
        throw parsing_error(
            ErrorType::TYPE_ERROR,
            "Member access base must be an identifier",
            lhs_source_pos
        );
    }

    const auto last_source_pos = chained_accessors.back().get()->get_source_position();

    return std::make_unique<AstMemberAccessor>(
        SourceLocation(
            set.get_source(),
            lhs_source_pos.offset,
            last_source_pos.offset + last_source_pos.length - lhs_source_pos.offset
        ),
        context,
        std::unique_ptr<AstIdentifier>(lhs_identifier),
        std::move(chained_accessors)
    );
}

llvm::Value* AstMemberAccessor::codegen(
    const std::shared_ptr<ParsingContext>& context,
    llvm::Module* module,
    llvm::IRBuilder<>* builder
)
{
    // Special handling for Global context (no active block)
    // We must evaluate to a constant (Constant Folding) as we cannot generate instructions.
    // Note: This assumes the base identifier's codegen handles null blocks gracefully
    // and returns the GlobalVariable* (pointer) rather than loading it.
    if (!builder->GetInsertBlock())
    {
        llvm::Value* base_val = this->get_base()->codegen(context, module, builder);

        // We look for a GlobalVariable with an initializer
        auto* global_var = llvm::dyn_cast_or_null<llvm::GlobalVariable>(base_val);
        if (!global_var || !global_var->hasInitializer())
        {
            // If the base isn't a global with an initializer, we can't fold it.
            return nullptr;
        }

        llvm::Constant* current_const = global_var->getInitializer();
        std::unique_ptr<IAstType> current_ast_type = infer_expression_type(context, this->get_base());
        std::string current_struct_name = current_ast_type->get_internal_name();

        for (const auto& accessor : this->get_members())
        {
            auto struct_def_opt = context->get_struct_def(current_struct_name);
            if (!struct_def_opt.has_value()) return nullptr;

            auto struct_def = struct_def_opt.value();
            while (struct_def->is_reference_struct())
            {
                struct_def_opt = context->get_struct_def(struct_def->get_reference_struct().value().name);
                if (!struct_def_opt.has_value()) return nullptr;
                struct_def = struct_def_opt.value();
            }

            const auto member_index = struct_def->get_struct_field_member_index(accessor->get_name());
            if (!member_index.has_value()) return nullptr;

            // Extract the constant field value
            current_const = current_const->getAggregateElement(member_index.value());
            if (!current_const) return nullptr; // Index out of bounds or invalid aggregate

            auto member_field_type = struct_def->get_struct_member_field_type(accessor->get_name());
            if (!member_field_type.has_value()) return nullptr;

            current_ast_type = member_field_type.value()->clone();
            current_struct_name = current_ast_type->get_internal_name();
        }

        return current_const;
    }

    // Standard Code Generation (Function context)
    llvm::Value* current_val = this->get_base()->codegen(context, module, builder);
    if (!current_val)
    {
        return nullptr;
    }

    std::unique_ptr<IAstType> current_ast_type = infer_expression_type(context, this->get_base());
    std::string current_struct_name = current_ast_type->get_internal_name();

    // With opaque pointers, we need to know if we are operating on an address (L-value)
    // or a loaded struct value (R-value). Pointers allow GEP, values require ExtractValue.
    const bool is_pointer_ty = current_val->getType()->isPointerTy();

    for (const auto& accessor : this->get_members())
    {
        auto struct_def_opt = context->get_struct_def(current_struct_name);
        if (!struct_def_opt.has_value())
        {
            throw parsing_error(
                ErrorType::RUNTIME_ERROR,
                std::format("Unknown struct type '{}' during codegen", current_struct_name),
                this->get_source_position()
            );
        }

        auto struct_def = struct_def_opt.value();
        while (struct_def->is_reference_struct())
        {
            struct_def_opt = context->get_struct_def(struct_def->get_reference_struct().value().name);
            if (!struct_def_opt.has_value())
            {
                throw parsing_error(
                    ErrorType::RUNTIME_ERROR,
                    std::format("Unknown struct type '{}' during codegen",
                                struct_def->get_reference_struct().value().name),
                    this->get_source_position()
                );
            }
            struct_def = struct_def_opt.value();
        }

        const auto member_index = struct_def->get_struct_field_member_index(accessor->get_name());

        if (!member_index.has_value())
        {
            throw parsing_error(
                ErrorType::RUNTIME_ERROR,
                std::format("Unknown member '{}' in struct '{}'", accessor->get_name(), current_struct_name),
                this->get_source_position()
            );
        }

        if (is_pointer_ty)
        {
            // Get the LLVM type for the current struct to generate the GEP
            llvm::StructType* struct_llvm_type = llvm::StructType::getTypeByName(
                module->getContext(), current_struct_name);

            // Create the GEP (GetElementPtr) instruction: &current_ptr->member
            current_val = builder->CreateStructGEP(
                struct_llvm_type,
                current_val,
                member_index.value(),
                "ptr_" + accessor->get_name()
            );
        }
        else
        {
            // We have a direct value, extract the member: current_val.member
            current_val = builder->CreateExtractValue(
                current_val,
                member_index.value(),
                "val_" + accessor->get_name()
            );
        }

        // Update loop state
        auto member_field_type = struct_def->get_struct_member_field_type(accessor->get_name());
        if (!member_field_type.has_value()) return nullptr;

        current_ast_type = member_field_type.value()->clone();
        current_struct_name = current_ast_type->get_internal_name();
    }

    // if we were working with pointers, we need to load the final result
    if (is_pointer_ty)
    {
        llvm::Type* final_llvm_type = internal_type_to_llvm_type(current_ast_type.get(), module);
        return builder->CreateLoad(
            final_llvm_type,
            current_val,
            "val_member_access"
        );
    }

    // If we were working with values (ExtractValue), we already have the result.
    return current_val;
}

std::string AstMemberAccessor::to_string()
{
    std::vector<std::string> member_names;

    for (const auto& member : this->get_members())
    {
        member_names.push_back(member->get_name());
    }

    return std::format(
        "MemberAccessor(base: {}, member: {})",
        this->get_base()->to_string(),
        join(member_names, ",")
    );
}

void AstMemberAccessor::validate()
{
    // Since type inference also does validation, we don't really have to do anything else here
    infer_member_accessor_type(this->get_context(), this);
}

IAstNode* AstMemberAccessor::reduce()
{
    return this; // Not yet reducible
}

bool AstMemberAccessor::is_reducible()
{
    return false;
}
