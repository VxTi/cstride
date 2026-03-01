#pragma once

#include "ast/nodes/ast_node.h"

#include <optional>
#include <vector>

namespace stride::ast
{
    enum class TokenType;
    class TokenSet;

    class AstBlock
        : public IAstNode
    {
        std::vector<std::unique_ptr<IAstNode>> _children;

    public:
        explicit AstBlock(
            const SourceFragment& source,
            const std::shared_ptr<ParsingContext>& context,
            std::vector<std::unique_ptr<IAstNode>> children
        ) :
            IAstNode(source, context),
            _children(std::move(children)) {}

        std::string to_string() override;

        void validate() override;

        llvm::Value* codegen(
            llvm::Module* module,
            llvm::IRBuilderBase* builder
        ) override;

        void resolve_forward_references(
            const ParsingContext* context,
            llvm::Module* module,
            llvm::IRBuilderBase* builder
        ) override;

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

        static std::unique_ptr<AstBlock> create_empty(
            const std::shared_ptr<ParsingContext>& context,
            const SourceFragment& source
        )
        {
            return std::make_unique<AstBlock>(source, context, std::vector<std::unique_ptr<IAstNode>>{});
        }
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

    std::optional<TokenSet> collect_parenthesized_block(TokenSet& set);
} // namespace stride::ast
