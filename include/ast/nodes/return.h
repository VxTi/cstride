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

        static bool can_parse(const TokenSet& tokens);

        static std::unique_ptr<AstReturn> try_parse(const Scope& scope, TokenSet& tokens);

        [[nodiscard]] IAstNode* value() const { return _value.get(); }
    };
}

