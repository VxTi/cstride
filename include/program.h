#pragma once
#include "ast/scope.h"
#include "ast/nodes/ast_node.h"

#define MAIN_FN_NAME ("main")

namespace stride
{
    class ProgramObject
    {
        std::unique_ptr<ast::IAstNode> _root;

    public:
        explicit ProgramObject(std::unique_ptr<ast::IAstNode> root) :
            _root(std::move(root)) {}

        explicit ProgramObject(ast::IAstNode* root) : _root(root) {}

        [[nodiscard]]
        ast::IAstNode* root() const { return _root.get(); }

        ~ProgramObject() = default;
    };

    class Program
    {
        std::shared_ptr<ast::Scope> _global_scope;
        std::vector<ProgramObject> _nodes;

        void print_ast_nodes() const;

    public:
        explicit Program(std::vector<std::string> files);

        ~Program() = default;

        std::vector<ProgramObject>& nodes() { return _nodes; }

        void execute(int argc,char* argv[]) const;

        [[nodiscard]]
        std::shared_ptr<ast::Scope> get_global_scope() const { return this->_global_scope; }
    };

}
