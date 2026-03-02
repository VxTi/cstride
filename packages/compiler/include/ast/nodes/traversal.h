#pragma once

namespace stride::ast
{
    class IAstNode;
    class IAstExpression;
    class AstBlock;
    class IAstFunction;
    class AstConditionalStatement;
    class AstWhileLoop;
    class AstForLoop;
    class AstReturnStatement;

    /// Visitor interface for expression nodes.
    /// Implementations receive each expression after all its child expressions
    /// have already been visited (bottom-up, post-order traversal).
    class IVisitor
    {
    public:
        virtual ~IVisitor() = default;

        /// Called for every expression node, after its sub-expressions have been visited.
        virtual void accept(IAstExpression* expr) = 0;
    };

    /// Traverses an AST tree and invokes an IVisitor for each expression node.
    /// Traversal is bottom-up (children are visited before their parent expression),
    /// ensuring that child expression types are available when the parent is visited.
    class AstNodeTraverser
    {
        void visit(IVisitor* visitor, AstBlock* node);
        void visit(IVisitor* visitor, IAstExpression* node);
        void visit(IVisitor* visitor, IAstFunction* node);
        void visit(IVisitor* visitor, AstConditionalStatement* node);
        void visit(IVisitor* visitor, AstWhileLoop* node);
        void visit(IVisitor* visitor, AstForLoop* node);
        void visit(IVisitor* visitor, AstReturnStatement* node);

    public:
        /// Entry point: traverses the subtree rooted at `node`, calling the visitor
        /// for every expression encountered.
        void visit(IVisitor* visitor, IAstNode* node);
    };
}
