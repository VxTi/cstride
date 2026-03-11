#include "ast/nodes/function_declaration.h"
#include "ast/tokens/token.h"
#include "ast/tokens/token_set.h"

using namespace stride::ast;

void stride::ast::parse_function_parameters(
    const std::shared_ptr<ParsingContext>& context,
    TokenSet& set,
    std::vector<std::unique_ptr<AstFunctionParameter>>& parameters,
    int& function_flags
)
{
    // If the first argument is variadic, no other parameters are allowed.
    if (set.peek_next_eq(TokenType::THREE_DOTS))
    {
        function_flags |= SRFLAG_FN_TYPE_VARIADIC;
        set.next();
        return;
    }

    parse_standalone_fn_param(context, set, parameters);

    int recursion_depth = 0;
    while (set.peek_next_eq(TokenType::COMMA))
    {
        set.next(); // Skip comma
        const auto next = set.peek_next();

        if (parameters.size() > MAX_FUNCTION_PARAMETERS)
        {
            throw parsing_error(
                ErrorType::SYNTAX_ERROR,
                std::format("Function cannot have more than {} parameters", MAX_FUNCTION_PARAMETERS),
                next.get_source_fragment()
            );
        }

        // Variadic arguments are noted like "..." and must be the last parameter,
        // so if we encounter it, we can break out of the loop immediately.
        if (next.get_type() == TokenType::THREE_DOTS)
        {
            function_flags |= SRFLAG_FN_TYPE_VARIADIC;
            set.next();
            break;
        }

        parse_standalone_fn_param(context, set, parameters);

        if (recursion_depth++ > MAX_RECURSION_DEPTH)
        {
            set.throw_error(
                "Maximum recursion depth exceeded when parsing function parameters"
            );
        }
    }
}

void stride::ast::parse_standalone_fn_param(
    const std::shared_ptr<ParsingContext>& context,
    TokenSet& set,
    std::vector<std::unique_ptr<AstFunctionParameter>>& parameters
)
{
    int flags = 0;

    if (set.peek_next_eq(TokenType::KEYWORD_LET))
    {
        flags |= SRFLAG_FN_PARAM_DEF_MUTABLE;
        set.next();
    }

    const auto reference_token =
        set.expect(TokenType::IDENTIFIER, "Expected a function parameter name");
    set.expect(TokenType::COLON);

    std::unique_ptr<IAstType> fn_param_type = parse_type(
        context,
        set,
        { "Expected function parameter type", "", flags }
    );

    const auto& param_name = reference_token.get_lexeme();

    if (std::ranges::find_if(
        parameters,
        [&](const std::unique_ptr<AstFunctionParameter>& p)
        {
            return p->get_name() == param_name;
        }) != parameters.end())
    {
        TokenSet::throw_error(
            reference_token,
            ErrorType::SEMANTIC_ERROR,
            std::format(
                "Duplicate parameter name '{}' in function definition",
                param_name
            )
        );
    }

    parameters.push_back(std::make_unique<AstFunctionParameter>(
        reference_token.get_source_fragment(),
        context,
        param_name,
        std::move(fn_param_type)
    ));
}
