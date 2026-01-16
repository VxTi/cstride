#include "ast/nodes/literals.h"

using namespace stride::ast;

std::optional<std::unique_ptr<AstLiteral>> AstLiteral::try_parse(const Scope& scope, TokenSet& tokens)
{
    if (auto str = AstStringLiteral::try_parse_optional(scope, tokens); str.has_value())
    {
        return str;
    }

    if (auto int_lit = AstIntegerLiteral::try_parse_optional(scope, tokens); int_lit.has_value())
    {
        return int_lit;
    }

    return std::nullopt;
}