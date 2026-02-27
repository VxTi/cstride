#pragma once

namespace llvm {
    class IRBuilderBase;
    class Module;
    class Value;
}

namespace stride::ast
{
    class AstExpression;


    llvm::Value* codegen_conditional_value(
        llvm::Module* module,
        llvm::IRBuilderBase* builder,
        AstExpression* condition
    );
}
