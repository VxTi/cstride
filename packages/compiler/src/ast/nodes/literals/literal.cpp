#include "ast/nodes/expression.h"
#include "ast/nodes/literal_values.h"
#include "ast/tokens/token.h"
#include "ast/tokens/token_set.h"

using namespace stride::ast;

std::optional<std::unique_ptr<AstLiteral>> stride::ast::parse_literal_optional(
    const std::shared_ptr<ParsingContext>& context,
    TokenSet& set)
{
    if (auto str = parse_string_literal_optional(context, set); str.
        has_value())
    {
        return str;
    }

    if (auto int_lit = parse_integer_literal_optional(context, set); int_lit.
        has_value())
    {
        return int_lit;
    }

    if (auto float_lit = parse_float_literal_optional(context, set);
        float_lit.has_value())
    {
        return float_lit;
    }

    if (auto char_lit = parse_char_literal_optional(context, set); char_lit.
        has_value())
    {
        return char_lit;
    }

    if (auto bool_lit = parse_boolean_literal_optional(context, set);
        bool_lit.has_value())
    {
        return bool_lit;
    }

    if (set.peek_next_eq(TokenType::KEYWORD_NIL))
    {
        const auto reference_token = set.next();
        return std::make_unique<AstNilLiteral>(
            reference_token.get_source_fragment(),
            context);
    }

    return std::nullopt;
}

bool stride::ast::is_literal_ast_node(IAstNode* node)
{
    return dynamic_cast<AstLiteral*>(node) != nullptr;
}
