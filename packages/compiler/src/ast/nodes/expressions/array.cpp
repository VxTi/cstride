#include "ast/nodes/expression.h"
#include "ast/nodes/types.h"
#include "ast/optionals.h"

#include <llvm/IR/Module.h>

using namespace stride::ast;


void AstArray::validate()
{
    for (const auto& element : this->get_elements())
    {
        element->validate();
    }
}

void AstArray::resolve_forward_references(
    const ParsingContext* context,
    llvm::Module* module,
    llvm::IRBuilder<>* builder
)
{
    for (const auto& element : this->get_elements())
    {
        element->resolve_forward_references(context, module, builder);
    }
}

std::string AstArray::to_string()
{
    return "Array";
}

llvm::Value* AstArray::codegen(
    llvm::Module* module,
    llvm::IRBuilder<>* builder
)
{
    const auto resolved_type = infer_expression_type(this->get_context(), this);

    llvm::ArrayType* concrete_array_type = nullptr;

    if (const auto* array_type = dynamic_cast<AstArrayType*>(resolved_type.
        get()))
    {
        llvm::Type* element_llvm_type = internal_type_to_llvm_type(
            array_type->get_element_type(),
            module
        );
        concrete_array_type = llvm::ArrayType::get(
            element_llvm_type,
            this->get_elements().size()
        );
    }
    else
    {
        // Fallback: If we can't determine the type from the AST, try to verify
        // if the resolved LLVM type is already an array type.
        if (llvm::Type* possible_type = internal_type_to_llvm_type(
            resolved_type.get(),
            module); llvm::isa<llvm::ArrayType>(possible_type))
        {
            concrete_array_type = llvm::cast<llvm::ArrayType>(possible_type);
        }
        else
        {
            throw std::runtime_error(
                "Codegen failed: Array literal must have a valid array type.");
        }
    }

    const size_t array_size = this->get_elements().size();

    // If we have no members, we'll return a nullptr.
    if (array_size == 0)
    {
        llvm::PointerType* array_ptr_type = llvm::PointerType::get(
            module->getContext(),
            0);
        return llvm::ConstantPointerNull::get(array_ptr_type);
    }

    // Try to build a constant aggregate initializer
    bool all_const_initializers = true;
    std::vector<llvm::Constant*> const_elements;
    const_elements.reserve(array_size);

    // Save the insert point before generating elements
    llvm::BasicBlock* saved_block = builder->GetInsertBlock();

    for (size_t i = 0; i < array_size; ++i)
    {
        llvm::Value* v = this->get_elements()[i]->codegen(
            module,
            builder);

        // Restore insert point after each element (in case it's a lambda)
        builder->SetInsertPoint(saved_block);

        auto* c = llvm::dyn_cast<llvm::Constant>(v);
        if (!c)
        {
            all_const_initializers = false;
            break;
        }
        const_elements.push_back(c);
    }

    // If we have all-const initializers, create a global array
    // (This is important for arrays used in global variable initialization)
    if (all_const_initializers)
    {
        llvm::Constant* init = llvm::ConstantArray::get(
            concrete_array_type,
            const_elements);

        llvm::GlobalVariable* global_array = new llvm::GlobalVariable(
            *module,
            concrete_array_type,
            true,  // isConstant
            llvm::GlobalValue::PrivateLinkage,
            init,
            ""  // anonymous
        );

        return global_array;
    }

    // For non-constant arrays, use stack allocation
    llvm::Value* array_alloca = builder->CreateAlloca(concrete_array_type);

    // Fallback: element-by-element stores into the aggregate
    for (size_t i = 0; i < array_size; ++i)
    {
        // Save the current insert point before generating element code
        // (element codegen might be a lambda that redirects the builder)
        llvm::BasicBlock* saved_block = builder->GetInsertBlock();

        llvm::Value* element_value = this->get_elements()[i]->codegen(
            module,
            builder);

        // Restore the insert point after element codegen
        builder->SetInsertPoint(saved_block);

        llvm::Value* elementPtr = builder->CreateInBoundsGEP(
            concrete_array_type,
            array_alloca,
            {
                llvm::ConstantInt::get(
                    llvm::Type::getInt64Ty(module->getContext()),
                    0),
                llvm::ConstantInt::get(
                    llvm::Type::getInt64Ty(module->getContext()),
                    i),
            });

        builder->CreateStore(element_value, elementPtr);
    }

    return array_alloca;
}
