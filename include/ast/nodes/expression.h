#pragma once

#include "ast_node.h"
#include "ast/scope.h"
#include "ast/tokens/token_set.h"

namespace stride::ast
{
#define EXPRESSION_VARIABLE_DECLARATION        1
#define EXPRESSION_INLINE_VARIABLE_DECLARATION 2 // Variables declared after initial one

    class AstExpression :
        public virtual IAstNode,
        public virtual ISynthesisable
    {
    public:
        const std::vector<std::unique_ptr<IAstNode>> children;

        explicit AstExpression(std::vector<std::unique_ptr<IAstNode>> children) : children(std::move(children))
        {
        };

        llvm::Value* codegen(llvm::Module* module, llvm::LLVMContext& context) override;

        std::string to_string() override;

        static bool can_parse(const TokenSet& tokens);

        static std::unique_ptr<AstExpression> try_parse(const Scope& scope, TokenSet& tokens);

        static std::unique_ptr<AstExpression> try_parse_expression(
            int expression_type_flags,
            const Scope& scope,
            const TokenSet& tokens
        );
    };
}
