#include "ast/nodes/function_declaration.h"

using namespace stride::ast;


void stride::ast::parse_subsequent_fn_params(
    const Scope& scope,
    TokenSet& tokens,
    std::vector<u_ptr<AstFunctionParameter>>& parameters
)
{
    while (tokens.peak_next_eq(TokenType::COMMA))
    {
        tokens.next();
        const auto next = tokens.peak_next();

        auto param = parse_first_fn_param(scope, tokens);

        if (std::ranges::find_if(
            parameters, [&](const u_ptr<AstFunctionParameter>& p)
            {
                return p->get_name() == param->get_name();
            }) != parameters.end())
        {
            tokens.throw_error(
                next,
                ErrorType::SEMANTIC_ERROR,
                std::format(
                    "Duplicate parameter name \"{}\" in function definition",
                    param->get_name())
            );
        }

        parameters.push_back(std::move(param));
    }
}

std::string AstFunctionParameter::to_string()
{
    return std::format("{}()", get_name(), this->get_type()->to_string());
}

u_ptr<AstFunctionParameter> stride::ast::parse_first_fn_param(
    [[maybe_unused]] const Scope& scope,
    TokenSet& tokens
)
{
    const auto reference_token = tokens.expect(TokenType::IDENTIFIER);

    tokens.expect(TokenType::COLON);

    std::unique_ptr<types::AstType> type_ptr = types::parse_primitive_type(tokens);

    return std::make_unique<AstFunctionParameter>(
        tokens.source(),
        reference_token.offset,
        reference_token.lexeme,
        std::move(type_ptr)
    );
}
