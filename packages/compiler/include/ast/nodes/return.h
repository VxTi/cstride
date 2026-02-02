#pragma once

#include "ast_node.h"
#include "expression.h"
#include "ast/symbol_registry.h"
#include "ast/tokens/token_set.h"

namespace stride::ast
{
    class AstReturn :
        public IAstNode,
        public ISynthesisable
    {
        const std::unique_ptr<AstExpression> _value;

    public:
        explicit AstReturn(
            const std::shared_ptr<SourceFile>& source,
            const SourcePosition source_position,
            const std::shared_ptr<SymbolRegistry>& registry,
            std::unique_ptr<AstExpression> value
        )
            :
            IAstNode(source, source_position, registry),
            _value(std::move(value)) {}

        std::string to_string() override;

        llvm::Value* codegen(
            const std::shared_ptr<SymbolRegistry>& registry,
            llvm::Module* module,
            llvm::IRBuilder<>* builder
        ) override;


        [[nodiscard]]
        AstExpression* get_return_expr() const { return _value.get(); }

        void validate() override;
    };

    bool is_return_statement(const TokenSet& tokens);

    std::unique_ptr<AstReturn> parse_return_statement(const std::shared_ptr<SymbolRegistry>& registry, TokenSet& set);
}
