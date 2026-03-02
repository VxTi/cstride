#include "ast/nodes/traversal.h"

#include "ast/parsing_context.h"
#include "ast/nodes/ast_node.h"
#include "ast/nodes/blocks.h"
#include "ast/nodes/conditional_statement.h"
#include "ast/nodes/expression.h"
#include "ast/nodes/for_loop.h"
#include "ast/nodes/function_declaration.h"
#include "ast/nodes/module.h"
#include "ast/nodes/return_statement.h"
#include "ast/nodes/while_loop.h"

#include <ranges>

using namespace stride::ast;

void AstNodeTraverser::visit(IVisitor* visitor, const AstBlock* node)
{
    if (!node)
        return;

    for (const auto& child : node->get_children())
    {
        visit(visitor, child.get());
    }
}

void AstNodeTraverser::visit(IVisitor* visitor, AstModule* node)
{
    visit(visitor, node->get_body());
}

void AstNodeTraverser::visit(IVisitor* visitor, IAstExpression* node)
{
    if (!node)
        return;

    // IAstFunction is an expression but needs special handling (body traversal + params)
    if (auto* fn = dynamic_cast<IAstFunction*>(node))
    {
        visit(visitor, fn);
        return;
    }

    // Recurse into child expressions first (bottom-up post-order)
    if (const auto* binary = dynamic_cast<IBinaryOp*>(node))
    {
        visit(visitor, binary->get_left());
        visit(visitor, binary->get_right());
    }
    else if (const auto* unary = dynamic_cast<AstUnaryOp*>(node))
    {
        visit(visitor, &unary->get_operand());
    }
    else if (const auto* var_decl = dynamic_cast<AstVariableDeclaration*>(node))
    {
        if (var_decl->get_initial_value())
            visit(visitor, var_decl->get_initial_value().get());
    }
    else if (const auto* fn_call = dynamic_cast<AstFunctionCall*>(node))
    {
        for (const auto& arg : fn_call->get_arguments())
            visit(visitor, arg.get());
    }
    else if (const auto* array = dynamic_cast<AstArray*>(node))
    {
        for (const auto& elem : array->get_elements())
            visit(visitor, elem.get());
    }
    else if (const auto* array_accessor = dynamic_cast<AstArrayMemberAccessor*>(node))
    {
        visit(visitor, array_accessor->get_array_identifier());
        visit(visitor, array_accessor->get_index());
    }
    else if (const auto* struct_init = dynamic_cast<AstStructInitializer*>(node))
    {
        for (const auto& val : struct_init->get_initializers() | std::views::values)
            visit(visitor, val.get());
    }
    else if (const auto* tuple_init = dynamic_cast<AstTupleInitializer*>(node))
    {
        for (const auto& member : tuple_init->get_members())
            visit(visitor, member.get());
    }
    else if (const auto* reassign = dynamic_cast<AstVariableReassignment*>(node))
    {
        visit(visitor, reassign->get_value());
    }
    else if (const auto* member_access = dynamic_cast<AstMemberAccessor*>(node))
    {
        // Visit the base identifier so its type is resolved before the accessor's type is inferred.
        visit(visitor, member_access->get_base());
    }
    // AstLiteral, AstIdentifier, AstVariadicArgReference,
    // AstArrayMemberAccessor (base/index already handled above) — leaf nodes, no children.

    visitor->accept(node);
}

void AstNodeTraverser::visit(IVisitor* visitor, AstConditionalStatement* node)
{
    visit(visitor, node->get_condition());
    visit(visitor, node->get_body());
    if (node->get_else_body())
        visit(visitor, node->get_else_body());
}

void AstNodeTraverser::visit(IVisitor* visitor, AstWhileLoop* node)
{
    if (node->get_condition())
        visit(visitor, node->get_condition());
    visit(visitor, node->get_body());
}

void AstNodeTraverser::visit(IVisitor* visitor, AstForLoop* node)
{
    if (node->get_initializer())
        visit(visitor, node->get_initializer());
    if (node->get_condition())
        visit(visitor, node->get_condition());
    if (node->get_incrementor())
        visit(visitor, node->get_incrementor());
    visit(visitor, node->get_body());
}

void AstNodeTraverser::visit(IVisitor* visitor, const AstReturnStatement* node)
{
    if (node->get_return_expr())
        visit(visitor, node->get_return_expr());
}

void AstNodeTraverser::visit(IVisitor* visitor, AstFunctionDeclaration* node)
{
    visitor->accept(node);
    visit(visitor, node->get_body());
}

void AstNodeTraverser::visit(IVisitor* visitor, IAstNode* node)
{
    if (!node)
        return;

    if (auto* fn_decl = dynamic_cast<AstFunctionDeclaration*>(node))
    {
        visit(visitor, fn_decl);
    }
    else if (auto* conditional = dynamic_cast<AstConditionalStatement*>(node))
    {
        visit(visitor, conditional);
    }
    else if (auto* while_loop = dynamic_cast<AstWhileLoop*>(node))
    {
        visit(visitor, while_loop);
    }
    else if (auto* for_loop = dynamic_cast<AstForLoop*>(node))
    {
        visit(visitor, for_loop);
    }
    else if (const auto* return_stmt = dynamic_cast<AstReturnStatement*>(node))
    {
        visit(visitor, return_stmt);
    }
    else if (auto* module = dynamic_cast<AstModule*>(node))
    {
        visit(visitor, module);
    }
    else if (const auto* block = dynamic_cast<AstBlock*>(node))
    {
        visit(visitor, block);
    }
    else if (auto* expr = dynamic_cast<IAstExpression*>(node))
    {
        visit(visitor, expr);
    }
}
