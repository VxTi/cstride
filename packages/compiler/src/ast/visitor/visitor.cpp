#include "../../../include/ast/visitor.h"

#include "ast/parsing_context.h"
#include "ast/type_inference.h"
#include "ast/nodes/expression.h"
#include "ast/nodes/function_declaration.h"
#include "ast/nodes/types.h"

using namespace stride::ast;

void TypeInferenceVisitor::accept(IAstExpression* expr)
{
    // Infer and store the expression's type
    expr->set_type(infer_expression_type(expr));

    // --- Variable declaration: register the resolved type in the local context
    // so that subsequent expressions in the same scope can look up the variable.
    if (const auto* var_decl = dynamic_cast<AstVariableDeclaration*>(expr))
    {
        // Use the initial value's type (already set by bottom-up traversal) as the
        // canonical type registered in context, which is what identifier lookups rely on.
        const auto canonical_type = var_decl->get_type();
        canonical_type->set_flags(var_decl->get_flags()); // Ensure type flags are preserved

        var_decl->get_context()->define_variable(
            var_decl->get_symbol(),
            canonical_type->clone_ty()
        );
    }
}

void FunctionDeclareVisitor::accept(AstFunctionDeclaration* fn_declaration)
{
    // Define parameters in the function's own context BEFORE traversing the body,
    // so that identifiers referencing params resolve correctly inside the body.
    for (const auto& param : fn_declaration->get_parameters_ref())
    {
        const auto param_symbol = Symbol(param->get_source_fragment(), param->get_name());
        fn_declaration->get_context()->define_variable(param_symbol, param->get_type()->clone_ty());
    }
    fn_declaration->set_type(infer_expression_type(fn_declaration));
    fn_declaration->get_context()->define_function(
        fn_declaration->get_symbol(),
        fn_declaration->get_type()->clone_as<AstFunctionType>(),
        fn_declaration->get_flags()
    );
}
