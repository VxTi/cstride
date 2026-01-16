#pragma once

#include "ast_node.h"
#include "ast/scope.h"
#include "ast/tokens/token_set.h"

namespace stride::ast
{
    class AstReturn :
        public virtual IAstNode,
        public virtual ISynthesisable
    {
        const std::unique_ptr<IAstNode> _value;

    public:
        explicit AstReturn(std::unique_ptr<IAstNode> value)
            : _value(std::move(value))
        {
        }

        std::string to_string() override;

        llvm::Value* codegen(llvm::Module* module, llvm::LLVMContext& context, llvm::IRBuilder<>* builder) override;


        [[nodiscard]] IAstNode* value() const { return _value.get(); }
    };

    bool is_return_statement(const TokenSet& tokens);

    std::unique_ptr<AstReturn> parse_return_statement(const Scope& scope, TokenSet& tokens);
}
