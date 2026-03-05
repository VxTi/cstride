#include "ast/parsing_context.h"
#include "ast/type_inference.h"
#include "ast/visitor.h"
#include "ast/nodes/expression.h"
#include "ast/nodes/function_declaration.h"
#include "ast/nodes/types.h"

using namespace stride::ast;

void FunctionVisitor::accept(IAstFunction* fn_declaration)
{
    fn_declaration->set_type(infer_expression_type(fn_declaration));

    // Define parameters in the function's own context BEFORE traversing the body,
    // so that identifiers referencing params resolve correctly inside the body.
    for (const auto& param : fn_declaration->get_parameters_ref())
    {
        const auto param_symbol = Symbol(param->get_source_fragment(), param->get_name());
        fn_declaration->get_context()->define_variable(param_symbol, param->get_type()->clone_ty());
    }


    // Forward declare the function in the symbol registry
    if (dynamic_cast<AstFunctionDeclaration*>(fn_declaration))
    {
        fn_declaration->get_context()->define_function(
            fn_declaration->get_symbol(),
            fn_declaration->get_type()->clone_as<AstFunctionType>(),
            fn_declaration->get_flags()
        );
    }
}
