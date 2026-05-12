#include "ast/nodes/control_flow_statements.h"

namespace stride::ast
{
    std::unique_ptr<IAstNode> IAstControlFlowStatement::clone()
    {
        throw std::runtime_error("clone() not implemented for IAstControlFlowStatement");
    }
}
