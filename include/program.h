#pragma once
#include "ast/nodes/ast_node.h"

namespace stride
{
    class ProgramObject
    {
        std::unique_ptr<ast::IAstNode> _root;

    public:
        explicit ProgramObject(std::unique_ptr<ast::IAstNode> root) :
            _root(std::move(root))
        {
        }

        [[nodiscard]] ast::IAstNode* root() const { return _root.get(); }
    };

    class Program
    {
        std::vector<ProgramObject> _nodes;

    public:
        explicit Program(std::vector<std::string> files);

        std::vector<ProgramObject>& nodes() { return _nodes; }

        void execute();
    };
}
