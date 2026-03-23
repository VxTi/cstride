#pragma once

#include "blocks.h"
#include "expression.h"

namespace stride::ast
{
    class AstConditionalStatement
        : public IAstStatement,
          public IReducible,
          public IAstNode,
          public IAstContainer
    {
        std::unique_ptr<IAstExpression> _condition;
        std::unique_ptr<AstBlock> _body;
        std::unique_ptr<AstBlock> _else_body;

    public:
        explicit AstConditionalStatement(
            const SourcePosition& source,
            std::unique_ptr<IAstExpression> condition,
            std::unique_ptr<AstBlock> body,
            std::unique_ptr<AstBlock> else_body
        ) :
            IAstNode(source),
            _condition(std::move(condition)),
            _body(std::move(body)),
            _else_body(std::move(else_body)) {}

        [[nodiscard]]
        IAstExpression* get_condition() const
        {
            return this->_condition.get();
        }

        [[nodiscard]]
        AstBlock* get_body() override
        {
            return this->_body.get();
        }

        [[nodiscard]]
        AstBlock* get_else_body() const
        {
            return this->_else_body.get();
        }

        ~AstConditionalStatement() override = default;

        bool is_reducible() override;

        std::optional<std::unique_ptr<IAstNode>> reduce() override;

        llvm::Value* codegen(SymbolTable* symbol_table, llvm::Module* module, llvm::IRBuilderBase* builder) override;

        std::unique_ptr<IAstNode> clone() override;

        std::string to_string() override;
    };

    std::unique_ptr<AstConditionalStatement> parse_if_statement(TokenSet& set);
} // namespace stride::ast
