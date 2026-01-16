#pragma once

#include "expression.h"
#include "ast/tokens/token.h"

namespace stride::ast
{
    class AstLogicalOp :
        public AstExpression
    {
    public:
        std::unique_ptr<AstExpression> left;
        TokenType op;
        std::unique_ptr<AstExpression> right;

        AstLogicalOp(
            std::unique_ptr<AstExpression> left,
            TokenType op,
            std::unique_ptr<AstExpression> right
        );

        llvm::Value* codegen(llvm::Module* module, llvm::LLVMContext& context, llvm::IRBuilder<>* builder) override;

        std::string to_string() override;
    };
}
