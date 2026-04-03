#pragma once
#include "symbol_table.h"

#include <memory>
#include <vector>

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
        std::shared_ptr<SymbolTable> _current_symbol_table;

        std::string _context_name;
        ContextType _current_context_type;

        std::vector<std::shared_ptr<SymbolTable>> _symbol_table_stack;

    public:
        explicit AstNodeTraverser() :
            _current_context_type(ContextType::GLOBAL) {}

        void traverse(IVisitor* visitor, const AstBranch* branch);

    private:
        void visit(IVisitor* visitor, IAstNode* node);

        void visit_expression(IVisitor* visitor, IAstExpression* node);

        void visit_block(IVisitor* visitor, const AstBlock* node);

        void push_symbol_table(AstBlock* block);

        void pop_symbol_table();
    };
}
