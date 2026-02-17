#include "ast/nodes/expression.h"

#include "ast/nodes/blocks.h"
#include "ast/nodes/function_declaration.h"

using namespace stride::ast;

std::unique_ptr<AstFunctionDeclaration> stride::ast::parse_lambda_fn_expression(
    const std::shared_ptr<ParsingContext>& context,
    TokenSet& set
)
{
    const auto reference_token = set.peek_next();
    std::vector<std::unique_ptr<AstFunctionParameter>> parameters = {};

    // Parses expressions like:
    // (<param1>: <type1>, ...): <ret_type> -> {}

    if (auto header_definition = collect_parenthesized_block(set);
        header_definition.has_value())
    {
        parameters.push_back(parse_standalone_fn_param(
            context,
            header_definition.value()
        ));

        parse_subsequent_fn_params(context, set, parameters);
    }

    auto ret_type = parse_type(context, set, "Expected type after anonymous function header definition");
    const auto lambda_arrow = set.expect(TokenType::DASH_RARROW, "Expected '->' after lambda parameters");

    auto lambda_body = parse_block(context, set);

    static int anonymous_lambda_id = 0;

    auto symbol_name = Symbol(
        SourcePosition(
            reference_token.get_source_position().offset,
            lambda_arrow.get_source_position().offset - reference_token.get_source_position().offset
        ),
        "__anonymous_" + std::to_string(anonymous_lambda_id++)
    );

    return std::make_unique<AstFunctionDeclaration>(
        set.get_source(),
        context,
        symbol_name,
        std::move(parameters),
        std::move(lambda_body),
        std::move(ret_type),
        SRFLAG_NONE
    );
}

bool stride::ast::is_lambda_expression(const TokenSet& set)
{
    return set.peek_eq(TokenType::LPAREN, 0)
        && set.peek_eq(TokenType::IDENTIFIER, 1)
        && set.peek_eq(TokenType::COLON, 2);
}
