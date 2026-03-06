#pragma once
#include "ast_node.h"
#include "expression.h"

#include <memory>
#include <llvm/IR/IRBuilder.h>

namespace llvm
{
    class Module;
    class Value;
}

namespace stride
{
    struct SourceFragment;
}

namespace stride::ast
{
    class ParsingContext;

    class AstReturnStatement
        : public IAstNode
    {
        std::optional<std::unique_ptr<IAstExpression>> _value;

    public:
        explicit AstReturnStatement(
            const SourceFragment& source,
            const std::shared_ptr<ParsingContext>& context,
            std::optional<std::unique_ptr<IAstExpression>> value
        ) :
            IAstNode(source, context),
            _value(std::move(value)) {}

        std::string to_string() override;

        llvm::Value* codegen(
            llvm::Module* module,
            llvm::IRBuilderBase* builder) override;


        [[nodiscard]]
        const std::optional<std::unique_ptr<IAstExpression>>& get_return_expression() const
        {
            return this->_value;
        }

        [[nodiscard]]
        bool is_void_type() const
        {
            return !this->_value.has_value();
        }

        void validate() override;

        void resolve_forward_references(
            ParsingContext* context,
            llvm::Module* module,
            llvm::IRBuilderBase* builder
        ) override;

        std::unique_ptr<IAstNode> clone() override;
    };

    std::unique_ptr<AstReturnStatement> parse_return_statement(
        const std::shared_ptr<ParsingContext>& context,
        TokenSet& set);
} // namespace stride::ast
