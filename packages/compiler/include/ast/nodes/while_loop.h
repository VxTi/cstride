#pragma once

#include "blocks.h"
#include "expression.h"

namespace stride::ast
{
    class IAstNode;
    class SymbolTable;
    class TokenSet;
    enum class VisibilityModifier;

    class AstWhileLoop
        : public IAstNode,
          public IAstContainer
    {
        std::unique_ptr<AstBlock> _body;
        std::unique_ptr<IAstExpression> _condition;

    public:
        explicit AstWhileLoop(
            const SourcePosition& source,
            std::unique_ptr<IAstExpression> condition,
            std::unique_ptr<AstBlock> body
        ) :
            IAstNode(source),
            _body(std::move(body)),
            _condition(std::move(condition)) {}

        llvm::Value* codegen(
            SymbolTable* symbol_table,
            llvm::Module* module, llvm::IRBuilderBase* builder) override;

        std::string to_string() override;

        [[nodiscard]]
        AstBlock* get_body() override
        {
            return _body.get();
        }

        [[nodiscard]]
        IAstExpression* get_condition() const
        {
            return _condition.get();
        }

        std::unique_ptr<IAstNode> clone() override;
    };

    std::unique_ptr<AstWhileLoop> parse_while_loop_statement(
        TokenSet& set,
        VisibilityModifier modifier);
} // namespace stride::ast
