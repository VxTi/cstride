#pragma once

#include "errors.h"
#include "ast/symbol_table.h"
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
        std::shared_ptr<SymbolTable> _symbol_table;

    public:
        explicit AstBlock(
            const SourcePosition& source,
            std::vector<std::unique_ptr<IAstNode>> children
        ) :
            IAstNode(source),
            _children(std::move(children)) {}

        std::string to_string() override;

        llvm::Value* codegen(
            SymbolTable* symbol_table,
            llvm::Module* module,
            llvm::IRBuilderBase* builder
        ) override;

        void resolve_forward_references(
            SymbolTable* symbol_table,
            llvm::Module* module,
            llvm::IRBuilderBase* builder
        ) override;

        void aggregate_block(AstBlock* other);

        [[nodiscard]]
        std::shared_ptr<SymbolTable> get_symbol_table() const
        {
            return this->_symbol_table;
        }

        void set_symbol_table(std::shared_ptr<SymbolTable> symbol_table)
        {
            this->_symbol_table = std::move(symbol_table);
        }

        [[nodiscard]]
        const std::vector<std::unique_ptr<IAstNode>>& get_children() const
        {
            return this->_children;
        }

        ~AstBlock() override = default;

        static std::unique_ptr<AstBlock> create_empty(
            const SourcePosition& source
        )
        {
            return std::make_unique<AstBlock>(
                source,
                std::vector<std::unique_ptr<IAstNode>>{}
            );
        }

        std::unique_ptr<IAstNode> clone() override;
    };

    std::unique_ptr<AstBlock> parse_block(TokenSet& set);

    std::optional<TokenSet> collect_block(TokenSet& set);

    TokenSet collect_block_required(TokenSet& set, const std::string& error);

    std::optional<TokenSet> collect_block_variant(
        TokenSet& set,
        TokenType start_token,
        TokenType end_token);

    std::optional<TokenSet> collect_until_token(TokenSet& set, TokenType token);

    std::optional<TokenSet> collect_parenthesized_block(TokenSet& set);
} // namespace stride::ast
