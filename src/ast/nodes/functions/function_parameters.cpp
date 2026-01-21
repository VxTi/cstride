#include "ast/nodes/function_definition.h"

using namespace stride::ast;

void stride::ast::parse_variadic_fn_param(
    const Scope& scope,
    TokenSet& tokens,
    std::vector<std::unique_ptr<AstFunctionParameter>>& parameters
)
{
    const auto reference_token = tokens.expect(TokenType::THREE_DOTS);
    auto param = parse_standalone_fn_param(scope, tokens);
    parameters.push_back(std::move(param));
}

void stride::ast::parse_subsequent_fn_params(
    const Scope& scope,
    TokenSet& set,
    std::vector<std::unique_ptr<AstFunctionParameter>>& parameters
)
{
    while (set.peak_next_eq(TokenType::COMMA))
    {
        set.next(); // Skip comma
        const auto next = set.peak_next();

        // If the next token is `...`, then we assume it's variadic. This parsing is done elsewhere.
        /* TODO: Further implement
         *if (next.type == TokenType::THREE_DOTS)
        {
            parse_variadic_fn_param(scope, set, parameters);
            if (!set.peak_next_eq(TokenType::RPAREN))
            {
                set.throw_error(
                    next,
                    ErrorType::SYNTAX_ERROR,
                    "Expected closing parenthesis after variadic parameter; variadic parameter must be last parameter"
                );
            }
            return;
        }*/

        auto param = parse_standalone_fn_param(scope, set);

        if (std::ranges::find_if(
            parameters, [&](const std::unique_ptr<AstFunctionParameter>& p)
            {
                return p->get_name() == param->get_name();
            }) != parameters.end())
        {
            set.throw_error(
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
    return std::format("{}({})", get_name(), this->get_type()->to_string());
}

std::unique_ptr<AstFunctionParameter> stride::ast::parse_standalone_fn_param(
    [[maybe_unused]] const Scope& scope,
    TokenSet& set
)
{
    int flags = 0;

    if (set.peak_next_eq(TokenType::KEYWORD_MUT))
    {
        flags |= SRFLAG_FN_PARAM_DEF_MUTABLE;
        set.next();
    }

    const auto reference_token = set.expect(TokenType::IDENTIFIER, "Expected a function parameter name");

    set.expect(TokenType::COLON);

    std::unique_ptr<IAstInternalFieldType> type_ptr = parse_ast_type(set);

    return std::make_unique<AstFunctionParameter>(
        set.source(),
        reference_token.offset,
        reference_token.lexeme,
        std::move(type_ptr),
        flags
    );
}
