#include "ast/symbol_table.h"
#include "ast/type_inference.h"
#include "ast/visitor.h"
#include "ast/nodes/expression.h"
#include "ast/nodes/function_declaration.h"
#include "ast/nodes/types.h"
#include "ast/nodes/type_definition.h"

using namespace stride::ast;

/**
 * Infers function types and defines the function and its parameters in the symbol table.
 */
void SymbolResolver::accept_function_node(SymbolTable* symbol_table, IAstFunction* function)
{
    // This is fine here because we only need parameter types to infer the
    // function type, and parameter types are already set at this point.
    function->set_type(infer_function_type(function));

    for (const auto& param : function->get_parameters_ref())
    {
        const auto param_symbol = Symbol(param->get_source_position(), param->get_name());
        symbol_table->define_variable(
            param_symbol,
            param->get_type()->clone(),
            VisibilityModifier::PRIVATE,
            param->get_type()->get_flags()
        );
    }

    // static int function_counter = 0;

    const auto internalized_name = /*function->is_extern()
        ? function->get_function_name()
        : std::format("{}.{}", function->get_function_name(), ++function_counter);*/
        function->get_function_name();

    const auto function_symbol = Symbol(
        function->get_source_position(),
        symbol_table->get_scope_name(),
        function->get_function_name(),
        internalized_name
    );

    // Forward declare the function in the symbol registry
    symbol_table->define_function(
        function_symbol,
        function->get_type()->clone_as<AstFunctionType>(),
        function->get_visibility(),
        function->get_flags()
    );
}

/**
 * Registers variable declarations in the symbol table, assigning them an internalized symbol and appropriate flags.
 */
void SymbolResolver::accept_expression(SymbolTable* symbol_table, IAstExpression* expr)
{
    auto* var_decl = dynamic_cast<AstVariableDeclaration*>(expr);
    if (!var_decl)
        return;

    const auto is_global_variable = symbol_table->get_context_type() == ContextType::GLOBAL;
    int flags = var_decl->get_flags();

    // Ensure global variables keep their global name
    if (is_global_variable)
        flags |= SRFLAG_TYPE_GLOBAL;

    static int var_counter = 0;

    // TODO: Validate whether this internalized name is sufficient
    const auto internalized_name = is_global_variable
        ? std::format("{}.{}", var_decl->get_variable_name(), ++var_counter)
        : var_decl->get_variable_name();

    const auto variable_symbol = Symbol(
        var_decl->get_source_position(),
        symbol_table->get_scope_name(),
        var_decl->get_variable_name(),
        internalized_name
    );

    var_decl->set_symbol(variable_symbol);

    symbol_table->define_variable(
        variable_symbol,
        var_decl->get_visibility(),
        flags
    );
}

/**
 * Registers a type definition in the symbol table so that it can be referenced in the current context.
 */
void SymbolResolver::accept_type_definition_node(SymbolTable* symbol_table, AstTypeDefinition* type_definition)
{
    // Register the type definition in the current context so that it can be referenced by subsequent expressions.
    const auto type_symbol = Symbol(
        type_definition->get_source_position(),
        symbol_table->get_scope_name(),
        type_definition->get_name()
    );

    symbol_table->define_type(
        type_symbol,
        type_definition->get_type()->clone(),
        type_definition->get_generic_parameters(),
        type_definition->get_visibility()
    );
}
