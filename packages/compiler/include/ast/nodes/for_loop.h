#pragma once

#include "blocks.h"
#include "expression.h"

namespace stride::ast
{
    class AstForLoop
        : public IAstNode,
          public IAstContainer
    {
        std::unique_ptr<AstBlock> _body;
        std::unique_ptr<IAstExpression> _initializer;
        std::unique_ptr<IAstExpression> _condition;
        std::unique_ptr<IAstExpression> _incrementor;

    public:
        explicit AstForLoop(
            const SourcePosition& source,
            std::unique_ptr<IAstExpression> initiator,
            std::unique_ptr<IAstExpression> condition,
            std::unique_ptr<IAstExpression> increment,
            std::unique_ptr<AstBlock> body
        ) :
            IAstNode(source),
            _body(std::move(body)),
            _initializer(std::move(initiator)),
            _condition(std::move(condition)),
            _incrementor(std::move(increment)) {}

        llvm::Value* codegen(
            SymbolTable* symbol_table,
            llvm::Module* module, llvm::IRBuilderBase* builder) override;

        std::string to_string() override;

        [[nodiscard]]
        IAstExpression* get_initializer() const
        {
            return _initializer.get();
        }

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

        [[nodiscard]]
        IAstExpression* get_incrementor() const
        {
            return _incrementor.get();
        }

        void validate(const SymbolTable* symbol_table) override;

        std::unique_ptr<IAstNode> clone() override;
    };

    std::unique_ptr<AstForLoop> parse_for_loop_statement(
        TokenSet& set,
        VisibilityModifier modifier);
} // namespace stride::ast
