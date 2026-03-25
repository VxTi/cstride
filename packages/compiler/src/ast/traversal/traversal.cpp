#include "../../../include/ast/traversal.h"

#include "ast/ast.h"
#include "ast/casting.h"
#include "ast/symbol_table.h"
#include "ast/visitor.h"
#include "ast/nodes/ast_node.h"
#include "ast/nodes/blocks.h"
#include "ast/nodes/conditional_statement.h"
#include "ast/nodes/expression.h"
#include "ast/nodes/for_loop.h"
#include "ast/nodes/function_declaration.h"
#include "ast/nodes/import.h"
#include "ast/nodes/module.h"
#include "ast/nodes/package.h"
#include "ast/nodes/return_statement.h"
#include "ast/nodes/while_loop.h"

#include <ranges>

using namespace stride::ast;

void AstNodeTraverser::traverse(IVisitor* visitor, const AstBranch* branch)
{
    this->_context_name = "";
    this->_current_context_type = ContextType::GLOBAL;
    this->_current_symbol_table = this->_root_symbol_table;

    this->visit(visitor, branch->get_node());
}

void AstNodeTraverser::visit_block(IVisitor* visitor, const AstBlock* node)
{
    if (!node)
        return;

    for (const auto& child : node->get_children())
    {
        visit(visitor, child.get());
    }
}

void AstNodeTraverser::visit_expression(IVisitor* visitor, IAstExpression* node)
{
    if (!node)
        return;

    // IAstFunction is an expression but needs special handling (body traversal + params)
    if (auto* fn = cast_expr<IAstFunction*>(node))
    {
        visit_block(visitor, fn->get_body());
        visitor->accept_function_node(fn);
    }
    else if (const auto* binary = cast_expr<IBinaryOp*>(node))
    {
        visit_expression(visitor, binary->get_left());
        visit_expression(visitor, binary->get_right());
    }
    else if (const auto* unary = cast_expr<AstUnaryOp*>(node))
    {
        visit_expression(visitor, &unary->get_operand());
    }
    else if (const auto* var_decl = cast_expr<AstVariableDeclaration*>(node))
    {
        if (var_decl->get_initial_value())
            visit_expression(visitor, var_decl->get_initial_value());
    }
    else if (auto* fn_call = cast_expr<AstFunctionCall*>(node))
    {
        for (const auto& arg : fn_call->get_arguments())
            visit_expression(visitor, arg.get());

        visitor->accept_function_call(fn_call);
    }
    else if (const auto* array = cast_expr<AstArray*>(node))
    {
        for (const auto& elem : array->get_elements())
            visit_expression(visitor, elem.get());
    }
    else if (const auto* array_accessor = cast_expr<AstArrayMemberAccessor*>(node))
    {
        visit_expression(visitor, array_accessor->get_array_base());
        visit_expression(visitor, array_accessor->get_index());
    }
    else if (const auto* struct_init = cast_expr<AstObjectInitializer*>(node))
    {
        for (const auto& val : struct_init->get_initializers() | std::views::values)
            visit_expression(visitor, val.get());
    }
    else if (const auto* tuple_init = cast_expr<AstTupleInitializer*>(node))
    {
        for (const auto& member : tuple_init->get_members())
            visit_expression(visitor, member.get());
    }
    else if (const auto* reassign = cast_expr<AstVariableReassignment*>(node))
    {
        visit_expression(visitor, reassign->get_identifier());
        visit_expression(visitor, reassign->get_value());
    }
    else if (const auto* chained = cast_expr<AstChainedExpression*>(node))
    {
        // Visit the base expression so its type is resolved before the accessor's type is inferred.
        visit_expression(visitor, chained->get_base());
        // visit_expression(visitor, chained->get_followup());
    }
    else if (auto* function_node = cast_expr<IAstFunction*>(node))
    {
        visit_block(visitor, function_node->get_body());
        visitor->accept_function_node(function_node);
    }
    else if (auto* type_cast = cast_expr<AstTypeCastOp*>(node))
    {
        visit_expression(visitor, type_cast->get_value());
        visitor->accept_expression_node(type_cast);
    }
    else if (auto* indirect_call = cast_expr<AstIndirectCall*>(node))
    {
        for (const auto& arg : indirect_call->get_args())
        {
            visit_expression(visitor, arg.get());
        }
        visit_expression(visitor, indirect_call->get_callee());
        visitor->accept_expression_node(indirect_call);
    }

    // AstLiteral, AstIdentifier, AstVariadicArgReference,
    // AstArrayMemberAccessor (base/index already handled above) — leaf nodes, no children.

    visitor->accept_expression_node(node);
}

void AstNodeTraverser::visit(IVisitor* visitor, IAstNode* node)
{
    if (!node)
        return;

    // Mainly statement parsing here

    if (auto* conditional = dynamic_cast<AstConditionalStatement*>(node))
    {
        visit_expression(visitor, conditional->get_condition());
        visit_block(visitor, conditional->get_body());
        if (conditional->get_else_body())
            visit_block(visitor, conditional->get_else_body());
    }
    else if (auto* while_loop = dynamic_cast<AstWhileLoop*>(node))
    {
        if (while_loop->get_condition())
            visit_expression(visitor, while_loop->get_condition());
        visit_block(visitor, while_loop->get_body());
    }
    else if (const auto* for_loop = dynamic_cast<AstForLoop*>(node))
    {
        if (for_loop->get_initializer())
            visit_expression(visitor, for_loop->get_initializer());
        if (for_loop->get_condition())
            visit_expression(visitor, for_loop->get_condition());
        if (for_loop->get_incrementor())
            visit_expression(visitor, for_loop->get_incrementor());
    }
    else if (const auto* return_stmt = dynamic_cast<AstReturnStatement*>(node))
    {
        if (return_stmt->get_return_expression().has_value())
            visit_expression(visitor, return_stmt->get_return_expression().value().get());
    }
    else if (auto* module = dynamic_cast<AstModule*>(node))
    {
        visit_block(visitor, module->get_body());
    }
    else if (const auto* block = dynamic_cast<AstBlock*>(node))
    {
        visit_block(visitor, block);
    }
    else if (auto* expr = dynamic_cast<IAstExpression*>(node))
    {
        visit_expression(visitor, expr);
    }
    else if (auto* variable_declaration = dynamic_cast<AstVariableDeclaration*>(node))
    {
        visit_expression(visitor, variable_declaration->get_initial_value());
        visitor->accept_expression_node(variable_declaration);
    }
    else if (auto* import_node = dynamic_cast<AstImport*>(node))
    {
        visitor->accept_import_node(import_node);
    }
    else if (auto* package_node = dynamic_cast<AstPackage*>(node))
    {
        visitor->accept_package_node(package_node);
    }
}
