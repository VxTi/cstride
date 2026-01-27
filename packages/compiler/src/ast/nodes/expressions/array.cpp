#include "ast/nodes/expression.h"

using namespace stride::ast;


void AstArray::validate() {}

std::string AstArray::to_string()
{
    return "Array";
}

llvm::Value* AstArray::codegen(
    const std::shared_ptr<SymbolRegistry>& scope,
    llvm::Module* module,
    llvm::LLVMContext& context,
    llvm::IRBuilder<>* builder
)
{
    const auto resolved_type = infer_expression_type(this->scope, this);

    llvm::Type* llvm_array_type = internal_type_to_llvm_type(resolved_type.get(), module, context);

    const size_t array_size = this->get_elements().size();

    // If we have no members, we'll return a nullptr.
    if (array_size == 0)
    {
        llvm::PointerType* array_ptr_type = llvm::PointerType::get(context, 0);
        return llvm::ConstantPointerNull::get(array_ptr_type);
    }

    llvm::Value* array_alloca = builder->CreateAlloca(llvm_array_type);

    // Try to build a constant aggregate initializer
    bool all_const_initializers = true;
    std::vector<llvm::Constant*> const_elements;
    const_elements.reserve(array_size);

    for (size_t i = 0; i < array_size; ++i)
    {
        llvm::Value* v = this->get_elements()[i]->codegen(scope, module, context, builder);
        auto* c = llvm::dyn_cast<llvm::Constant>(v);
        if (!c)
        {
            all_const_initializers = false;
            break;
        }
        const_elements.push_back(c);
    }

    // If we have all-const initializers, we can store them
    // all in a single [N x T] operation
    if (all_const_initializers)
    {
        llvm::Constant* init = llvm::ConstantArray::get(llvm::cast<llvm::ArrayType>(llvm_array_type), const_elements);
        builder->CreateStore(init, array_alloca);
        return array_alloca;
    }

    // Fallback: element-by-element stores into the aggregate
    for (size_t i = 0; i < array_size; ++i)
    {
        llvm::Value* element_value = this->get_elements()[i]->codegen(scope, module, context, builder);

        llvm::Value* elementPtr = builder->CreateInBoundsGEP(
            llvm_array_type,
            array_alloca,
            {
                llvm::ConstantInt::get(llvm::Type::getInt64Ty(context), 0),
                llvm::ConstantInt::get(llvm::Type::getInt64Ty(context), i),
            }
        );

        builder->CreateStore(element_value, elementPtr);
    }

    return array_alloca;
}
