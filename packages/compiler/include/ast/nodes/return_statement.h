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
    struct SourcePosition;
}

namespace stride::ast
{
    class SymbolTable;

    class AstReturnStatement
        : public IAstNode
    {
        std::optional<std::unique_ptr<IAstExpression>> _value;

    public:
        explicit AstReturnStatement(
            const SourcePosition& source,
            std::optional<std::unique_ptr<IAstExpression>> value
        ) :
            IAstNode(source),
            _value(std::move(value)) {}

        std::string to_string() override;

        llvm::Value* codegen(
            SymbolTable* symbol_table,
            llvm::Module* module, llvm::IRBuilderBase* builder
        ) override;

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

        void validate(SymbolTable* symbol_table) override;

        std::unique_ptr<IAstNode> clone() override;
    };

    std::unique_ptr<AstReturnStatement> parse_return_statement(
        TokenSet& set);
} // namespace stride::ast
