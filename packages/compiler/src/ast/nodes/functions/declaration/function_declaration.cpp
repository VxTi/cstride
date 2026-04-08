#include "ast/nodes/function_declaration.h"

#include "errors.h"
#include "ast/closures.h"
#include "ast/modifiers.h"
#include "ast/symbol_table.h"
#include "ast/type_inference.h"
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

using namespace stride::ast;
using namespace stride::ast::definition;

/**
 * Will attempt to parse the provided token stream into an AstFunctionDefinitionNode.
 */
std::unique_ptr<AstFunctionDeclaration> stride::ast::parse_fn_declaration(TokenSet& set, VisibilityModifier modifier)
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
        parse_function_parameters(set, parameters, function_flags);

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
    auto return_type = parse_type(set, { "Expected return type in function header" });

    const auto position = SourcePosition::join(
        reference_token.get_source_position(),
        return_type->get_source_position()
    );

    std::unique_ptr<AstBlock> body = nullptr;

    if (function_flags & SRFLAG_FN_TYPE_EXTERN)
    {
        set.expect(TokenType::SEMICOLON, "Expected ';' after extern function declaration");
        body = AstBlock::create_empty(position);
    }
    else
    {
        body = parse_block(set);
    }

    return std::make_unique<AstFunctionDeclaration>(
        position,
        fn_name,
        std::move(parameters),
        std::move(body),
        std::move(return_type),
        modifier,
        function_flags,
        std::move(generic_parameter_names)
    );
}

std::unique_ptr<AstBlock> consume_anonymous_fn_body(TokenSet& set)
{
    if (!set.peek_next_eq(TokenType::LBRACE))
    {
        auto expr = parse_inline_expression(set);

        const auto src_frag = expr->get_source_position();
        std::vector<std::unique_ptr<IAstNode>> body_nodes;
        body_nodes.push_back(std::move(expr));

        return std::make_unique<AstBlock>(
            src_frag,
            std::move(body_nodes)
        );
    }

    return parse_block(set);
}

std::unique_ptr<IAstExpression> stride::ast::parse_anonymous_fn_expression(TokenSet& set)
{
    const auto reference_token = set.peek_next();
    std::vector<std::unique_ptr<AstFunctionParameter>> parameters = {};

    int function_flags = SRFLAG_FN_TYPE_ANONYMOUS;

    // Parses expressions like:
    // (<param1>: <type1>, ...): <ret_type> -> {}
    if (auto header_definition = collect_parenthesized_block(set);
        header_definition.has_value() && header_definition->has_next())
    {
        parse_function_parameters(
            header_definition.value(),
            parameters,
            function_flags
        );
    }

    set.expect(TokenType::COLON, "Expected ':' after lambda function header definition");
    auto return_type = parse_type(
        set,
        { "Expected type after anonymous function header definition" }
    );
    const auto lambda_arrow = set.expect(
        TokenType::RARROW,
        "Expected '->' after lambda parameters"
    );

    auto lambda_body = consume_anonymous_fn_body(set);

    static int anonymous_lambda_id = 0;

    const auto lambda_function_name = std::format("{}{}", ANONYMOUS_FN_PREFIX, anonymous_lambda_id++);

    std::vector<std::unique_ptr<IAstType>> cloned_params;
    cloned_params.reserve(parameters.size());
    for (auto& param : parameters)
    {
        cloned_params.push_back(param->get_type()->clone());
    }

    return std::make_unique<AstLambdaFunctionExpression>(
        SourcePosition::join(reference_token.get_source_position(), lambda_body->get_source_position()),
        lambda_function_name,
        std::move(parameters),
        std::move(lambda_body),
        std::move(return_type),
        // Anonymous functions are always private
        VisibilityModifier::PRIVATE,
        function_flags
    );
}

