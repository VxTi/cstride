#include "ast/nodes/expression.h"
#include "ast/nodes/literals.h"
#include "ast/nodes/primitive_type.h"

using namespace stride::ast;

llvm::Value* AstExpression::codegen(llvm::Module* module, llvm::LLVMContext& context)
{
    return nullptr;
}

bool AstExpression::can_parse(const TokenSet& tokens)
{
    const auto type = tokens.peak_next().type;

    if (is_literal(type)) return true;

    switch (type)
    {
    case TokenType::IDENTIFIER: // <something>
    case TokenType::LPAREN: // (
    case TokenType::RPAREN: // )
    case TokenType::BANG: // !
    case TokenType::MINUS: // -
    case TokenType::PLUS: // +
    case TokenType::TILDE: // ~
    case TokenType::CARET: // ^
    case TokenType::LSQUARE_BRACKET: // [
    case TokenType::RSQUARE_BRACKET: // ]
    case TokenType::STAR: // *
    case TokenType::AMPERSAND: // &
    case TokenType::DOUBLE_MINUS: // --
    case TokenType::DOUBLE_PLUS: // ++
    case TokenType::KEYWORD_NIL: // nil
        return true;
    default: break;
    }
    return false;
}

std::string AstExpression::to_string()
{
    std::string children_str;

    for (const auto& child : children)
    {
        children_str += child->to_string() + ", ";
    }

    return std::format("Expression({})", children_str.substr(0, children_str.length() - 2));
}

bool is_variable_declaration(const TokenSet& set)
{
    return (set.peak_next_eq(TokenType::IDENTIFIER) || is_primitive(set.peak_next_type()))
        && set.peak(1).type == TokenType::IDENTIFIER
        && set.peak(2).type == TokenType::EQUALS;
}

std::unique_ptr<AstExpression> AstExpression::try_parse_expression(
    int expression_type_flags,
    const Scope& scope,
    const TokenSet& tokens
)
{
    if ((expression_type_flags & EXPRESSION_VARIABLE_DECLARATION) != 0 &&
        is_variable_declaration(tokens))
    {
        tokens.except("Variable declarations are not allowed in this context");
    }

    return nullptr;
}

std::unique_ptr<AstExpression> AstExpression::try_parse(const Scope& scope, TokenSet& tokens)
{
    try_parse_expression(
        EXPRESSION_VARIABLE_DECLARATION
        | EXPRESSION_INLINE_VARIABLE_DECLARATION,
        scope,
        tokens
    );
    return nullptr;
}
