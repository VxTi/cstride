#include "ast/nodes/blocks.h"
#include "ast/nodes/expression.h"
#include "ast/tokens/token_set.h"

using namespace stride::ast;


bool stride::ast::is_array_initializer(const TokenSet& set)
{
    return set.peek_next_eq(TokenType::LSQUARE_BRACKET);
}

std::unique_ptr<AstArray> stride::ast::parse_array_initializer(
    const std::shared_ptr<ParsingContext>& context,
    TokenSet& set
)
{
    const auto reference_token = set.peek_next();
    const auto expression_block = collect_block_variant(
        set,
        TokenType::LSQUARE_BRACKET,
        TokenType::RSQUARE_BRACKET
    );

    ExpressionList elements;

    if (expression_block.has_value())
    {
        auto subset = expression_block.value();

        /// Here we'll parse the subset of tokens (the actual array initializer)
        if (auto first_initializer = parse_inline_expression(context, subset))
        {
            elements.push_back(std::move(first_initializer));
        }

        while (subset.has_next())
        {
            subset.expect(TokenType::COMMA,
                          "Expected ',' between array elements");
            elements.push_back(parse_inline_expression(context, subset));
        }
    }

    const auto& last_token_pos = set.peek(-1).get_source_fragment();
    // `]` is already consumed, so we peek back at it

    const auto position = SourceFragment::combine(reference_token.get_source_fragment(), last_token_pos);

    return std::make_unique<AstArray>(
        position,
        context,
        std::move(elements)
    );
}
