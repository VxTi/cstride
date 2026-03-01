#include "ast/nodes/literal_values.h"

#include <llvm/IR/Constant.h>
#include <llvm/IR/IRBuilder.h>

using namespace stride::ast;

std::string AstNilLiteral::to_string()
{
    return "nil";
}

std::unique_ptr<IAstExpression> AstNilLiteral::clone()
{
    return std::make_unique<AstNilLiteral>(
        this->get_source_fragment(),
        this->get_context()
    );
}

llvm::Value* AstNilLiteral::codegen(
    llvm::Module* module,
    llvm::IRBuilderBase* builder)
{
    return llvm::Constant::getNullValue(builder->getPtrTy());
}
