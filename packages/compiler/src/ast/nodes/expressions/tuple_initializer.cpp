#include "ast/nodes/expression.h"

#include <llvm/IR/IRBuilder.h>

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

std::unique_ptr<IAstNode> AstTupleInitializer::clone()
{
    std::vector<std::unique_ptr<IAstExpression>> cloned_members;
    cloned_members.reserve(this->_members.size());

    for (const auto& member : this->_members)
    {
        cloned_members.push_back(member->clone_as<IAstExpression>());
    }

    return std::make_unique<AstTupleInitializer>(
        this->get_source_fragment(),
        this->get_context(),
        std::move(cloned_members)
    );
}

std::string AstTupleInitializer::to_string()
{
    return "tuple(...)";
}
