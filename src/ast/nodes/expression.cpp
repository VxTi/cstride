#include "ast/nodes/expression.h"

#include "ast/nodes/literals.h"

using namespace stride::ast;

llvm::Value* AstExpression::codegen()
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

std::unique_ptr<AstExpression> AstExpression::try_parse(Scope, TokenSet& tokens)
{
    const auto type = tokens.peak_next().type;
    tokens.next();

    // tokens.expect(TokenType::SEMICOLON);


    if (is_literal(type))
    {
    }
    return nullptr;
}
