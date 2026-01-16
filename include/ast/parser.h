#pragma once

#include "ast/nodes/enumerables.h"
#include "ast/nodes/expression.h"
#include "ast/nodes/function_declaration.h"
#include "ast/nodes/import.h"
#include "ast/nodes/module.h"
#include "ast/nodes/return.h"
#include "ast/nodes/struct.h"
#include "nodes/ast_node.h"
#include "nodes/blocks.h"
#include "tokens/token.h"

namespace stride::ast
{
    namespace parser
    {
        [[nodiscard]] std::unique_ptr<IAstNode> parse(const std::string& source_path);
    }

    std::unique_ptr<IAstNode> parse_next_statement(const Scope& scope, TokenSet& tokens);

    std::unique_ptr<AstBlock> parse_sequential(const Scope& scope, TokenSet& tokens);
}
