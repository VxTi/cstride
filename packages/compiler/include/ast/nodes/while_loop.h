#pragma once

#include "ast_node.h"
#include "blocks.h"
#include "expression.h"
#include "ast/symbol_registry.h"
#include "ast/tokens/token_set.h"

namespace stride::ast
{
    class AstWhileLoop
        : public IAstNode,
          public ISynthesisable,
          public IAstContainer
    {
        std::unique_ptr<AstBlock> _body;
        std::unique_ptr<AstExpression> _condition;

    public:
        AstWhileLoop(
            const std::shared_ptr<SourceFile>& source,
            const SourcePosition source_position,
            const std::shared_ptr<SymbolRegistry>& registry,
            std::unique_ptr<AstExpression> condition,
            std::unique_ptr<AstBlock> body
        ) : IAstNode(source, source_position, registry),
            _body(std::move(body)),
            _condition(std::move(condition)) {}

        llvm::Value* codegen(
            const std::shared_ptr<SymbolRegistry>& registry,
            llvm::Module* module,
            llvm::IRBuilder<>* builder
        ) override;

        std::string to_string() override;

        [[nodiscard]]
        AstBlock* get_body() override { return _body.get(); }

        [[nodiscard]]
        AstExpression* get_condition() const { return _condition.get(); }

        void validate() override;
    };

    bool is_while_loop_statement(const TokenSet& set);

    std::unique_ptr<AstWhileLoop> parse_while_loop_statement(const std::shared_ptr<SymbolRegistry>& registry,
                                                             TokenSet& set);
}
