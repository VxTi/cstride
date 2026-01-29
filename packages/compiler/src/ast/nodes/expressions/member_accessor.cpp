#include "ast/nodes/expression.h"

using namespace stride::ast;

/// This parses both function call chaining, and struct member access
/// e.g.,
/// foo().bar.baz()
/// or
/// struct_var.member.member2 ...
std::optional<std::unique_ptr<AstExpression>> stride::ast::parse_chained_member_access_optional(
    const std::shared_ptr<SymbolRegistry>& scope,
    TokenSet& set,
    std::unique_ptr<AstExpression> lhs
)
{
    if (set.peak_next_eq(TokenType::DOT))
    {
        set.next();
        const auto accessor_iden_tok = set.expect(
            TokenType::IDENTIFIER,
            "Expected identifier after '.' in member access"
        );

        // const auto next_expr
    }
    return lhs;
}

llvm::Value* AstMemberAccessor::codegen(
    const std::shared_ptr<SymbolRegistry>& scope,
    llvm::Module* module,
    llvm::LLVMContext& context,
    llvm::IRBuilder<>* builder
) {}

std::string AstMemberAccessor::to_string()
{
    return std::format(
        "MemberAccessor(base: {}, member: {})",
        this->get_base_expr()->to_string(),
        this->get_member_expr()->to_string()
    );
}

void AstMemberAccessor::validate()
{
    // Currently, function return types don't exist yet,
    // so we only support struct member access. Validation therefore
    // includes checking whether the main member exists and whether it contains the
    // target field.

    // TODO: Update this once function return types are supported
    const auto base_iden = dynamic_cast<AstIdentifier*>(this->get_base_expr());
    const auto accessor_iden = dynamic_cast<AstIdentifier*>(this->get_member_expr());

    if (!base_iden || !accessor_iden)
    {
        throw parsing_error(
            ErrorType::TYPE_ERROR,
            "Member access base and member must be identifiers",
            *this->get_source(),
            this->get_source_position()
        );
    }

    const auto registered_base = this->get_registry()->get_struct_def(base_iden->get_name());

    if (!registered_base)
    {
        throw parsing_error(
            ErrorType::TYPE_ERROR,
            std::format("Struct '{}' in member accessor does not exist", base_iden->get_name()),
            *this->get_source(),
            this->get_source_position()
        );
    }

    if (!registered_base->has_member(accessor_iden->get_name()))
    {
        throw parsing_error(
            ErrorType::TYPE_ERROR,
            std::format("Undefined member '{}' in struct '{}'", accessor_iden->get_name(), base_iden->get_name()),
            *this->get_source(),
            this->get_source_position()
        );
    }

    // All good!
}

IAstNode* AstMemberAccessor::reduce() {}

bool AstMemberAccessor::is_reducible() {}
