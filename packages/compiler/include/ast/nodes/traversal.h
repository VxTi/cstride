#pragma once

namespace stride::ast
{
    class IAstNode;
    class AstBlock;
    class AstExpression;
    class AstModule;
    class AstFunctionDeclaration;
    class AstBinaryArithmeticOp;

    class IVisitor
    {
    public:
        virtual ~IVisitor() = default;
        virtual void accept(const IAstNode* node);

        virtual void accept(const AstExpression* node);
    };

    class AstNodeTraverser
    {
        void visit(IVisitor* visitor, AstBlock* node);
        void visit(IVisitor* visitor, AstExpression* node);
        void visit(IVisitor* visitor, AstModule* node);
        void visit(IVisitor* visitor, AstFunctionDeclaration* node);

    public:
        void visit(IVisitor* visitor, IAstNode* node);
    };
}
