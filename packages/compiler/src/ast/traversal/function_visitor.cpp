#include "ast/parsing_context.h"
#include "ast/type_inference.h"
#include "ast/visitor.h"
#include "ast/nodes/expression.h"
#include "ast/nodes/function_declaration.h"
#include "ast/nodes/types.h"

using namespace stride::ast;

void FunctionVisitor::accept(IAstFunction* function)
{
    function->set_type(infer_function_type(function));

    // Define parameters in the function's own context BEFORE traversing the body,
    // so that identifiers referencing params resolve correctly inside the body.
    for (const auto& param : function->get_parameters_ref())
    {
        const auto param_symbol = Symbol(param->get_source_fragment(), param->get_name());
        function->get_context()->define_variable(
            param_symbol,
            param->get_type()->clone_ty(),
            VisibilityModifier::PRIVATE
        );
    }

    // Forward declare the function in the symbol registry
    function->get_context()->define_function(
        function->get_symbol(),
        function->get_type()->clone_as<AstFunctionType>(),
        function->get_visibility(),
        function->get_flags()
    );
}
