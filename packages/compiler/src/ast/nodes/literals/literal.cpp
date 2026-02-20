#include "ast/nodes/expression.h"
#include "ast/nodes/literal_values.h"

using namespace stride::ast;

std::optional<std::unique_ptr<AstLiteral>> stride::ast::parse_literal_optional(
    const std::shared_ptr<ParsingContext>& context,
    TokenSet& tokens
)
{
    if (
        auto str = parse_string_literal_optional(context, tokens);
        str.has_value()
    )
    {
        return str;
    }

    if (
        auto int_lit = parse_integer_literal_optional(context, tokens);
        int_lit.has_value()
    )
    {
        return int_lit;
    }

    if (
        auto float_lit = parse_float_literal_optional(context, tokens);
        float_lit.has_value()
    )
    {
        return float_lit;
    }

    if (
        auto char_lit = parse_char_literal_optional(context, tokens);
        char_lit.has_value()
    )
    {
        return char_lit;
    }

    if (
        auto bool_lit = parse_boolean_literal_optional(context, tokens);
        bool_lit.has_value()
    )
    {
        return bool_lit;
    }

    if (tokens.peek_next_eq(TokenType::KEYWORD_NIL))
    {
        const auto reference_token = tokens.next();
        return std::make_unique<AstNilLiteral>(reference_token.get_source_position(), context);
    }

    return std::nullopt;
}

bool stride::ast::is_literal_ast_node(IAstNode* node)
{
    return dynamic_cast<AstLiteral*>(node) != nullptr;
}
