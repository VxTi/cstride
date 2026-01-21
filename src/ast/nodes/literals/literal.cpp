#include "ast/nodes/expression.h"
#include "ast/nodes/literal_values.h"

using namespace stride::ast;

std::optional<std::unique_ptr<AstLiteral>> stride::ast::parse_literal_optional(const Scope& scope, TokenSet& tokens)
{
    if (
        auto str = parse_string_literal_optional(scope, tokens);
        str.has_value()
    )
    {
        return str;
    }

    if (
        auto int_lit = parse_integer_literal_optional(scope, tokens);
        int_lit.has_value()
    )
    {
        return int_lit;
    }

    if (
        auto float_lit = parse_float_literal_optional(scope, tokens);
        float_lit.has_value()
    )
    {
        return float_lit;
    }

    if (
        auto char_lit = parse_char_literal_optional(scope, tokens);
        char_lit.has_value()
    )
    {
        return char_lit;
    }

    if (
        auto bool_lit = parse_boolean_literal_optional(scope, tokens);
        bool_lit.has_value()
    )
    {
        return bool_lit;
    }

    return std::nullopt;
}

bool stride::ast::is_literal_ast_node(IAstNode* node)
{
    return dynamic_cast<AstLiteral*>(node) != nullptr;
}
