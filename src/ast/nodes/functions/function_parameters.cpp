#include "ast/nodes/function_definition.h"

using namespace stride::ast;

void stride::ast::parse_variadic_fn_param(
    const std::shared_ptr<Scope>& scope,
    TokenSet& tokens,
    std::vector<std::unique_ptr<AstFunctionParameter>>& parameters
)
{
    const auto reference_token = tokens.expect(TokenType::THREE_DOTS);
    auto param = parse_standalone_fn_param(scope, tokens);
    parameters.push_back(std::move(param));
}

void stride::ast::parse_subsequent_fn_params(
    const std::shared_ptr<Scope>& scope,
    TokenSet& set,
    std::vector<std::unique_ptr<AstFunctionParameter>>& parameters
)
{
    while (set.peak_next_eq(TokenType::COMMA))
    {
        set.next(); // Skip comma
        const auto next = set.peak_next();

        if (parameters.size() > MAX_FUNCTION_PARAMETERS)
        {
            throw parsing_error(make_ast_error(
                *set.source(),
                next.offset,
                "Function cannot have more than " + std::to_string(MAX_FUNCTION_PARAMETERS) + " parameters"
            ));
        }

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
    const auto name = this->get_name();
    auto type_str = this->get_type()->to_string();
    return std::format("{}({})", name, type_str);
}

std::unique_ptr<AstFunctionParameter> stride::ast::parse_standalone_fn_param(
    const std::shared_ptr<Scope>& scope,
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
    const auto param_name = reference_token.lexeme;
    set.expect(TokenType::COLON);

    std::unique_ptr<IAstInternalFieldType> fn_param_type = parse_ast_type(set);

    scope->define_field(param_name, reference_token.lexeme, fn_param_type.get(), flags);

    return std::make_unique<AstFunctionParameter>(
        set.source(),
        reference_token.offset,
        reference_token.lexeme,
        std::move(fn_param_type),
        flags
    );
}
