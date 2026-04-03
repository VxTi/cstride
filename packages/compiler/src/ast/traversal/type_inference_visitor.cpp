#include "ast/symbol_table.h"
#include "ast/type_inference.h"
#include "ast/visitor.h"
#include "ast/definitions/function_definition.h"
#include "ast/nodes/expression.h"
#include "ast/nodes/type_definition.h"

using namespace stride::ast;

/**
 * Registers a type definition in the symbol table so that it can be referenced in the current context.
 */
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

/**
 * Infers the type of an expression and updates it in the symbol table if it is a variable declaration.
 */
void TypeInferenceVisitor::accept_expression(SymbolTable* symbol_table, IAstExpression* expr)
{
    expr->set_type(infer_expression_type(symbol_table, expr));

    // If we've encountered a variable declaration, we must assign the inferred type to its definition
    // in the symbol table. We know the variable is defined at this point, so we can safely
    if (const auto *variable_def = dynamic_cast<AstVariableDeclaration *>(expr))
    {
        symbol_table->set_variable_type(variable_def->get_symbol(), variable_def->get_type()->clone());
    }
}
