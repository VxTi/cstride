#include "ast/nodes/blocks.h"
#include "ast/nodes/expression.h"

using namespace stride::ast;


bool stride::ast::is_array_initializer(const TokenSet& set)
{
    return set.peek_next_eq(TokenType::LSQUARE_BRACKET);
}

std::unique_ptr<AstArray> stride::ast::parse_array_initializer(
    const std::shared_ptr<SymbolRegistry>& registry,
    TokenSet& set
)
{
    const auto reference_token = set.peek_next();
    const auto expression_block = collect_block_variant(set, TokenType::LSQUARE_BRACKET, TokenType::RSQUARE_BRACKET);

    std::vector<std::unique_ptr<AstExpression>> elements;

    if (expression_block.has_value())
    {
        auto subset = expression_block.value();

        /// Here we'll parse the subset of tokens (the actual array initializer)
        if (auto first_initializer = parse_standalone_expression(registry, subset);
            first_initializer != nullptr)
        {
            elements.push_back(std::move(first_initializer));
        }

        while (subset.has_next())
        {
            subset.expect(TokenType::COMMA, "Expected ',' between array elements");
            elements.push_back(parse_standalone_expression(registry, subset));
        }
    }

    const auto ref_src_pos = reference_token.get_source_position();
    const auto last_token_pos = set.peek(-1).get_source_position(); // `]` is already consumed, so we peek back at it

    return std::make_unique<AstArray>(
        set.get_source(),
        SourcePosition(
            ref_src_pos.offset,
            last_token_pos.offset + last_token_pos.length - ref_src_pos.offset
        ),
        registry,
        std::move(elements)
    );
}
