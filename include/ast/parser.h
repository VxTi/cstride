#pragma once

#include "ast/nodes/enumerables.h"
#include "ast/nodes/function_definition.h"
#include "ast/nodes/import.h"
#include "nodes/ast_node.h"
#include "nodes/blocks.h"

namespace stride::ast
{
    namespace parser
    {
        [[nodiscard]]
        std::unique_ptr<IAstNode> parse(const std::string& source_path);
    }

    std::unique_ptr<IAstNode> parse_next_statement(std::shared_ptr<Scope> scope, TokenSet& set);

    std::unique_ptr<AstBlock> parse_sequential(std::shared_ptr<Scope> scope, TokenSet& set);
}
