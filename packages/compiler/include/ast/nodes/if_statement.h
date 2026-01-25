#pragma once
#include "ast_node.h"
#include "blocks.h"
#include "expression.h"

namespace stride::ast
{
    class AstIfStatement
        : public IAstNode,
          public ISynthesisable,
          public IReducible
    {
        std::unique_ptr<AstExpression> _condition;
        std::unique_ptr<AstBlock> _block;
        std::unique_ptr<AstBlock> _else_block;

    public:
        AstIfStatement(
            const std::shared_ptr<SourceFile>& source,
            const int source_offset,
            const std::shared_ptr<Scope>& scope,
            std::unique_ptr<AstExpression> condition,
            std::unique_ptr<AstBlock> block,
            std::unique_ptr<AstBlock> else_block
        ) : IAstNode(source, source_offset, scope),
            _condition(std::move(condition)),
            _block(std::move(block)),
            _else_block(std::move(else_block)) {}


        [[nodiscard]]
        AstExpression* get_condition() const { return this->_condition.get(); }

        [[nodiscard]]
        AstBlock* get_block() const { return this->_block.get(); }

        [[nodiscard]]
        AstBlock* get_else_block() const { return this->_else_block.get(); }

        ~AstIfStatement() override = default;

        bool is_reducible() override;

        IAstNode* reduce() override;

        void validate() override;

        llvm::Value* codegen(const std::shared_ptr<Scope>& scope, llvm::Module* module, llvm::LLVMContext& context,
                             llvm::IRBuilder<>* builder) override;

        std::string to_string() override;
    };

    bool is_if_statement(const TokenSet& tokens);

    std::unique_ptr<AstIfStatement> parse_if_statement(const std::shared_ptr<Scope>& scope, TokenSet& set);
}
