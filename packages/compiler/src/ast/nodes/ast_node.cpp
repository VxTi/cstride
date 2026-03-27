#include "ast/nodes/ast_node.h"

namespace stride::ast
{
    std::unique_ptr<IAstNode> IAstNode::clone()
    {
        throw std::runtime_error("clone() not implemented for IAstNode");
    }
}
