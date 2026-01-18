#include "ast/nodes/function_declaration.h"

using namespace stride::ast;


void stride::ast::parse_subsequent_fn_params(
    const Scope& scope,
    TokenSet& tokens,
    std::vector<std::unique_ptr<AstFunctionParameter>>& parameters
)
{
    while (tokens.peak_next_eq(TokenType::COMMA))
    {
        tokens.next();
        const auto next = tokens.peak_next();

        auto param = parse_first_fn_param(scope, tokens);

        if (std::ranges::find_if(
            parameters, [&](const std::unique_ptr<AstFunctionParameter>& p)
            {
                return p->name == param->name;
            }) != parameters.end())
        {
            tokens.throw_error(
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

std::string AstFunctionParameter::to_string()
{
    return std::format("{}()", name.to_string(), this->type->to_string());
}

std::unique_ptr<AstFunctionParameter> stride::ast::parse_first_fn_param(
    [[maybe_unused]] const Scope& scope,
    TokenSet& tokens
)
{
    const auto param_name = Symbol(tokens.expect(TokenType::IDENTIFIER).lexeme);

    tokens.expect(TokenType::COLON);

    std::unique_ptr<types::AstType> type_ptr = types::parse_primitive_type(tokens);

    return std::move(std::make_unique<AstFunctionParameter>(param_name, std::move(type_ptr)));
}