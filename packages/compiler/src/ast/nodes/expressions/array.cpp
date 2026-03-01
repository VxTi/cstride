#include "errors.h"
#include "ast/casting.h"
#include "ast/nodes/expression.h"
#include "ast/nodes/types.h"

#include <llvm/IR/Constants.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Module.h>

using namespace stride::ast;

void AstArray::validate_expr()
{
    for (const auto& element : this->get_elements())
    {
        element->validate();
    }
}

void AstArray::resolve_forward_references(
    const ParsingContext* context,
    llvm::Module* module,
    llvm::IRBuilderBase* builder
)
{
    for (const auto& element : this->get_elements())
    {
        element->resolve_forward_references(context, module, builder);
    }
}

std::unique_ptr<IAstExpression> AstArray::clone()
{
    std::vector<std::unique_ptr<IAstExpression>> elements_clone;
    elements_clone.reserve(this->get_elements().size());

    for (const auto& element : this->get_elements())
    {
        elements_clone.push_back(element->clone());
    }

    return std::make_unique<AstArray>(
        this->get_source_fragment(),
        this->get_context(),
        std::move(elements_clone)
    );
}

std::string AstArray::to_string()
{
    return "Array";
}

llvm::Value* AstArray::codegen(
    llvm::Module* module,
    llvm::IRBuilderBase* builder
)
{
    const auto resolved_type = this->get_type();

    llvm::ArrayType* concrete_array_type = nullptr;

    if (const auto* array_type = cast_type<AstArrayType*>(resolved_type))
    {
        llvm::Type* element_llvm_type = type_to_llvm_type(
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
        if (llvm::Type* possible_type = type_to_llvm_type(resolved_type, module);
            llvm::isa<llvm::ArrayType>(possible_type))
        {
            concrete_array_type = llvm::cast<llvm::ArrayType>(possible_type);
        }
        else
        {
            throw parsing_error(
                ErrorType::COMPILATION_ERROR,
                "Codegen failed: Array literal must have a valid array type.",
                this->get_source_fragment()
            );
        }
    }

    const size_t array_size = this->get_elements().size();

    // If we have no members, we'll return a nullptr.
    if (array_size == 0)
    {
        llvm::PointerType* array_ptr_type = llvm::PointerType::get(module->getContext(), 0);
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
        llvm::Value* v = this->get_elements()[i]->codegen(module, builder);

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
        llvm::Constant* init = llvm::ConstantArray::get(concrete_array_type, const_elements);

        const auto global_array = new llvm::GlobalVariable(
            *module,
            concrete_array_type,
            true,
            // isConstant
            llvm::GlobalValue::PrivateLinkage,
            init,
            "" // anonymous
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
        llvm::BasicBlock* saved_ib = builder->GetInsertBlock();

        llvm::Value* element_value = this->get_elements()[i]->codegen(
            module,
            builder);

        // Restore the insert point after element codegen
        builder->SetInsertPoint(saved_ib);

        llvm::Value* elementPtr = builder->CreateInBoundsGEP(
            concrete_array_type,
            array_alloca,
            {
                llvm::ConstantInt::get(llvm::Type::getInt64Ty(module->getContext()), 0),
                llvm::ConstantInt::get(llvm::Type::getInt64Ty(module->getContext()), i),
            });

        builder->CreateStore(element_value, elementPtr);
    }

    return array_alloca;
}
