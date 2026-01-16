#include "ast/nodes/literals.h"

using namespace stride::ast;

std::optional<std::unique_ptr<AstLiteral>> AstIntegerLiteral::try_parse_optional(const Scope& scope, TokenSet& tokens)
{
    if (tokens.peak_next_eq(TokenType::INTEGER_LITERAL))
    {
        const auto next = tokens.next();
        return std::make_unique<AstIntegerLiteral>(std::stoi(next.lexeme));
    }
    return std::nullopt;
}

std::string AstIntegerLiteral::to_string()
{
    return std::format("IntLiteral({})", value);
}
