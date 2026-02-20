#pragma once

#include "ast_node.h"
#include "expression.h"

namespace stride::ast
{
    class AstReturnStatement : public IAstNode, public ISynthesisable
    {
        const std::unique_ptr<AstExpression> _value;

    public:
        explicit AstReturnStatement(
            const SourceFragment& source,
            const std::shared_ptr<ParsingContext>& context,
            std::unique_ptr<AstExpression> value) :
            IAstNode(source, context),
            _value(std::move(value)) {}

        std::string to_string() override;

        llvm::Value* codegen(
            llvm::Module* module,
            llvm::IRBuilder<>* builder) override;


        [[nodiscard]]
        AstExpression* get_return_expr() const
        {
            return _value.get();
        }

        void validate() override;
    };

    std::unique_ptr<AstReturnStatement> parse_return_statement(
        const std::shared_ptr<ParsingContext>& context,
        TokenSet& set);
} // namespace stride::ast
