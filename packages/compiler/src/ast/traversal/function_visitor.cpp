#include "ast/symbol_table.h"
#include "ast/type_inference.h"
#include "ast/visitor.h"
#include "ast/nodes/expression.h"
#include "ast/nodes/function_declaration.h"
#include "ast/nodes/types.h"

using namespace stride::ast;

void FunctionVisitor::accept_function_node(SymbolTable* symbol_table, IAstFunction* function)
{
    function->set_type(infer_function_type(function));

    // Define parameters in the function's own context BEFORE traversing the body,
    // so that identifiers referencing params resolve correctly inside the body.
    for (const auto& param : function->get_parameters_ref())
    {
        const auto param_symbol = Symbol(param->get_source_position(), param->get_name());
        symbol_table->define_variable(
            param_symbol,
            param->get_type()->clone_ty(),
            VisibilityModifier::PRIVATE
        );
    }

    const auto function_symbol = Symbol(
        function->get_source_position(),
        symbol_table->get_scope_name(),
        function->get_function_name()
    );

    // Forward declare the function in the symbol registry
    symbol_table->define_function(
        function_symbol,
        function->get_type()->clone_as<AstFunctionType>(),
        function->get_visibility(),
        function->get_flags()
    );
}
