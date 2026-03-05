#include "ast/parsing_context.h"
#include "ast/type_inference.h"
#include "ast/visitor.h"
#include "ast/nodes/expression.h"

using namespace stride::ast;

void ExpressionVisitor::accept(IAstExpression* expr)
{
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