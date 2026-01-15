#include "ast/nodes/literals.h"

using namespace stride::ast;

std::optional<std::unique_ptr<AstNode>> stride::ast::try_parse_literal(const Scope& scope, TokenSet& tokens)
{
    if (auto str = StringLiteral::try_parse(scope, tokens); str.has_value())
    {
        return str;
    }

    if (auto int_lit = IntegerLiteral::try_parse(scope, tokens); int_lit.has_value())
    {
        return int_lit;
    }

    return std::nullopt;
}
