#include "ast/symbol_table.h"
#include "ast/type_inference.h"
#include "ast/visitor.h"
#include "ast/definitions/function_definition.h"
#include "ast/nodes/expression.h"
#include "ast/nodes/type_definition.h"

using namespace stride::ast;

void TypeInferenceVisitor::accept_type_definition_node(SymbolTable* symbol_table, AstTypeDefinition* type_definition)
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

void TypeInferenceVisitor::accept_expression(SymbolTable* symbol_table, IAstExpression* expr)
{
    expr->set_type(infer_expression_type(symbol_table, expr));

    // --- Variable declaration: register the resolved type in the local context
    // so that subsequent expressions in the same scope can look up the variable.
    if (const auto* var_decl = dynamic_cast<AstVariableDeclaration*>(expr))
    {
        // Use the initial value's type (already set by bottom-up traversal) as the
        // canonical type registered in context, which is what identifier lookups rely on.
        const auto inferred_type = var_decl->get_type();

        const auto is_global_variable = symbol_table->get_context_type() == ContextType::GLOBAL;

        // Ensure global variables keep their global name
        if (is_global_variable)
        {
            inferred_type->set_flags(inferred_type->get_flags() | SRFLAG_TYPE_GLOBAL);
        }

        inferred_type->set_flags(var_decl->get_flags()); // Ensure type flags are preserved

        const auto variable_symbol = Symbol(
            var_decl->get_source_position(),
            symbol_table->get_scope_name(),
            var_decl->get_variable_name()
        );

        symbol_table->define_variable(
            variable_symbol,
            inferred_type->clone(),
            var_decl->get_visibility()
        );
    }
}
