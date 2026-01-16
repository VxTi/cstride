#pragma once

#include "ast/scope.h"
#include "ast/nodes/ast_node.h"
#include "ast/tokens/token_set.h"

namespace stride::ast
{
    class AstBlockNode : public AstNode
    {
        std::vector<std::unique_ptr<AstNode>> children;

    public:
        explicit AstBlockNode(
            std::vector<std::unique_ptr<AstNode>> children
        ) : children(std::move(children))
        {
        };

        llvm::Value* codegen() override;

        std::string to_string() override;

        static std::optional<TokenSet> collect_block(TokenSet &set);

        static std::unique_ptr<AstNode> try_parse(const Scope& scope, TokenSet& set);

        static std::unique_ptr<AstNode> try_parse_block(const Scope& scope, TokenSet& set);
    };

}
