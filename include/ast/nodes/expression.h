#pragma once

#include "ast_node.h"
#include "ast/scope.h"
#include "ast/tokens/token_set.h"

namespace stride::ast
{
    class AstExpression : public AstNode
    {
    public:
        const std::vector<std::unique_ptr<AstNode>> children;

        explicit AstExpression(std::vector<std::unique_ptr<AstNode>> children) : children(std::move(children))
        {
        };

        llvm::Value* codegen() override;

        std::string to_string() override;

        static bool can_parse(const TokenSet& tokens);

        static std::unique_ptr<AstExpression> try_parse(const Scope& scope, TokenSet& tokens);
    };
}
