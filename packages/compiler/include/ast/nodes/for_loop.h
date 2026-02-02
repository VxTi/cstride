#pragma once

#include "ast_node.h"
#include "blocks.h"
#include "expression.h"
#include "ast/symbol_registry.h"
#include "ast/tokens/token_set.h"

namespace stride::ast
{
    class AstForLoop
        : public IAstNode,
          public ISynthesisable,
          public IAstContainer
    {
        std::unique_ptr<AstBlock> _body;
        std::unique_ptr<AstExpression> _initializer;
        std::unique_ptr<AstExpression> _condition;
        std::unique_ptr<AstExpression> _incrementor;

    public:
        explicit AstForLoop(
            const std::shared_ptr<SourceFile>& source,
            const SourcePosition source_position,
            const std::shared_ptr<SymbolRegistry>& registry,
            std::unique_ptr<AstExpression> initiator,
            std::unique_ptr<AstExpression> condition,
            std::unique_ptr<AstExpression> increment,
            std::unique_ptr<AstBlock> body
        ) : IAstNode(source, source_position, registry),
            _body(std::move(body)),
            _initializer(std::move(initiator)),
            _condition(std::move(condition)),
            _incrementor(std::move(increment)) {}

        llvm::Value* codegen(
            const std::shared_ptr<SymbolRegistry>& registry,
            llvm::Module* module,
            llvm::IRBuilder<>* builder
        ) override;

        std::string to_string() override;

        [[nodiscard]]
        AstExpression* get_initializer() const { return _initializer.get(); }

        [[nodiscard]]
        AstBlock* get_body() override { return _body.get(); }

        [[nodiscard]]
        AstExpression* get_condition() const { return _condition.get(); }

        [[nodiscard]]
        AstExpression* get_incrementor() const { return _incrementor.get(); }

        void validate() override;
    };

    bool is_for_loop_statement(const TokenSet& set);

    std::unique_ptr<AstForLoop>
    parse_for_loop_statement(const std::shared_ptr<SymbolRegistry>& registry, TokenSet& set);
}
