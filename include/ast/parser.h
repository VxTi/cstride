#pragma once

#include "tokens/token.h"
#include "nodes/ast_node.h"

namespace stride::ast::parser
{
    [[nodiscard]] std::unique_ptr<IAstNode> parse(const std::string& source_path);
}
