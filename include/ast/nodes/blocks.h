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
        explicit AstBlock(
            const std::shared_ptr<SourceFile>& source,
            const int source_offset,
            std::vector<std::unique_ptr<IAstNode>> children
        )
            : IAstNode(source, source_offset), _children(std::move(children)) {};

        std::string to_string() override;

        llvm::Value* codegen(llvm::Module* module, llvm::LLVMContext& context, llvm::IRBuilder<>* builder) override;

        void define_symbols(llvm::Module* module, llvm::LLVMContext& context, llvm::IRBuilder<>* builder) override;

        [[nodiscard]]
        const std::vector<std::unique_ptr<IAstNode>>& children() const { return this->_children; }
    };

    std::unique_ptr<AstBlock> parse_block(std::shared_ptr<Scope> scope, TokenSet& set);

    std::unique_ptr<AstBlock> parse_sequential(std::shared_ptr<Scope> scope, TokenSet& set);

    std::optional<TokenSet> collect_block(TokenSet& set);

    std::optional<TokenSet> collect_block_variant(TokenSet& set, TokenType start_token, TokenType end_token);

    std::optional<TokenSet> collect_until_token(TokenSet& set, TokenType token);

    inline std::optional<TokenSet> collect_parenthesized_block(TokenSet& set)
    {
        return collect_block_variant(set, TokenType::LPAREN, TokenType::RPAREN);
    }
}
