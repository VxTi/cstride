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
        u_ptr<AstExpression> _condition;
        u_ptr<AstBlock> _block;
        u_ptr<AstBlock> _else_block;

    public:
        AstIfStatement(
            const s_ptr<SourceFile>& source,
            const int source_offset,
            u_ptr<AstExpression> condition,
            u_ptr<AstBlock> block,
            u_ptr<AstBlock> else_block
        )
            : IAstNode(source, source_offset),
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

        llvm::Value* codegen(llvm::Module* module, llvm::LLVMContext& context, llvm::IRBuilder<>* builder) override;

        std::string to_string() override;
    };

    bool is_if_statement(const TokenSet& tokens);

    u_ptr<AstIfStatement> parse_if_statement(const Scope& scope, TokenSet& set);
}
