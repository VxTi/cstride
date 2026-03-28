#pragma once
#include "symbol_table.h"

#include <memory>

namespace stride::ast
{
    class AstBranch;
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
        std::shared_ptr<SymbolTable> _root_symbol_table;
        std::shared_ptr<SymbolTable> _current_symbol_table;

        std::string _context_name;
        ContextType _current_context_type;

    public:
        explicit AstNodeTraverser(
            std::shared_ptr<SymbolTable> root_symbol_table
        ) :
            _root_symbol_table(std::move(root_symbol_table)),
            _current_symbol_table(root_symbol_table),
            _current_context_type(ContextType::GLOBAL) {}

        void traverse(IVisitor* visitor, const AstBranch *branch);

    private:
        void visit(IVisitor* visitor, IAstNode* node);

        void visit_expression(IVisitor* visitor, IAstExpression* node);

        void visit_block(IVisitor* visitor, AstBlock* node);
    };
}
