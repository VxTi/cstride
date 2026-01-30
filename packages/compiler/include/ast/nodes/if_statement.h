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
        std::unique_ptr<AstBlock> _body;
        std::unique_ptr<AstBlock> _else_body;

    public:
        explicit AstIfStatement(
            const std::shared_ptr<SourceFile>& source,
            const SourcePosition source_position,
            const std::shared_ptr<SymbolRegistry>& scope,
            std::unique_ptr<AstExpression> condition,
            std::unique_ptr<AstBlock> body,
            std::unique_ptr<AstBlock> else_body
        ) : IAstNode(source, source_position, scope),
            _condition(std::move(condition)),
            _body(std::move(body)),
            _else_body(std::move(else_body)) {}


        [[nodiscard]]
        AstExpression* get_condition() const { return this->_condition.get(); }

        [[nodiscard]]
        AstBlock* get_body() const { return this->_body.get(); }

        [[nodiscard]]
        AstBlock* get_else_body() const { return this->_else_body.get(); }

        ~AstIfStatement() override = default;

        bool is_reducible() override;

        IAstNode* reduce() override;

        void validate() override;

        llvm::Value* codegen(const std::shared_ptr<SymbolRegistry>& scope, llvm::Module* module,
                             llvm::IRBuilder<>* builder) override;

        std::string to_string() override;
    };

    bool is_if_statement(const TokenSet& tokens);

    std::unique_ptr<AstIfStatement> parse_if_statement(const std::shared_ptr<SymbolRegistry>& scope, TokenSet& set);
}
