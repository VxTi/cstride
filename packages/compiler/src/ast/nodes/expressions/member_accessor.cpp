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
        return set.peak_eq(TokenType::DOT, 0) && set.peak_eq(TokenType::IDENTIFIER, 1);
    }
    return false;
}

/// This parses both function call chaining, and struct member access
/// e.g.,
/// foo().bar.baz()
/// or
/// struct_var.member.member2 ...
std::unique_ptr<AstExpression> stride::ast::parse_chained_member_access(
    const std::shared_ptr<SymbolRegistry>& scope,
    TokenSet& set,
    std::unique_ptr<AstExpression> lhs
)
{
    std::vector<std::unique_ptr<AstIdentifier>> chained_accessors = {};

    // Initial accessors
    const auto reference_token = set.peak_next();

    while (set.peak_next_eq(TokenType::DOT))
    {
        set.expect(TokenType::DOT, "Expected '.' after identifier in member access");

        const auto accessor_iden_tok = set.expect(
            TokenType::IDENTIFIER,
            "Expected identifier after '.' in member access"
        );

        chained_accessors.push_back(
            std::make_unique<AstIdentifier>(
                set.get_source(),
                accessor_iden_tok.get_source_position(),
                scope,
                accessor_iden_tok.get_lexeme(),
                accessor_iden_tok.get_lexeme()
            )
        );
    }

    return std::make_unique<AstMemberAccessor>(
        set.get_source(),
        SourcePosition(
            reference_token.get_source_position().offset,
            set.position() - reference_token.get_source_position().offset
        ),
        scope,
        std::unique_ptr<AstIdentifier>(dynamic_cast<AstIdentifier*>(lhs.release())),
        std::move(chained_accessors)
    );
}

llvm::Value* AstMemberAccessor::codegen(
    const std::shared_ptr<SymbolRegistry>& scope,
    llvm::Module* module,
    llvm::LLVMContext& context,
    llvm::IRBuilder<>* builder
)
{
    llvm::Value* current_ptr = this->get_base()->codegen(scope, module, context, builder);
    if (!current_ptr)
    {
        return nullptr;
    }

    // Determine the starting type name (e.g., the type of "struct_var")
    // We use your type inference utilities to resolve the AST type of the base
    std::unique_ptr<IAstInternalFieldType> current_ast_type = infer_expression_type(scope, this->get_base());
    std::string current_struct_name = current_ast_type->get_internal_name();

    for (const auto& accessor : this->get_member())
    {
        const auto struct_def = scope->get_struct_def(current_struct_name);
        if (!struct_def)
        {
            throw parsing_error(
                ErrorType::RUNTIME_ERROR,
                std::format("Unknown struct type '{}' during codegen", current_struct_name),
                *this->get_source(),
                this->get_source_position()
            );
        }

        const auto member_index = struct_def->get_member_index(accessor->get_name());

        if (!member_index.has_value())
        {
            throw parsing_error(
                ErrorType::RUNTIME_ERROR,
                std::format("Unknown member '{}' in struct '{}'", accessor->get_name(), current_struct_name),
                *this->get_source(),
                this->get_source_position()
            );
        }

        // Get the LLVM type for the current struct to generate the GEP
        // We use the internal name which maps to LLVM struct name
        llvm::StructType* struct_llvm_type = llvm::StructType::getTypeByName(context, current_struct_name);

        // Create the GEP (GetElementPtr) instruction
        // This calculates the address of the member: &current_ptr->member
        current_ptr = builder->CreateStructGEP(
            struct_llvm_type,
            current_ptr,
            member_index.value(),
            "ptr_" + accessor->get_name()
        );

        // E. Update type information for the next iteration
        // The current pointer now points to the member's type
        const IAstInternalFieldType* member_field_type = struct_def->get_field(accessor->get_name());

        if (!member_field_type)
        {
            throw parsing_error(
                ErrorType::RUNTIME_ERROR,
                std::format("Unknown member '{}' in struct '{}'", accessor->get_name(), current_struct_name),
                *this->get_source(),
                this->get_source_position()
            );
        }

        current_ast_type = member_field_type->clone();
        current_struct_name = current_ast_type->get_internal_name();
    }

    // 3. Load the final value
    // We must provide the specific LLVM type to Load because opaque pointers might be in use
    llvm::Type* final_llvm_type = internal_type_to_llvm_type(current_ast_type.get(), module, context);

    return builder->CreateLoad(
        final_llvm_type,
        current_ptr,
        "val_member_access"
    );
}

std::string AstMemberAccessor::to_string()
{
    std::vector<std::string> member_names;

    for (const auto& member : this->get_member())
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
    // Currently, function return types don't exist yet,
    // so we only support struct member access. Validation therefore
    // includes checking whether the main member exists and whether it contains the
    // target field.

    // TODO: Update this once function return types are supported
    // Also add support for base returning a struct
    const auto base_iden = dynamic_cast<AstIdentifier*>(this->get_base());
    if (!base_iden)
    {
        throw parsing_error(
            ErrorType::TYPE_ERROR,
            "Member access base must be an identifier",
            *this->get_source(),
            this->get_source_position()
        );
    }

    auto prev_struct_definition = this->get_registry()->get_struct_def(base_iden->get_name());

    if (!prev_struct_definition)
    {
        throw parsing_error(
            ErrorType::TYPE_ERROR,
            std::format("Struct '{}' in member accessor does not exist", base_iden->get_name()),
            *this->get_source(),
            this->get_source_position()
        );
    }

    // We'll iterate over all middle accessors. If there's just a single accessor,
    // it gets skipped here.
    for (int i = 0; i < this->get_member().size() - 1; i++)
    {
        // Member is in between a chain (<...>.<sub1>.<sub2>.<...>),
        const auto sub_iden = dynamic_cast<AstIdentifier*>(this->get_member()[i]);

        if (!sub_iden)
        {
            throw parsing_error(
                ErrorType::TYPE_ERROR,
                "Member access submember must be an identifier",
                *this->get_source(),
                this->get_source_position()
            );
        }

        // The type of the struct member is the name of the referring struct
        const auto sub_iden_field_type = prev_struct_definition->get_field(sub_iden->get_name());
        prev_struct_definition = this->get_registry()->get_struct_def(sub_iden_field_type->get_internal_name());

        // If `prev_struct_definition` is `nullptr` here, we already know it's not a struct type,
        // as it's not registered
        if (!prev_struct_definition)
        {
            throw parsing_error(
                ErrorType::TYPE_ERROR,
                std::format("Struct member '{}' accessor does not exist", base_iden->get_name()),
                *this->get_source(),
                this->get_source_position()
            );
        }
    }

    // The final member can be something other than a struct type, and we don't really care either.
}

IAstNode* AstMemberAccessor::reduce()
{
    return this; // Not yet reducible
}

bool AstMemberAccessor::is_reducible()
{
    return false;
}
