#pragma once

#include "ast/modifiers.h"
#include "ast/parsing_context.h"
#include "ast/tokens/token_set.h"
#include "ast_node.h"
#include "blocks.h"
#include "expression.h"

namespace stride::ast
{
    class AstWhileLoop : public IAstNode, public ISynthesisable,
                         public IAstContainer
    {
        std::unique_ptr<AstBlock> _body;
        std::unique_ptr<AstExpression> _condition;

    public:
        AstWhileLoop(
            const SourceFragment& source,
            const std::shared_ptr<ParsingContext>& context,
            std::unique_ptr<AstExpression> condition,
            std::unique_ptr<AstBlock> body) :
            IAstNode(source, context),
            _body(std::move(body)),
            _condition(std::move(condition)) {}

        llvm::Value* codegen(
            llvm::Module* module,
            llvm::IRBuilder<>* builder) override;

        std::string to_string() override;

        [[nodiscard]]
        AstBlock* get_body() override
        {
            return _body.get();
        }

        [[nodiscard]]
        AstExpression* get_condition() const
        {
            return _condition.get();
        }

        void validate() override;
    };

    std::unique_ptr<AstWhileLoop> parse_while_loop_statement(
        const std::shared_ptr<ParsingContext>& context,
        TokenSet& set,
        VisibilityModifier modifier);
} // namespace stride::ast
