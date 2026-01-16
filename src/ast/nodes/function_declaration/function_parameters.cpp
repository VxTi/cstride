#include "ast/nodes/function_signature.h"

using namespace stride::ast;


void AstFunctionParameterNode::try_parse_subsequent_parameters(
    const Scope& scope,
    TokenSet& tokens,
    std::vector<std::unique_ptr<AstFunctionParameterNode>>& parameters
)
{
    while (tokens.peak_next_eq(TokenType::COMMA))
    {
        tokens.next();
        const auto next = tokens.peak_next();

        auto param = try_parse(scope, tokens);

        if (std::ranges::find_if(
            parameters, [&](const std::unique_ptr<AstFunctionParameterNode>& p)
            {
                return p->name == param->name;
            }) != parameters.end())
        {
            tokens.except(
                next,
                stride::ErrorType::SEMANTIC_ERROR,
                std::format(
                    "Duplicate parameter name \"{}\" in function definition",
                    param->name.value)
            );
        }

        parameters.push_back(std::move(param));
    }
}

std::string AstFunctionParameterNode::to_string()
{
    return name.value;
}

std::unique_ptr<AstFunctionParameterNode> AstFunctionParameterNode::try_parse(
    [[maybe_unused]] const Scope& scope,
    TokenSet& tokens
)
{
    const auto param_name = Symbol(tokens.expect(TokenType::IDENTIFIER).lexeme);

    tokens.expect(TokenType::COLON);

    std::unique_ptr<types::AstType> type_ptr = types::try_parse_type(tokens);

    return std::move(std::make_unique<AstFunctionParameterNode>(param_name, std::move(type_ptr)));
}
