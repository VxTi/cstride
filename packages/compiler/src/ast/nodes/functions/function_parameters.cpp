#include "ast/nodes/function_declaration.h"

using namespace stride::ast;

void stride::ast::parse_variadic_fn_param(
    const std::shared_ptr<SymbolRegistry>& scope,
    TokenSet& tokens,
    std::vector<std::unique_ptr<AstFunctionParameter>>& parameters
)
{
    const auto reference_token = tokens.expect(TokenType::THREE_DOTS);
    const auto param = parse_standalone_fn_param(scope, tokens);

    param->get_type()->set_flags(param->get_type()->get_flags() | SRFLAG_TYPE_VARIADIC);

    // Create a new parameter with the variadic flag
    parameters.push_back(std::make_unique<AstFunctionParameter>(
        param->get_source(),
        param->get_source_position(),
        param->get_registry(),
        param->get_name(),
        param->get_type()->clone()
    ));
}

void stride::ast::parse_subsequent_fn_params(
    const std::shared_ptr<SymbolRegistry>& scope,
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
            throw parsing_error(
                make_source_error(
                    ErrorType::SYNTAX_ERROR,
                    "Function cannot have more than " + std::to_string(MAX_FUNCTION_PARAMETERS) + " parameters",
                    *set.get_source(),
                    next.get_source_position()
                )
            );
        }

        if (next.get_type() == TokenType::THREE_DOTS)
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
        }

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
    const std::shared_ptr<SymbolRegistry>& scope,
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
    const auto param_name = reference_token.get_lexeme();
    set.expect(TokenType::COLON);

    std::unique_ptr<IAstInternalFieldType> fn_param_type = parse_type(
        scope, set, "Expected function parameter type", flags
    );

    scope->define_field(param_name, reference_token.get_lexeme(), fn_param_type->clone());

    return std::make_unique<AstFunctionParameter>(
        set.get_source(),
        reference_token.get_source_position(),
        scope,
        reference_token.get_lexeme(),
        std::move(fn_param_type)
    );
}