std::shared_ptr<IAstFunction> IAstFunction::instantiate_generic_function_template(
    const SymbolTable* symbol_table,
    const GenericTypeList& instantiated_types)
{
    auto instantiated_return_ty = resolve_generics(
        this->_annotated_return_type.get(),
        this->_generic_parameters,
        instantiated_types
    );

    std::vector<std::unique_ptr<AstFunctionParameter>> instantiated_function_params;
    instantiated_function_params.reserve(this->_parameters.size());

    printf("+ FUNCTION INSTANTIATION - %s<", this->get_function_name().c_str());
    for (const auto& type : instantiated_types)
    {
        printf("%s", type->get_type_name().c_str());
    }
    printf(">\n");

    auto resolved_body = this->_body->clone_as<AstBlock>();

    const auto resolved_symbol_table = symbol_table->empty_copy();

    // Explicitly reset the symbol table after cloning to prevent duplicate symbol registration
    resolved_body->set_symbol_table(resolved_symbol_table);

    for (const auto& param : this->_parameters)
    {
        instantiated_function_params.push_back(
            std::make_unique<AstFunctionParameter>(
                param->get_source_position(),
                param->get_name(),
                resolve_generics(param->get_type(), this->_generic_parameters, instantiated_types)
            )
        );
    }

    // Define all generic parameters in the symbol table as reference type to the instantiation
    // This makes type resolution easier, as any reference to a generic parameter in the function
    // body will be resolved to the correct instantiation type.
    for (size_t i = 0; i < this->_generic_parameters.size(); ++i)
    {
        resolved_symbol_table->define_generic_type_alias(
            Symbol(this->_generic_parameters[i].position, this->_generic_parameters[i].name),
            instantiated_types[i]->clone()
        );
    }

    auto instantiation = std::make_shared<IAstFunction>(
        this->get_source_position(),
        this->get_function_name(),
        std::move(instantiated_function_params),
        std::move(resolved_body),
        std::move(instantiated_return_ty),
        this->get_visibility(),
        this->get_flags(),
        EMPTY_GENERIC_PARAMETER_LIST
    );

    this->_generic_instantiations.push_back(instantiation);
    instantiation->set_type(infer_function_type(instantiation.get()));

    return instantiation;
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
        types.push_back(param->get_type()->clone());
    }

    return types;
}

FunctionDefinition* IAstFunction::get_function_definition(SymbolTable* symbol_table)
{
    if (this->_function_definition != nullptr)
        return this->_function_definition;

    const auto& definition = symbol_table->get_function_definition(
        this->get_function_name(),
        this->get_parameter_types(),
        this->get_generic_parameters().size()
    );

    if (!definition.has_value())
    {
        throw stride_error(
            ErrorType::REFERENCE_ERROR,
            std::format("Function definition for '{}' not found in context", this->get_function_name()),
            this->get_source_position()
        );
    }

    this->_function_definition = definition.value();
    return this->_function_definition;
}

std::vector<FunctionImplementation> IAstFunction::get_function_implementation_data(SymbolTable* symbol_table)
{
    const auto& definition = this->get_function_definition(symbol_table);

    // If the function is generic, we return its instantiated overloads.
    // If it's generic but has no overloads, return empty list.
    if (this->is_generic())
    {
        std::vector<FunctionImplementation> implementations;

        for (const auto& [types, llvm_function, node] : definition->get_generic_overloads())
        {
            implementations.emplace_back(
                get_overloaded_function_name(node->get_function_name(), types),
                llvm_function,
                node->get_body()
            );
        }

        return implementations;
    }

    // For non-generic functions, return the single implementation.
    return {
        FunctionImplementation{ this->get_function_name(), definition->get_llvm_function() }
    };
}

std::unique_ptr<IAstNode> AstFunctionParameter::clone()
{
    return std::make_unique<AstFunctionParameter>(
        this->get_source_position(),
        this->get_name(),
        this->get_type()->clone()
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
        this->get_source_position(),
        this->get_function_name(),
        std::move(cloned_params),
        this->_body->clone_as<AstBlock>(),
        this->_annotated_return_type->clone(),
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
        this->is_anonymous() ? "<anonymous>" : this->get_function_name(),
        this->get_function_name(),
        params,
        body_str,
        this->is_extern() ? " (extern)" : "",
        this->get_return_type()->get_type_name()
    );
}
