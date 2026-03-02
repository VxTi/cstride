#include "errors.h"
#include "ast/nodes/expression.h"

#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Module.h>

using namespace stride::ast;

namespace
{
    /// Get or declare the va_start intrinsic for the current module
    llvm::Function* get_or_declare_va_start(llvm::Module* module)
    {
        llvm::Type* i8_ptr_ty = llvm::PointerType::get(module->getContext(), 0);
        return getOrInsertDeclaration(
            module,
            llvm::Intrinsic::vastart,
            { i8_ptr_ty }
        );
    }

    /// Get or declare the va_copy intrinsic for the current module
    llvm::Function* get_or_declare_va_copy(llvm::Module* module)
    {
        llvm::Type* i8_ptr_ty = llvm::PointerType::get(module->getContext(), 0);
        return getOrInsertDeclaration(
            module,
            llvm::Intrinsic::vacopy,
            { i8_ptr_ty }
        );
    }

    /// Get or declare the va_end intrinsic for the current module
    llvm::Function* get_or_declare_va_end(llvm::Module* module)
    {
        llvm::Type* i8_ptr_ty = llvm::PointerType::get(module->getContext(), 0);
        return getOrInsertDeclaration(
            module,
            llvm::Intrinsic::vaend,
            { i8_ptr_ty }
        );
    }

llvm::Type* get_va_list_type(const llvm::Module* module) {
    const llvm::Triple& triple(module->getTargetTriple());

    // x86_64 System V ABI (Linux, macOS, BSD)
    if (triple.getArch() == llvm::Triple::x86_64 && !triple.isOSWindows()) {
        // struct __va_list_tag {
        //   i32 gp_offset;
        //   i32 fp_offset;
        //   i8* overflow_arg_area;
        //   i8* reg_save_area;
        // };
        llvm::Type* i32_ty = llvm::Type::getInt32Ty(module->getContext());
        llvm::Type* i8_ptr_ty = llvm::PointerType::get(module->getContext(), 0);

        llvm::StructType* va_list_struct = llvm::StructType::create(module->getContext(), "struct.__va_list_tag");
        va_list_struct->setBody({i32_ty, i32_ty, i8_ptr_ty, i8_ptr_ty});

        // This ABI usually passes va_list as an array of 1 of this struct
        return llvm::ArrayType::get(va_list_struct, 1);
    }

    // AArch64 (ARM64)
    if (triple.getArch() == llvm::Triple::aarch64) {
        // struct __va_list {
        //    void *__stack;
        //    void *__gr_top;
        //    void *__vr_top;
        //    int   __gr_off;
        //    int   __vr_off;
        // };
        llvm::Type* i32_ty = llvm::Type::getInt32Ty(module->getContext());
        llvm::Type* i8_ptr_ty = llvm::PointerType::get(module->getContext(), 0);

        return llvm::StructType::get(module->getContext(), {i8_ptr_ty, i8_ptr_ty, i8_ptr_ty, i32_ty, i32_ty});
    }

    // Default (Windows x64, 32-bit x86, many others) - simple char* pointer
    return llvm::PointerType::get(module->getContext(), 0);
}

}

llvm::Value* AstVariadicArgReference::codegen(llvm::Module* module, llvm::IRBuilderBase* builder)
{
    // Verify we're inside a variadic function
    if (const llvm::Function* current_function = builder->GetInsertBlock()->getParent();
        !current_function || !current_function->isVarArg())
    {
        throw parsing_error(
            ErrorType::SEMANTIC_ERROR,
            "Variadic argument reference '...' can only be used inside a variadic function",
            this->get_source_fragment()
        );
    }

    // Create and initialize a va_list to access the variadic arguments
    llvm::Type* va_list_ty = get_va_list_type(module);

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

    // Declare the va_start intrinsic and call it to initialize the va_list
    // va_start captures the current state of the variadic arguments
    llvm::Function* va_start_fn = get_or_declare_va_start(module);
    builder->CreateCall(va_start_fn, { va_list_i8_ptr });

    // When forwarding variadic arguments, we need to create a copy of the va_list
    // This is because the receiving function will consume the va_list, and we may
    // need the original for cleanup or multiple uses.
    llvm::AllocaInst* va_list_copy_ptr = builder->CreateAlloca(
        va_list_ty,
        nullptr,
        "varargs_list_copy"
    );

    llvm::Value* va_list_copy_i8_ptr = builder->CreateBitCast(
        va_list_copy_ptr,
        llvm::PointerType::get(module->getContext(), 0),
        "varargs_list_copy.cast"
    );

    // Declare and call va_copy to create a copy of the va_list
    // va_copy(dest, src) copies the state from src to dest
    llvm::Function* va_copy_fn = get_or_declare_va_copy(module);
    builder->CreateCall(va_copy_fn, { va_list_copy_i8_ptr, va_list_i8_ptr });

    // Clean up the original va_list immediately since we're using the copy
    // This ensures proper resource management
    llvm::Function* va_end_fn = get_or_declare_va_end(module);
    builder->CreateCall(va_end_fn, { va_list_i8_ptr });

    // Return the copied va_list pointer
    // Note: The caller (or function cleanup) is responsible for calling va_end
    // on this copy when done using it
    return va_list_copy_ptr;
}

std::unique_ptr<IAstNode> AstVariadicArgReference::clone()
{
    return std::make_unique<AstVariadicArgReference>(
        this->get_source_fragment(),
        this->get_context()
    );
}

std::string AstVariadicArgReference::to_string()
{
    return "...";
}
