#include "nodes/traversal.h"

#include "nodes/blocks.h"
#include "nodes/expression.h"
#include "nodes/function_declaration.h"
#include "nodes/module.h"

using namespace stride::ast;

void visit(IVisitor* visitor, AstBlock* node)
{
    for (const auto& child : node->get_children())
    {
        const auto child_node = child.get();
        visit(visitor, child_node);
    }
}

void visit(IVisitor* visitor, IAstExpression* node)
{
    visitor->accept(node);
    if (const auto* expr = dynamic_cast<AstBinaryArithmeticOp*>(node))
    {
        visit(visitor, expr->get_left());
        visit(visitor, expr->get_right());
    }
}

void visit(IVisitor* visitor, AstModule* node)
{
    visit(visitor, node->get_body());
}

void visit(IVisitor* visitor, AstFunctionDeclaration* node)
{
    visit(visitor, node->get_body());
}

void visit(IVisitor* visitor, IAstNode* node)
{
    visitor->accept(node);

    if (auto* block = dynamic_cast<AstBlock*>(node))
    {
        visit(visitor, block);
    }
    else if (auto* module = dynamic_cast<AstModule*>(node))
    {
        visit(visitor, module);
    }
    else if (auto* fn_decl = dynamic_cast<AstFunctionDeclaration*>(node))
    {
        visit(visitor, fn_decl);
    } else if (auto* expr = dynamic_cast<IAstExpression*>(node))
    {
        visit(visitor, expr);
    }


}
