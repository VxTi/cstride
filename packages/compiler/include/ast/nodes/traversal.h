#pragma once

namespace stride::ast
{
    class AstPackage;
    class AstImport;
    class AstVariableDeclaration;
    class AstLambdaFunctionExpression;
    class AstModule;
    class AstFunctionDeclaration;
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
        virtual void accept(IAstExpression* expr) {};

        virtual void accept(IAstFunction* expr) {};

        virtual void accept(AstImport *node) {}

        virtual void accept(AstPackage *node) {}
    };

    /// Traverses an AST tree and invokes an IVisitor for each expression node.
    /// Traversal is bottom-up (children are visited before their parent expression),
    /// ensuring that child expression types are available when the parent is visited.
    class AstNodeTraverser
    {
    public:
        void visit_variable_declaration(IVisitor* visitor, AstVariableDeclaration* node);
        void visit(IVisitor* visitor, IAstNode* node);
        void visit_for_loop(IVisitor* visitor, AstForLoop* node);
        void visit_while_loop(IVisitor* visitor, AstWhileLoop* node);
        void visit(IVisitor* visitor, IAstExpression* node);
        void visit(IVisitor* visitor, AstConditionalStatement* node);
        void visit_return_statement(IVisitor* visitor, const AstReturnStatement* node);
        void visit(IVisitor* visitor, const AstBlock* node);
    };
}
