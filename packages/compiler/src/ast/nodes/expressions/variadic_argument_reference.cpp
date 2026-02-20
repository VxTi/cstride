#include "ast/nodes/expression.h"
#include <llvm/IR/Intrinsics.h>
#include <llvm/IR/Module.h>

using namespace stride::ast;

namespace
{
    /// Get or declare the va_start intrinsic for the current module
    llvm::Function* get_or_declare_va_start(llvm::Module* module)
    {
        constexpr std::string intrinsic_name = "llvm.va_start";
        if (auto* existing = module->getFunction(intrinsic_name))
            return existing;

        llvm::Type* void_ty = llvm::Type::getVoidTy(module->getContext());
        llvm::Type* i8_ptr_ty = llvm::PointerType::get(module->getContext(), 0);

        llvm::FunctionType* ft = llvm::FunctionType::get(
            void_ty,
            {i8_ptr_ty},
            false
        );

        return llvm::Function::Create(
            ft,
            llvm::Function::ExternalLinkage,
            intrinsic_name,
            module
        );
    }

    /// Get or declare the va_copy intrinsic for the current module
    llvm::Function* get_or_declare_va_copy(llvm::Module* module)
    {
        constexpr std::string intrinsic_name = "llvm.va_copy";
        if (auto* existing = module->getFunction(intrinsic_name))
            return existing;

        llvm::Type* void_ty = llvm::Type::getVoidTy(module->getContext());
        llvm::Type* i8_ptr_ty = llvm::PointerType::get(module->getContext(), 0);

        llvm::FunctionType* ft = llvm::FunctionType::get(
            void_ty,
            {i8_ptr_ty, i8_ptr_ty},
            false
        );

        return llvm::Function::Create(
            ft,
            llvm::Function::ExternalLinkage,
            intrinsic_name,
            module
        );
    }

    /// Get or declare the va_end intrinsic for the current module
    llvm::Function* get_or_declare_va_end(llvm::Module* module)
    {
        constexpr std::string intrinsic_name = "llvm.va_end";
        if (auto* existing = module->getFunction(intrinsic_name))
            return existing;

        llvm::Type* void_ty = llvm::Type::getVoidTy(module->getContext());
        llvm::Type* i8_ptr_ty = llvm::PointerType::get(module->getContext(), 0);

        llvm::FunctionType* ft = llvm::FunctionType::get(
            void_ty,
            {i8_ptr_ty},
            false
        );

        return llvm::Function::Create(
            ft,
            llvm::Function::ExternalLinkage,
            intrinsic_name,
            module
        );
    }
}

llvm::Value* AstVariadicArgReference::codegen(const ParsingContext* context, llvm::Module* module, llvm::IRBuilder<>* builder)
{
    // Verify we're inside a variadic function
    llvm::Function* current_function = builder->GetInsertBlock()->getParent();
    if (!current_function || !current_function->isVarArg())
    {
        throw parsing_error(
            ErrorType::SEMANTIC_ERROR,
            "Variadic argument reference '...' can only be used inside a variadic function",
            this->get_source_fragment()
        );
    }

    // Create and initialize a va_list to access the variadic arguments
    llvm::Type* i8_ty = llvm::Type::getInt8Ty(module->getContext());
    llvm::Type* va_list_ty = llvm::ArrayType::get(i8_ty, 24); // Standard va_list size

    // Allocate space for the va_list on the stack
    llvm::AllocaInst* va_list_ptr = builder->CreateAlloca(
        va_list_ty,
        nullptr,
        "varargs_list"
    );

    // Cast to i8* for the intrinsic calls
    llvm::Value* va_list_i8_ptr = builder->CreateBitCast(
        va_list_ptr,
        llvm::PointerType::get(module->getContext(), 0),
        "varargs_list.cast"
    );

    // Get or declare and call the va_start intrinsic
    llvm::Function* va_start_fn = get_or_declare_va_start(module);
    builder->CreateCall(va_start_fn, {va_list_i8_ptr});

    // Return the va_list pointer
    // Note: The caller is responsible for calling va_end when done
    return va_list_ptr;
}

void AstVariadicArgReference::validate()
{
    // Validation is performed during codegen to ensure we're in a variadic function context
}

std::string AstVariadicArgReference::to_string()
{
    return "...";
}
