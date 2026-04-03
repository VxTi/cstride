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
    printf("Accepting function %s\n", function->get_function_name().c_str());

    const auto function_symbol = Symbol(
        function->get_source_position(),
        symbol_table->get_scope_name(),
        function->get_function_name()
    );
    // This is fine here because we only need parameter types to infer the
    // function type, and parameter types are already set at this point.
    // We only want to do this for non-generic functions, as we can't resolve underlying generic type arguments yet
    function->set_type(infer_function_type(symbol_table, function));

    if (!function->is_generic())
    {
        symbol_table->define_function(function_symbol, function);
        return;
    }

    // Generic functions aren't resolved fully here. A copy is created with the instantiated parameters further down the line
    for (const auto& param : function->get_parameters_ref())
    {
        symbol_table->define_variable(
            param->get_symbol(),
            param->get_type()->clone(),
            VisibilityModifier::PRIVATE,
            param->get_type()->get_flags()
        );
    }

    symbol_table->define_generic_function(function_symbol, function);
}

/**
 * Registers variable declarations in the symbol table, assigning them an internalized symbol and appropriate flags.
 * Safe to call more than once: already-registered variables are silently skipped.
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
 * Safe to call more than once: already-registered types are silently skipped.
 */
void SymbolResolver::accept_type_definition_node(SymbolTable* symbol_table, AstTypeDefinition* type_definition)
{
    if (symbol_table->is_type_defined(type_definition->get_name()))
        return;

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
