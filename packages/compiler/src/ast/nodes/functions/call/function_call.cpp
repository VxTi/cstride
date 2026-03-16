#include "errors.h"
#include "formatting.h"
#include "ast/casting.h"
#include "ast/flags.h"
#include "ast/parsing_context.h"
#include "ast/symbols.h"
#include "ast/definitions/function_definition.h"
#include "ast/nodes/blocks.h"
#include "ast/nodes/expression.h"
#include "ast/nodes/types.h"
#include "ast/tokens/token_set.h"

#include <format>
#include <ranges>
#include <sstream>
#include <vector>

using namespace stride::ast;
using namespace stride::ast::definition;

std::unique_ptr<IAstExpression> stride::ast::parse_function_call(
    const std::shared_ptr<ParsingContext>& context,
    AstIdentifier* identifier,
    TokenSet& set
)
{
    const auto reference_token = set.peek(-1);
    auto generic_types = parse_generic_type_arguments(context, set);
    auto function_parameter_set = collect_parenthesized_block(set);

    ExpressionList function_arg_nodes;

    int function_call_flags = SRFLAG_NONE;

    // Parsing function parameter values
    if (function_parameter_set.has_value())
    {
        auto subset = function_parameter_set.value();

        if (auto initial_arg = parse_inline_expression(context, subset))
        {
            function_arg_nodes.push_back(std::move(initial_arg));

            // Consume next parameters
            while (subset.has_next())
            {
                const auto preceding = subset.expect(
                    TokenType::COMMA,
                    "Expected ',' between function arguments"
                );

                auto function_argument = parse_inline_expression(context, subset);

                if (!function_argument)
                {
                    // Since the RParen is already consumed, we have to manually extract its
                    // position with the following assumption It's possible this yields END_OF_FILE
                    const auto len =
                        set.peek(-1).get_source_fragment().offset - 1 -
                        preceding.get_source_fragment().offset;
                    throw parsing_error(
                        ErrorType::SYNTAX_ERROR,
                        "Expected expression for function argument",
                        SourceFragment(
                            subset.get_source(),
                            preceding.get_source_fragment().offset + 1,
                            len)
                    );
                }

                // If the next argument is a variadic argument reference, we stop parsing more arguments and mark this function call as variadic
                if (cast_expr<AstVariadicArgReference*>(function_argument.get()))
                {
                    if (subset.has_next())
                    {
                        subset.throw_error(
                            "Variadic argument propagation must be the last parameter in a function call"
                        );
                    }
                    function_call_flags |= SRFLAG_FN_TYPE_VARIADIC;

                    break;
                }

                function_arg_nodes.push_back(std::move(function_argument));
            }
        }
    }

    return std::make_unique<AstFunctionCall>(
        context,
        identifier->clone_as<AstIdentifier>(),
        std::move(function_arg_nodes),
        std::move(generic_types),
        function_call_flags
    );
}

// The previous token must be an identifier, otherwise this is not a function call
// This logic is not handled here.
bool stride::ast::is_direct_function_call(const TokenSet& set)
{
    // Function call for sure, "identifier::other(`
    if (set.peek_next_eq(TokenType::LPAREN))
        return true;

    // If the subsequent token is a LT, it might just be a generic function instantiation
    // We have to do some lookahead to make sure this is the case
    if (set.peek_next_eq(TokenType::LT))
    {
        int depth = 0;
        for (size_t offset = 0; set.position() + offset < set.size(); offset++)
        {
            switch (const auto next_token = set.at(set.position() + offset);
                next_token.get_type())
            {
            case TokenType::LT:
                ++depth;
                break;
            case TokenType::GT:
                --depth;
                break;

            default:
                // Optimization, where we know for sure it can't be part of a generic instantiation
                if (!next_token.is_type_token() && next_token.get_type() != TokenType::COMMA)
                {
                    return false;
                }
            }

            if (depth == 0)
            {
                return set.peek_eq(TokenType::LPAREN, offset + 1);
            }
        }
    }
    return false;
}

std::string AstFunctionCall::format_suggestion(const IDefinition* suggestion)
{
    if (const auto fn_call = dynamic_cast<const FunctionDefinition*>(suggestion))
    {
        // We'll format the arguments
        std::vector<std::string> arg_types;

        for (const auto& arg : fn_call->get_type()->get_parameter_types())
        {
            arg_types.push_back(arg->get_type_name());
        }

        if (arg_types.empty())
            arg_types.push_back(primitive_type_to_str(PrimitiveType::VOID));

        return std::format("{}({})",
                           fn_call->get_symbol().name,
                           join(arg_types, ", "));
    }

    return suggestion->get_internal_symbol_name();
}

