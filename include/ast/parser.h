#pragma once

#include "program.h"
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
        std::unique_ptr<AstBlock> parse_file(const Program &program, const std::string& source_path);
    }

    std::unique_ptr<IAstNode> parse_next_statement(const std::shared_ptr<Scope>& scope, TokenSet& set);

    std::unique_ptr<AstBlock> parse_sequential(const std::shared_ptr<Scope>& scope, TokenSet& set);
}
