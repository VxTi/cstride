#pragma once

namespace llvm
{
    class IRBuilderBase;
    class Module;
    class Value;
}

namespace stride::ast
{
    class SymbolTable;
    class IAstExpression;

    llvm::Value* codegen_conditional_value(
        SymbolTable* symbol_table,
        llvm::Module* module,
        llvm::IRBuilderBase* builder,
        IAstExpression* condition
    );
}
