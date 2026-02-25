#pragma once
#include <llvm/IR/IRBuilder.h>

namespace stride::ast
{
    class AstExpression;

    llvm::Value* codegen_conditional_value(
        llvm::Module* module,
        llvm::IRBuilder<>* builder,
        AstExpression* condition
    );
}
