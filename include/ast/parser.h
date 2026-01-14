#pragma once

#include "tokens/token.h"
#include "tokens/token_set.h"
#include "nodes/ast_node.h"

namespace stride::ast::parser
{
    class Parser
    {
        size_t position;
        const std::string source_path;
        std::unique_ptr<TokenSet> tokens;

    public:
        explicit Parser(const std::string &source_path);

        [[nodiscard]] std::unique_ptr<AstNode> parse() const;
    };
}
