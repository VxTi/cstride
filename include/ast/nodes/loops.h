#pragma once
#include "ast_node.h"
#include "blocks.h"
#include "expression.h"
#include "ast/scope.h"
#include "ast/tokens/token_set.h"

namespace stride::ast
{
    class AstLoop
        : public IAstNode,
          public ISynthesisable
    {
        std::unique_ptr<AstBlock> _body;
        std::unique_ptr<AstExpression> _initiator;
        std::unique_ptr<AstExpression> _condition;
        std::unique_ptr<AstExpression> _increment;

    public:
        AstLoop(
            std::unique_ptr<AstExpression> initiator,
            std::unique_ptr<AstExpression> condition,
            std::unique_ptr<AstExpression> increment,
            std::unique_ptr<AstBlock> body
        ) :
            _body(std::move(body)),
            _initiator(std::move(initiator)),
            _condition(std::move(condition)),
            _increment(std::move(increment)) {}

        llvm::Value* codegen(llvm::Module* module, llvm::LLVMContext& context, llvm::IRBuilder<>* builder) override;

        std::string to_string() override;

        [[nodiscard]]
        AstExpression* initiator() const { return _initiator.get(); }

        [[nodiscard]]
        AstBlock* body() const { return _body.get(); }

        [[nodiscard]]
        AstExpression* condition() const { return _condition.get(); }

        [[nodiscard]]
        AstExpression* increment() const { return _increment.get(); }
    };

    bool is_for_loop_statement(const TokenSet& tokens);

    bool is_while_loop_statement(const TokenSet& tokens);

    std::unique_ptr<AstLoop> parse_for_loop_statement(const Scope& scope, TokenSet& tokens);

    std::unique_ptr<AstLoop> parse_while_loop_statement(const Scope& scope, TokenSet& tokens);
}