std::string AstFunctionCall::format_function_name() const
{
    std::vector<std::string> arg_types;

    arg_types.reserve(this->_arguments.size());

    for (const auto& arg : this->_arguments)
    {
        arg_types.push_back(arg->get_type()->to_string());
    }

    if (arg_types.empty())
    {
        arg_types.push_back(primitive_type_to_str(PrimitiveType::VOID));
    }

    return std::format("{}({})", this->get_function_name(), join(arg_types, ", "));
}

std::vector<std::unique_ptr<IAstType>> AstFunctionCall::get_argument_types() const
{
    if (this->_arguments.empty())
        return {};

    std::vector<std::unique_ptr<IAstType>> param_types;
    param_types.reserve(this->_arguments.size());
    for (const auto& arg : this->_arguments)
    {
        // The last parameter should be the variadic argument reference,
        // which is not included in the type list, as this is dynamically expanded
        // Additionally, this would mess with function lookup by signature
        if (cast_expr<AstVariadicArgReference*>(arg.get()))
        {
            break;
        }
        param_types.push_back(arg->get_type()->clone_ty());
    }
    return param_types;
}

const GenericTypeList& AstFunctionCall::get_generic_type_arguments()
{
    return this->_generic_type_arguments;
}

IDefinition* AstFunctionCall::get_function_definition()
{
    if (this->_definition)
        return this->_definition;

    if (const auto def = this->get_context()->get_function_definition(
            this->get_scoped_function_name(),
            this->get_argument_types(),
            this->get_generic_type_arguments().size()
        );
        def.has_value())
    {
        this->_definition = def.value();
        return this->_definition;
    }

    if (const auto field_def = this->get_context()->lookup_variable(
        this->get_scoped_function_name(),
        true
    ))
    {
        this->_definition = field_def;
        return this->_definition;
    }

    throw parsing_error(
        ErrorType::REFERENCE_ERROR,
        std::format("Function '{}' was not found in this scope", this->format_function_name()),
        this->get_source_fragment()
    );
}

std::unique_ptr<IAstNode> AstFunctionCall::clone()
{
    ExpressionList cloned_args;
    GenericTypeList generic_type_list_cloned;

    generic_type_list_cloned.reserve(this->_generic_type_arguments.size());
    cloned_args.reserve(this->get_arguments().size());

    for (const auto& arg : this->get_arguments())
    {
        cloned_args.push_back(arg->clone_as<IAstExpression>());
    }

    for (const auto& generic_arg : this->get_generic_type_arguments())
    {
        generic_type_list_cloned.push_back(generic_arg->clone_ty());
    }

    return std::make_unique<AstFunctionCall>(
        this->get_context(),
        this->get_function_name_identifier()->clone_as<AstIdentifier>(),
        std::move(cloned_args),
        std::move(generic_type_list_cloned),
        this->_flags
    );
}

bool AstFunctionCall::is_reducible()
{
    // TODO: implement
    // Function calls can be reducible if the function returns
    // a constant value or if all arguments are reducible.
    return false;
}

std::optional<std::unique_ptr<IAstNode>> AstFunctionCall::reduce()
{
    return std::nullopt;
}

std::string AstFunctionCall::get_formatted_call() const
{
    std::vector<std::string> arg_names;
    arg_names.reserve(this->_arguments.size());

    std::vector<std::string> formatted_generics;
    formatted_generics.reserve(this->_generic_type_arguments.size());

    for (const auto& generic_arg : this->_generic_type_arguments)
    {
        formatted_generics.push_back(generic_arg->get_type_name());
    }

    for (const auto& arg : this->_arguments)
    {
        arg_names.push_back(arg->get_type()->get_type_name());
    }

    if (!formatted_generics.empty())
    {
        return std::format(
            "{}<{}>({})",
            this->get_function_name(),
            join(formatted_generics, ", "),
            join(arg_names, ", ")
        );
    }

    return std::format(
        "{}({})",
        this->get_function_name(),
        join(arg_names, ", ")
    );
}

std::string AstFunctionCall::to_string()
{
    std::ostringstream oss;

    std::vector<std::string> arg_types;
    for (const auto& arg : this->get_arguments())
    {
        arg_types.push_back(arg->to_string());
    }

    return std::format(
        "FunctionCall({} [{}])",
        this->get_function_name(),
        join(arg_types, ", ")
    );
}
