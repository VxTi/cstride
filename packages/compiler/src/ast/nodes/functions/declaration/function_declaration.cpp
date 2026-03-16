#include "ast/nodes/function_declaration.h"

#include "errors.h"
#include "ast/closures.h"
#include "ast/modifiers.h"
#include "ast/parsing_context.h"
#include "ast/symbols.h"
#include "ast/definitions/function_definition.h"
#include "ast/nodes/blocks.h"
#include "ast/nodes/expression.h"
#include "ast/nodes/return_statement.h"
#include "ast/tokens/token.h"
#include "ast/tokens/token_set.h"

#include <ranges>
#include <llvm/IR/Function.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Verifier.h>

using namespace stride::ast;
using namespace stride::ast::definition;

/**
 * Will attempt to parse the provided token stream into an AstFunctionDefinitionNode.
 */
std::unique_ptr<AstFunctionDeclaration> stride::ast::parse_fn_declaration(
    const std::shared_ptr<ParsingContext>& context,
    TokenSet& set,
    VisibilityModifier modifier
)
{
    int function_flags = 0;
    const auto reference_token = set.peek_next();
    if (set.peek_next_eq(TokenType::KEYWORD_EXTERN))
    {
        set.next();
        function_flags |= SRFLAG_FN_TYPE_EXTERN;
    }

    if (set.peek_next_eq(TokenType::KEYWORD_ASYNC))
    {
        set.next();
        function_flags |= SRFLAG_FN_TYPE_ASYNC;
    }

    set.expect(TokenType::KEYWORD_FN);

    // Here we expect to receive the function name
    const auto fn_name_tok = set.expect(TokenType::IDENTIFIER, "Expected function name");
    const auto& fn_name = fn_name_tok.get_lexeme();

    auto function_context = std::make_shared<ParsingContext>(context, ContextType::FUNCTION);

    GenericParameterList generic_parameter_names = parse_generic_declaration(set);

    if (function_flags & SRFLAG_FN_TYPE_EXTERN && !generic_parameter_names.empty())
    {
        set.throw_error("Extern functions cannot have generic parameters");
    }

    set.expect(TokenType::LPAREN, "Expected '(' after function name");
    std::vector<std::unique_ptr<AstFunctionParameter>> parameters;

    // Parameter parsing
    if (!set.peek_next_eq(TokenType::RPAREN))
    {
        parse_function_parameters(function_context, set, parameters, function_flags);

        if (!set.peek_next_eq(TokenType::RPAREN))
        {
            set.throw_error(
                "Expected closing parenthesis after variadic parameter; variadic parameter must be the last parameter in the function signature"
            );
        }
    }

    set.expect(TokenType::RPAREN, "Expected ')' after function parameters");
    set.expect(TokenType::COLON, "Expected a colon after function definition");

    // Return type doesn't have the same flags as the function, hence NONE
    auto return_type = parse_type(context, set, { "Expected return type in function header" });

    const auto& position = SourceFragment::join(
        reference_token.get_source_fragment(),
        return_type->get_source_fragment());
    auto sym_function_name = Symbol(position, context->get_name(), fn_name);

    std::unique_ptr<AstBlock> body = nullptr;

    if (function_flags & SRFLAG_FN_TYPE_EXTERN)
    {
        set.expect(TokenType::SEMICOLON, "Expected ';' after extern function declaration");
        body = AstBlock::create_empty(function_context, position);
    }
    else
    {
        body = parse_block(function_context, set);
    }

    return std::make_unique<AstFunctionDeclaration>(
        function_context,
        sym_function_name,
        std::move(parameters),
        std::move(body),
        std::move(return_type),
        modifier,
        function_flags,
        std::move(generic_parameter_names)
    );
}

std::unique_ptr<AstBlock> consume_anonymous_fn_body(
    const std::shared_ptr<ParsingContext>& context,
    TokenSet& set)
{
    if (!set.peek_next_eq(TokenType::LBRACE))
    {
        auto expr = parse_inline_expression(context, set);

        const auto src_frag = expr->get_source_fragment();
        std::vector<std::unique_ptr<IAstNode>> body_nodes;
        body_nodes.push_back(std::move(expr));

        return std::make_unique<AstBlock>(
            src_frag,
            context,
            std::move(body_nodes)
        );
    }

    return parse_block(context, set);
}

