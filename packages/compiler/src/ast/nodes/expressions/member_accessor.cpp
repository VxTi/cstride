#include "errors.h"
#include "formatting.h"
#include "ast/casting.h"
#include "ast/parsing_context.h"
#include "ast/nodes/enumerables.h"
#include "ast/nodes/expression.h"
#include "ast/tokens/token_set.h"

#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Module.h>

using namespace stride::ast;

/// This parses both function call chaining, and struct member access
/// e.g.,
/// foo().bar.baz()
/// or
/// struct_var.member.member2 ...
std::unique_ptr<IAstExpression> stride::ast::parse_chained_member_access(
    const std::shared_ptr<ParsingContext>& context,
    TokenSet& set,
    const std::unique_ptr<IAstExpression>& lhs
)
{
    std::vector<std::unique_ptr<AstIdentifier>> chained_accessors = {};

    // Initial accessors
    const auto reference_token = set.peek_next();

    while (set.peek_next_eq(TokenType::DOT))
    {
        set.expect(TokenType::DOT, "Expected '.' after identifier in member access");

        const auto accessor_iden_tok = set.expect(TokenType::IDENTIFIER,
                                                  "Expected identifier after '.' in member access");

        auto symbol = Symbol(accessor_iden_tok.get_source_fragment(),
                             accessor_iden_tok.get_lexeme());

        chained_accessors.push_back(std::make_unique<AstIdentifier>(context, symbol));
    }

    const auto lhs_source_pos = lhs->get_source_fragment();

    // TODO: Allow function calls to be the last element as well.
    auto lhs_identifier = cast_expr<AstIdentifier*>(lhs.get());
    if (!lhs_identifier)
    {
        throw parsing_error(
            ErrorType::TYPE_ERROR,
            "Member access base must be an identifier",
            lhs_source_pos);
    }

    const auto last_source_pos = chained_accessors.back().get()->get_source_fragment();
    const auto source_pos = SourceFragment::concat(lhs_source_pos, last_source_pos);

    return std::make_unique<AstMemberAccessor>(
        source_pos,
        context,
        lhs_identifier->clone_as<AstIdentifier>(),
        std::move(chained_accessors)
    );
}

std::vector<AstIdentifier*> AstMemberAccessor::get_members() const
{
    // We don't wanna transfer ownership to anyone else...
    std::vector<AstIdentifier*> result;
    result.reserve(this->_members.size());
    std::ranges::transform(
        this->_members,
        std::back_inserter(result),
        [](const std::unique_ptr<AstIdentifier>& member)
        {
            return member.get();
        });
    return result;
}

bool stride::ast::is_member_accessor(IAstExpression* lhs, const TokenSet& set)
{
    // We assume the expression and subsequent tokens are "member accessors"
    // if the LHS is an identifier, and it's followed by `.<identifier>`
    // E.g., `struct_var.member`
    if (cast_expr<AstIdentifier*>(lhs))
    {
        return set.peek_eq(TokenType::DOT, 0)
            && set.peek_eq(TokenType::IDENTIFIER, 1);
    }
    return false;
}

llvm::Value* AstMemberAccessor::codegen_global_member_accessor(
    llvm::Module* module,
    llvm::IRBuilderBase* builder
) const
{
    llvm::Value* base_val = this->_base->codegen(module, builder);
    IAstType* cloned_base_type = this->_base->get_type();
    std::string base_type_name = cloned_base_type->get_type_name();

    // We look for a GlobalVariable with an initializer
    auto* global_var = llvm::dyn_cast_or_null<llvm::GlobalVariable>(base_val);
    if (!global_var || !global_var->hasInitializer())
    {
        // If the base isn't a global with an initializer, we can't fold it.
        return nullptr;
    }

    llvm::Constant* current_const = global_var->getInitializer();
    std::string current_struct_name = base_type_name;

    for (const auto& accessor : this->_members)
    {
        auto struct_def_opt = this->get_context()->get_struct_type(current_struct_name);
        if (!struct_def_opt.has_value())
            return nullptr;

        const auto struct_def = struct_def_opt.value();

        const auto member_index = struct_def->get_member_field_index(accessor->get_name());
        if (!member_index.has_value())
        {
            return nullptr;
        }

        // Extract the constant field value
        current_const = current_const->getAggregateElement(member_index.value());
        if (!current_const)
        {
            // Index out of bounds or invalid aggregate
            throw parsing_error(
                ErrorType::COMPILATION_ERROR,
                std::format("Invalid member access on constant '{}'", base_type_name),
                this->get_source_fragment()
            );
        }

        auto member_field_type = struct_def->get_member_field_type(accessor->get_name());
        if (!member_field_type.has_value())
        {
            return nullptr;
        }

        cloned_base_type = member_field_type.value();
        current_struct_name = cloned_base_type->get_type_name();
    }

    return current_const;
}

