#include "ast/symbol_table.h"
#include "ast/type_inference.h"
#include "ast/visitor.h"
#include "ast/nodes/expression.h"
#include "ast/nodes/function_declaration.h"
#include "ast/nodes/types.h"

using namespace stride::ast;

/**
 * Infers function types and defines the function and its parameters in the symbol table.
 */
void SymbolResolver::accept_function_node(SymbolTable* symbol_table, IAstFunction* function)
{
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

    const auto function_symbol = Symbol(
        function->get_source_position(),
        symbol_table->get_scope_name(),
        function->get_function_name() // TODO: Internalize name
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

    const auto variable_symbol = Symbol(
        var_decl->get_source_position(),
        symbol_table->get_scope_name(),
        var_decl->get_variable_name(),
        // TODO: Validate whether this internalized name is sufficient
        std::format("{}{}", var_decl->get_variable_name(), ++var_counter)
    );

    var_decl->set_symbol(variable_symbol);

    symbol_table->define_variable(
        variable_symbol,
        var_decl->get_visibility(),
        flags
    );
}
