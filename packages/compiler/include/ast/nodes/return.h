#pragma once

#include "ast_node.h"
#include "expression.h"

namespace stride::ast
{
    class AstReturn :
        public IAstNode,
        public ISynthesisable
    {
        const std::unique_ptr<AstExpression> _value;

    public:
        explicit AstReturn(
            const SourceLocation &source,
            const std::shared_ptr<ParsingContext>& context,
            std::unique_ptr<AstExpression> value
        )
            :
            IAstNode(source, context),
            _value(std::move(value)) {}

        std::string to_string() override;

        llvm::Value* codegen(
            const ParsingContext* context,
            llvm::Module* module,
            llvm::IRBuilder<>* builder
        ) override;


        [[nodiscard]]
        AstExpression* get_return_expr() const { return _value.get(); }

        void validate() override;
    };

    std::unique_ptr<AstReturn> parse_return_statement(const std::shared_ptr<ParsingContext>& context, TokenSet& set);
}
