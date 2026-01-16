#pragma once
#include <utility>

#include "ast_node.h"
#include "ast/scope.h"
#include "ast/symbol.h"
#include "ast/tokens/token_set.h"

namespace stride::ast
{
    class AstFunctionInvocation :
        public virtual IAstNode,
        public virtual ISynthesisable
    {
    public:
        const Symbol function_name;

        explicit AstFunctionInvocation(Symbol function_name) : function_name(std::move(function_name))
        {
        }

        static bool can_parse(const TokenSet& tokens);

        static std::unique_ptr<AstFunctionInvocation> try_parse(const Scope& scope, TokenSet& tokens);

        std::string to_string() override;

        llvm::Value* codegen(llvm::Module* module, llvm::LLVMContext& context) override;
    };
}
