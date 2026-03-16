#pragma once

namespace stride::ast
{
    class IVisitor;
    class AstFunctionCall;
    class AstPackage;
    class AstImport;
    class IAstNode;
    class AstVariableDeclaration;
    class AstForLoop;
    class AstWhileLoop;
    class IAstFunction;
    class IAstExpression;
    class AstConditionalStatement;
    class AstReturnStatement;
    class AstBlock;

    /// Traverses an AST tree and invokes an IVisitor for each expression node.
    /// Traversal is bottom-up (children are visited before their parent expression),
    /// ensuring that child expression types are available when the parent is visited.
    class AstNodeTraverser
    {
    public:
        void visit(IVisitor* visitor, IAstNode* node);

        void visit_variable_declaration(IVisitor* visitor, AstVariableDeclaration* node);

        void visit_for_loop(IVisitor* visitor, AstForLoop* node);

        void visit_while_loop(IVisitor* visitor, AstWhileLoop* node);

        void visit_expression(IVisitor* visitor, IAstExpression* node);

        void visit_conditional_statement(IVisitor* visitor, AstConditionalStatement* node);

        void visit_return_statement(IVisitor* visitor, const AstReturnStatement* node);

        void visit_block(IVisitor* visitor, const AstBlock* node);
    };
}
