#include "ast/nodes/expression.h"

using namespace stride::ast;


llvm::Value* AstTupleInitializer::codegen(llvm::Module* module, llvm::IRBuilderBase* builder)
{
    const auto insertion_block = builder->GetInsertBlock();

    std::vector<llvm::Value*> member_values;
    member_values.reserve(this->_members.size());

    for (const auto& member : this->_members)
    {
        member_values.push_back(member->codegen(module, builder));
    }

    return nullptr;
}

std::string AstTupleInitializer::to_string()
{
    return "tuple(...)";
}

void AstTupleInitializer::validate()
{

}
