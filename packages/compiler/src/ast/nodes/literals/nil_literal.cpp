#include "ast/nodes/literal_values.h"

using namespace stride::ast;

std::string AstNilLiteral::to_string()
{
    return "nil";
}

llvm::Value* AstNilLiteral::codegen(
    llvm::Module* module,
    llvm::IRBuilder<>* builder)
{
    return llvm::Constant::getNullValue(builder->getPtrTy());
}