llvm::Value* AstMemberAccessor::codegen(
    llvm::Module* module,
    llvm::IRBuilderBase* builder
)
{
    // Global struct definitions have no insertion point, so we need to do
    // constant folding here by looking up the global variable and its initializer.
    if (!builder->GetInsertBlock())
    {
        return codegen_global_member_accessor(module, builder);
    }

    // Standard Code Generation (Function context)

    // Will codegen identifier - reference to variable (getptr)
    llvm::Value* current_val = this->_base->codegen(module, builder);
    if (!current_val)
    {
        return nullptr;
    }

    const auto base_struct_type = get_struct_type_from_type(this->_base->get_type());

    // Base must be a struct for member access to be valid.
    // This would be okay if there were on members, however, this should never happen
    if (!base_struct_type.has_value())
    {
        throw parsing_error(
            ErrorType::TYPE_ERROR,
            "Member access base must be a struct",
            this->get_source_fragment()
        );
    }

    IAstType* parent_type = base_struct_type.value();
    std::string parent_struct_internalized_name = base_struct_type.value()->get_internalized_name();
    std::string current_accessor_name = this->_base->get_name();

    // With opaque pointers, we need to know if we are operating on an address (L-value)
    // or a loaded struct value (R-value). Pointers allow GEP, values require ExtractValue.
    const bool is_pointer_ty = current_val->getType()->isPointerTy();

    for (const auto& accessor : this->_members)
    {
        auto parent_struct_type_opt = get_struct_type_from_type(parent_type);

        // In next iteration, it's possible that the previous member produced a non-struct type,
        // hence yielding a nullptr.
        if (!parent_struct_type_opt.has_value())
        {
            throw parsing_error(
                ErrorType::TYPE_ERROR,
                std::format(
                    "Cannot access member '{}' of non-struct type '{}'",
                    accessor->get_name(),
                    current_accessor_name
                ),
                this->get_source_fragment()
            );
        }

        const auto parent_struct_type = parent_struct_type_opt.value();

        parent_struct_internalized_name = parent_struct_type->get_internalized_name();

        const auto member_index = parent_struct_type->get_member_field_index(accessor->get_name());

        // Validate whether the previous struct contains the "member" field
        if (!member_index.has_value())
        {
            throw parsing_error(
                ErrorType::TYPE_ERROR,
                std::format(
                    "Cannot access member '{}' of struct '{}': member does not exist",
                    accessor->get_name(),
                    parent_struct_type->get_type_name()
                ),
                this->get_source_fragment()
            );
        }

        if (is_pointer_ty)
        {
            // Get the LLVM type for the current struct to generate the GEP
            llvm::StructType* struct_llvm_type = llvm::StructType::getTypeByName(
                module->getContext(),
                parent_struct_internalized_name
            );
            if (!struct_llvm_type)
            {
                throw parsing_error(
                    ErrorType::COMPILATION_ERROR,
                    std::format(
                        "Struct '{}' not registered internally",
                        parent_struct_type->get_type_name()),
                    this->get_source_fragment());
            }

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

        auto member_field_type = parent_struct_type->get_member_field_type(accessor->get_name());
        if (!member_field_type.has_value())
        {
            throw parsing_error(
                ErrorType::COMPILATION_ERROR,
                std::format(
                    "Unknown member type '{}' in struct '{}'",
                    accessor->get_name(),
                    parent_struct_type->get_type_name()
                ),
                this->get_source_fragment());
        }

        parent_type = member_field_type.value();
    }

    if (!parent_type)
    {
        throw parsing_error(
            ErrorType::COMPILATION_ERROR,
            std::format(
                "Invalid member access on non-struct type '{}'",
                this->_base->get_type()->get_type_name()
            ),
            this->get_source_fragment());
    }

    // if we were working with pointers, we need to load the final result
    if (is_pointer_ty)
    {
        llvm::Type* final_llvm_type = type_to_llvm_type(parent_type, module);

        return builder->CreateLoad(
            final_llvm_type,
            current_val,
            "val_member_access"
        );
    }

    // If we were working with values (ExtractValue), we already have the result.
    return current_val;
}

std::unique_ptr<IAstNode> AstMemberAccessor::clone()
{
    std::vector<std::unique_ptr<AstIdentifier>> members;
    members.reserve(this->_members.size());

    for (const auto& member : this->_members)
    {
        members.push_back(member->clone_as<AstIdentifier>());
    }

    return std::make_unique<AstMemberAccessor>(
        this->get_source_fragment(),
        this->get_context(),
        this->_base->clone_as<AstIdentifier>(),
        std::move(members)
    );
}

void AstMemberAccessor::validate()
{
    for (const auto& member : this->_members)
    {
        member->validate();
    }
}

std::string AstMemberAccessor::to_string()
{
    std::vector<std::string> member_names;
    member_names.reserve(this->_members.size());

    for (const auto& member : this->_members)
    {
        member_names.push_back(member->get_name());
    }

    return std::format(
        "MemberAccessor(base: {}, member: {})",
        this->get_base()->to_string(),
        join(member_names, ","));
}

std::optional<std::unique_ptr<IAstNode>> AstMemberAccessor::reduce()
{
    return std::nullopt; // Not yet reducible
}

bool AstMemberAccessor::is_reducible()
{
    return false;
}
