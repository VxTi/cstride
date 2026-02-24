#include "ast/nodes/types.h"
#include "ast/tokens/token_set.h"

using namespace stride::ast;

std::optional<std::unique_ptr<IAstType>> stride::ast::parse_function_type_optional(
    const std::shared_ptr<ParsingContext>& context,
    TokenSet& set,
    int context_type_flags
)
{
    // Must start with '('
    if (!set.peek_next_eq(TokenType::LPAREN))
    {
        return std::nullopt;
    }

    // This tries to parse `(<type>, <type>, ...) -> <return_type>`
    // With no parameters: `() -> <return_type>`
    // Arrays: ((<type>, ...) -> <return_type>)[]

    const auto reference_token = set.next();
    std::vector<std::unique_ptr<IAstType>> parameters;
    bool is_expecting_closing_paren = false;

    // If the previous paren is followed by another one,
    // then we might expect an array of functions
    // e.g., `((int32, int32) -> int32)[]
    if (set.peek_next_eq(TokenType::LPAREN))
    {
        set.next();
        is_expecting_closing_paren = true;
    }

    int recursion_depth = 0;

    while (set.has_next() && !set.peek_next_eq(TokenType::RPAREN))
    {
        parameters.push_back(
            parse_type(
                context,
                set,
                "Expected parameter type",
                context_type_flags
            )
        );
        if (set.peek_next_eq(TokenType::RPAREN))
        {
            break;
        }
        set.expect(TokenType::COMMA, "Expected ',' between function type parameters");

        if (recursion_depth++ > MAX_RECURSION_DEPTH)
        {
            set.throw_error("Maximum recursion depth exceeded when parsing function type");
        }
    }

    set.expect(TokenType::RPAREN, "Expected ')' after function type notation");
    set.expect(TokenType::DASH_RARROW, "Expected '->' between function parameters and return type");
    auto return_type = parse_type(
        context,
        set,
        "Expected return type",
        context_type_flags
    );

    if (is_expecting_closing_paren)
    {
        set.expect(TokenType::RPAREN, "Expected secondary ')' after function type notation");
    }


    auto fn_type = std::make_unique<AstFunctionType>(
        reference_token.get_source_fragment(),
        context,
        std::move(parameters),
        std::move(return_type),
        context_type_flags
    );

    return parse_type_metadata(std::move(fn_type), set, context_type_flags);
}

std::unique_ptr<IAstType> AstFunctionType::clone() const
{
    std::vector<std::unique_ptr<IAstType>> parameters_clone = {};
    for (const auto& p : this->_parameters)
    {
        parameters_clone.push_back(p->clone());
    }

    return std::make_unique<AstFunctionType>(
        this->get_source_fragment(),
        this->get_context(),
        std::move(parameters_clone),
        this->_return_type->clone(),
        this->get_flags()
    );
}

bool AstFunctionType::equals(IAstType& other)
{
    if (const auto* other_func = dynamic_cast<AstFunctionType*>(&other))
    {
        if (!other_func->get_return_type()->equals(
            *this->get_return_type()))
            return false;

        if (this->_parameters.size() != other_func->_parameters.size())
            return false;

        for (size_t i = 0; i < this->_parameters.size(); i++)
        {
            if (!this->_parameters[i]->equals(
                *other_func->_parameters[i]))
            {
                return false;
            }
        }
        return true;
    }
    return false;
}

std::string AstFunctionType::to_string()
{
    std::vector<std::string> param_strings;
    for (const auto& p : this->_parameters)
        param_strings.push_back(p->to_string());

    return std::format(
        "({}) -> {}",
        join(param_strings, ", "),
        this->_return_type->to_string());
}
