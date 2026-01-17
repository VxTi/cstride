#pragma once

#include "ast/scope.h"
#include "ast/nodes/ast_node.h"
#include "ast/tokens/token_set.h"

namespace stride::ast
{
    class AstBlock :
        public virtual IAstNode,
        public virtual ISynthesisable
    {
        std::vector<std::unique_ptr<IAstNode>> _children;

    public:
        explicit AstBlock(std::vector<std::unique_ptr<IAstNode>> children)
            : _children(std::move(children)) {};

        std::string to_string() override;

        llvm::Value* codegen(llvm::Module* module, llvm::LLVMContext& context, llvm::IRBuilder<>* builder) override;

        [[nodiscard]] const std::vector<std::unique_ptr<IAstNode>>& children() const { return this->_children; }
    };

    std::unique_ptr<AstBlock> parse_block(const Scope& scope, TokenSet& set);

    std::unique_ptr<AstBlock> parse_sequential(const Scope& scope, TokenSet& set);

    std::optional<TokenSet> collect_block(TokenSet& set);

    std::optional<TokenSet> collect_token_subset(TokenSet& set, TokenType start_token, TokenType end_token);

    std::optional<TokenSet> collect_until_token(TokenSet& set, TokenType token);
}
