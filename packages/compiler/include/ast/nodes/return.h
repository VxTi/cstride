#pragma once

#include "ast_node.h"
#include "ast/symbol_registry.h"
#include "ast/tokens/token_set.h"

namespace stride::ast
{
    class AstReturn :
        public IAstNode,
        public ISynthesisable
    {
        const std::unique_ptr<IAstNode> _value;

    public:
        explicit AstReturn(
            const std::shared_ptr<SourceFile>& source,
            const SourcePosition source_position,
            const std::shared_ptr<SymbolRegistry>& scope,
            std::unique_ptr<IAstNode> value
        )
            :
            IAstNode(source, source_position, scope),
            _value(std::move(value)) {}

        std::string to_string() override;

        llvm::Value* codegen(
            const std::shared_ptr<SymbolRegistry>& scope,
            llvm::Module* module,
            llvm::LLVMContext& context,
            llvm::IRBuilder<>* builder
        ) override;


        [[nodiscard]]
        IAstNode* value() const { return _value.get(); }
    };

    bool is_return_statement(const TokenSet& tokens);

    std::unique_ptr<AstReturn> parse_return_statement(const std::shared_ptr<SymbolRegistry>& scope, TokenSet& set);
}
