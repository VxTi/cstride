#pragma once

#include "ast/nodes/ast_node.h"
#include "ast/parsing_context.h"
#include "ast/tokens/token_set.h"

namespace stride::ast
{
    class AstBlock : public IAstNode, public ISynthesisable
    {
        std::vector<std::unique_ptr<IAstNode>> _children;

    public:
        explicit AstBlock(
            const SourceFragment& source,
            const std::shared_ptr<ParsingContext>& context,
            std::vector<std::unique_ptr<IAstNode>> children) :
            IAstNode(source, context),
            _children(std::move(children)) {};

        std::string to_string() override;

        void validate() override
        {
            for (const auto& child : this->_children)
            {
                child->validate();
            }
        }

        llvm::Value* codegen(
            const ParsingContext* context,
            llvm::Module* module,
            llvm::IRBuilder<>* builder) override;

        void resolve_forward_references(
            const ParsingContext* context,
            llvm::Module* module,
            llvm::IRBuilder<>* builder) override;

        void aggregate_block(AstBlock* other)
        {
            for (auto& child : other->_children)
            {
                this->_children.push_back(std::move(child));
            }
        }

        [[nodiscard]]
        const std::vector<std::unique_ptr<IAstNode>>& children() const
        {
            return this->_children;
        }

        ~AstBlock() override = default;
    };

    std::unique_ptr<AstBlock> parse_block(
        const std::shared_ptr<ParsingContext>& context,
        TokenSet& set);

    std::optional<TokenSet> collect_block(TokenSet& set);

    std::optional<TokenSet> collect_block_variant(
        TokenSet& set,
        TokenType start_token,
        TokenType end_token);

    std::optional<TokenSet> collect_until_token(TokenSet& set, TokenType token);

    inline std::optional<TokenSet> collect_parenthesized_block(TokenSet& set)
    {
        return collect_block_variant(set, TokenType::LPAREN, TokenType::RPAREN);
    }
} // namespace stride::ast
