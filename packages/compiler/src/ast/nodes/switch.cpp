
#include "ast/nodes/switch.h"

using namespace stride::ast;

llvm::Value* AstSwitch::codegen(SymbolTable* symbol_table, llvm::Module* module, llvm::IRBuilderBase* builder)
{
    return nullptr;
}

std::string AstSwitch::to_string()
{
    return "switch (...) { ... }";
}
