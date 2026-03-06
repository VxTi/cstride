#include "ast/nodes/traversal.h"

#include "ast/parsing_context.h"
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
    if (auto* fn = dynamic_cast<IAstFunction*>(node))
    {
        visitor->accept(fn);
        visit_block(visitor, fn->get_body());
    }
    else if (const auto* binary = dynamic_cast<IBinaryOp*>(node))
    {
        visit_expression(visitor, binary->get_left());
        visit_expression(visitor, binary->get_right());
    }
    else if (const auto* unary = dynamic_cast<AstUnaryOp*>(node))
    {
        visit_expression(visitor, &unary->get_operand());
    }
    else if (const auto* var_decl = dynamic_cast<AstVariableDeclaration*>(node))
    {
        if (var_decl->get_initial_value())
            visit_expression(visitor, var_decl->get_initial_value());
    }
    else if (const auto* fn_call = dynamic_cast<AstFunctionCall*>(node))
    {
        for (const auto& arg : fn_call->get_arguments())
            visit_expression(visitor, arg.get());
    }
    else if (const auto* array = dynamic_cast<AstArray*>(node))
    {
        for (const auto& elem : array->get_elements())
            visit_expression(visitor, elem.get());
    }
    else if (const auto* array_accessor = dynamic_cast<AstArrayMemberAccessor*>(node))
    {
        visit_expression(visitor, array_accessor->get_array_identifier());
        visit_expression(visitor, array_accessor->get_index());
    }
    else if (const auto* struct_init = dynamic_cast<AstStructInitializer*>(node))
    {
        for (const auto& val : struct_init->get_initializers() | std::views::values)
            visit_expression(visitor, val.get());
    }
    else if (const auto* tuple_init = dynamic_cast<AstTupleInitializer*>(node))
    {
        for (const auto& member : tuple_init->get_members())
            visit_expression(visitor, member.get());
    }
    else if (const auto* reassign = dynamic_cast<AstVariableReassignment*>(node))
    {
        visit_expression(visitor, reassign->get_value());
    }
    else if (const auto* member_access = dynamic_cast<AstMemberAccessor*>(node))
    {
        // Visit the base identifier so its type is resolved before the accessor's type is inferred.
        visit_expression(visitor, member_access->get_base());
    }
    else if (auto* function_node = dynamic_cast<IAstFunction*>(node))
    {
        visitor->accept(node);
        visit_block(visitor, function_node->get_body());
    }

    // AstLiteral, AstIdentifier, AstVariadicArgReference,
    // AstArrayMemberAccessor (base/index already handled above) — leaf nodes, no children.

    visitor->accept(node);
}

void AstNodeTraverser::visit_conditional_statement(IVisitor* visitor, AstConditionalStatement* node)
{
    visit_expression(visitor, node->get_condition());
    visit_block(visitor, node->get_body());
    if (node->get_else_body())
        visit_block(visitor, node->get_else_body());
}

void AstNodeTraverser::visit_while_loop(IVisitor* visitor, AstWhileLoop* node)
{
    if (node->get_condition())
        visit_expression(visitor, node->get_condition());
    visit_block(visitor, node->get_body());
}

void AstNodeTraverser::visit_for_loop(IVisitor* visitor, AstForLoop* node)
{
    if (node->get_initializer())
        visit_expression(visitor, node->get_initializer());
    if (node->get_condition())
        visit_expression(visitor, node->get_condition());
    if (node->get_incrementor())
        visit_expression(visitor, node->get_incrementor());
    visit_block(visitor, node->get_body());
}

void AstNodeTraverser::visit_return_statement(IVisitor* visitor, const AstReturnStatement* node)
{
    if (node->get_return_expression().has_value())
        visit_expression(visitor, node->get_return_expression().value().get());
}

void AstNodeTraverser::visit_variable_declaration(IVisitor* visitor, AstVariableDeclaration* node)
{
    visitor->accept(node);
    visit_expression(visitor, node->get_initial_value());
}

void AstNodeTraverser::visit(IVisitor* visitor, IAstNode* node)
{
    if (!node)
        return;


    if (auto* conditional = dynamic_cast<AstConditionalStatement*>(node))
    {
        visit_conditional_statement(visitor, conditional);
    }
    else if (auto* while_loop = dynamic_cast<AstWhileLoop*>(node))
    {
        visit_while_loop(visitor, while_loop);
    }
    else if (auto* for_loop = dynamic_cast<AstForLoop*>(node))
    {
        visit_for_loop(visitor, for_loop);
    }
    else if (const auto* return_stmt = dynamic_cast<AstReturnStatement*>(node))
    {
        visit_return_statement(visitor, return_stmt);
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
        visit_variable_declaration(visitor, variable_declaration);
    }
    else if (auto* import_node = dynamic_cast<AstImport*>(node))
    {
        visitor->accept(import_node);
    }
    else if (auto* package_node = dynamic_cast<AstPackage*>(node))
    {
        visitor->accept(package_node);
    }
}
