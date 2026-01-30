#include <llvm/IR/Module.h>
#include <llvm/IR/ValueSymbolTable.h>

#include "ast/nodes/expression.h"

using namespace stride::ast;

llvm::Value* AstIdentifier::codegen(const std::shared_ptr<SymbolRegistry>& scope, llvm::Module* module, llvm::IRBuilder<>* builder)
{
    llvm::Value* val = nullptr;

    std::string internal_name;

    if (const auto definition = scope->field_lookup(this->get_name()))
    {
        internal_name = definition->get_internal_symbol_name();
    }
    else
    {
        internal_name = this->get_internal_name();
    }

    if (const auto block = builder->GetInsertBlock())
    {
        if (const auto function = block->getParent())
        {
            val = function->getValueSymbolTable()->lookup(internal_name);
        }
    }

    // Check if it's a function argument
    if (auto* arg = llvm::dyn_cast_or_null<llvm::Argument>(val))
    {
        return arg;
    }

    if (auto* alloca = llvm::dyn_cast_or_null<llvm::AllocaInst>(val))
    {
        // Load the value from the allocated variable
        // Note: This is safe because 'val' is only found if GetInsertBlock() was not null
        return builder->CreateLoad(alloca->getAllocatedType(), alloca, internal_name);
    }

    if (const auto global = module->getNamedGlobal(internal_name))
    {
        // Only generate a Load instruction if we are inside a BasicBlock (Function Scope).
        if (builder->GetInsertBlock())
        {
            return builder->CreateLoad(global->getValueType(), global, internal_name);
        }

        // If we are in Global Scope (initializing a global variable), we cannot generate instructions.
        // We return the GlobalVariable* itself. This allows parent nodes (like MemberAccessor)
        // to perform Constant Folding or ConstantExpr GEPs on the address.
        return global;
    }

    if (auto* function = module->getFunction(internal_name))
    {
        return function;
    }

    llvm::errs() << "Unknown variable or function name: " << internal_name << "\n";
    return nullptr;
}

std::string AstIdentifier::to_string()
{
    return std::format("Identifier<{}({})>", this->get_name(), this->get_internal_name());
}