std::unique_ptr<IAstExpression> stride::ast::parse_anonymous_fn_expression(
    const std::shared_ptr<ParsingContext>& context,
    TokenSet& set
)
{
    const auto reference_token = set.peek_next();
    std::vector<std::unique_ptr<AstFunctionParameter>> parameters = {};

    int function_flags = SRFLAG_FN_TYPE_ANONYMOUS;
    auto function_context = std::make_shared<ParsingContext>(
        context,
        ContextType::FUNCTION
    );

    // Parses expressions like:
    // (<param1>: <type1>, ...): <ret_type> -> {}
    if (auto header_definition = collect_parenthesized_block(set);
        header_definition.has_value() && header_definition->has_next())
    {
        parse_function_parameters(
            function_context,
            header_definition.value(),
            parameters,
            function_flags
        );
    }

    set.expect(TokenType::COLON, "Expected ':' after lambda function header definition");
    auto return_type = parse_type(
        function_context,
        set,
        { "Expected type after anonymous function header definition" }
    );
    const auto lambda_arrow = set.expect(
        TokenType::RARROW,
        "Expected '->' after lambda parameters"
    );

    auto lambda_body = consume_anonymous_fn_body(function_context, set);

    static int anonymous_lambda_id = 0;

    auto symbol_name = Symbol(
        { set.get_source(),
          reference_token.get_source_fragment().offset,
          lambda_arrow.get_source_fragment().offset -
          reference_token.get_source_fragment().offset },
        ANONYMOUS_FN_PREFIX + std::to_string(anonymous_lambda_id++)
    );

    std::vector<std::unique_ptr<IAstType>> cloned_params;
    cloned_params.reserve(parameters.size());
    for (auto& param : parameters)
    {
        cloned_params.push_back(param->get_type()->clone_ty());
    }

    return std::make_unique<AstLambdaFunctionExpression>(
        function_context,
        symbol_name,
        std::move(parameters),
        std::move(lambda_body),
        std::move(return_type),
        VisibilityModifier::PRIVATE,
        // Anonymous functions are always private
        function_flags
    );
}

std::vector<std::unique_ptr<AstFunctionParameter>> IAstFunction::get_parameters() const
{
    std::vector<std::unique_ptr<AstFunctionParameter>> cloned_params;
    cloned_params.reserve(this->_parameters.size());

    for (const auto& param : this->_parameters)
    {
        cloned_params.push_back(param->clone_as<AstFunctionParameter>());
    }

    return cloned_params;
}

std::vector<std::unique_ptr<IAstType>> IAstFunction::get_parameter_types() const
{
    std::vector<std::unique_ptr<IAstType>> types;
    types.reserve(this->_parameters.size());

    for (const auto& param : this->_parameters)
    {
        types.push_back(param->get_type()->clone_ty());
    }

    return types;
}

FunctionDefinition* IAstFunction::get_function_definition()
{
    if (this->_function_definition != nullptr)
        return this->_function_definition;

    const auto& definition = this->get_context()->get_function_definition(
        this->get_registered_function_name(),
        this->get_parameter_types(),
        this->get_generic_parameters().size()
    );

    if (!definition.has_value())
    {
        throw parsing_error(
            ErrorType::REFERENCE_ERROR,
            std::format("Function definition for '{}' not found in context", this->get_registered_function_name()),
            this->get_source_fragment()
        );
    }

    this->_function_definition = definition.value();
    return this->_function_definition;
}

std::vector<FunctionImplementation> IAstFunction::get_function_implementation_data()
{
    const auto& definition = this->get_function_definition();

    // If the function is not generic, we just return a singular name (the regular internalized name)
    if (definition->get_generic_overloads().empty())
    {
        return {
            FunctionImplementation{ this->get_registered_function_name(), definition->get_llvm_function() }
        };
    }

    std::vector<FunctionImplementation> implementations;

    for (const auto& [types, llvm_function, node] : definition->get_generic_overloads())
    {
        implementations.emplace_back(
            get_overloaded_function_name(node->get_registered_function_name(), types),
            llvm_function
        );
    }

    return implementations;
}

std::unique_ptr<IAstNode> AstFunctionParameter::clone()
{
    return std::make_unique<AstFunctionParameter>(
        this->get_source_fragment(),
        this->get_context(),
        this->get_name(),
        this->get_type()->clone_ty()
    );
}

std::unique_ptr<IAstNode> IAstFunction::clone()
{
    std::vector<std::unique_ptr<AstFunctionParameter>> cloned_params;
    cloned_params.reserve(this->_parameters.size());

    for (const auto& param : this->_parameters)
    {
        cloned_params.push_back(param->clone_as<AstFunctionParameter>());
    }

    return std::make_unique<IAstFunction>(
        this->get_source_fragment(),
        this->get_context(),
        this->_symbol,
        std::move(cloned_params),
        this->_body->clone_as<AstBlock>(),
        this->_annotated_return_type->clone_ty(),
        this->_visibility,
        this->_flags,
        this->_generic_parameters
    );
}

std::string IAstFunction::to_string()
{
    std::string params;
    for (const auto& param : this->_parameters)
    {
        if (!params.empty())
            params += ", ";
        params += param->to_string();
    }

    const auto body_str = this->get_body() == nullptr
        ? "<empty>"
        : this->get_body()->to_string();

    return std::format(
        "Function(name: {}(internal: {}), params: [{}], body: {}{} -> {})",
        this->is_anonymous() ? "<anonymous>" : this->get_plain_function_name(),
        this->get_registered_function_name(),
        params,
        body_str,
        this->is_extern() ? " (extern)" : "",
        this->get_return_type()->to_string()
    );
}
