#pragma once

#include <chrono>
#include <cstdio>
#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <llvm/ExecutionEngine/Orc/LLJIT.h>

#include "ast/scope.h"
#include "ast/nodes/types.h"

/// Standard library functions implemented using C++ STL and exposed to Stride programs.

/// Implementation of printf function
inline int impl_printf(const char* format, ...)
{
    va_list args;
    va_start(args, format);
    int result = std::vprintf(format, args);
    va_end(args);
    return result;
}

/// Symbol definition for printf
inline void symbol_def_printf(
    const std::shared_ptr<stride::ast::Scope>& global_scope
)
{
    auto return_type = std::make_unique<stride::ast::AstPrimitiveFieldType>(
        nullptr,
        0,
        global_scope,
        stride::ast::PrimitiveType::INT32,
        32
    );

    // Define printf with variadic arguments support
    global_scope->define_function("printf", {}, std::move(return_type));
}

/// LLVM registration for printf
inline void llvm_reg_printf(llvm::orc::LLJIT* jit)
{
    llvm::cantFail(jit->getMainJITDylib().define(
        llvm::orc::absoluteSymbols({
            {
                jit->mangleAndIntern("printf"),
                {llvm::orc::ExecutorAddr::fromPtr(reinterpret_cast<void*>(impl_printf)), llvm::JITSymbolFlags::Exported}
            }
        })
    ));
}

/// Implementation of function
inline uint64_t impl_sys_time_ns()
{
    return std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now().time_since_epoch()).
        count();
}

/// Symbol definition
inline void symbol_def_sys_time_ns(
    const std::shared_ptr<stride::ast::Scope>& global_scope
)
{
    auto return_type = std::make_unique<stride::ast::AstPrimitiveFieldType>(
        nullptr,
        0,
        global_scope,
        stride::ast::PrimitiveType::UINT64,
        64
    );

    global_scope->define_function("system_time_ns", {}, std::move(return_type));
}

/// LLVM registration
inline void llvm_reg_sys_time_ns(llvm::orc::LLJIT* jit)
{
    llvm::cantFail(jit->getMainJITDylib().define(
        llvm::orc::absoluteSymbols({
            {
                jit->mangleAndIntern("system_time_ns"),
                {llvm::orc::ExecutorAddr::fromPtr(impl_sys_time_ns), llvm::JITSymbolFlags::Exported}
            }
        })
    ));
}

/// Implementation of function
inline uint64_t impl_sys_time_us()
{
    return std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()).
        count();
}

/// Symbol definition
inline void symbol_def_sys_time_us(
    const std::shared_ptr<stride::ast::Scope>& global_scope
)
{
    auto return_type = std::make_unique<stride::ast::AstPrimitiveFieldType>(
        nullptr,
        0,
        global_scope,
        stride::ast::PrimitiveType::UINT64,
        64
    );

    global_scope->define_function("system_time_us", {}, std::move(return_type));
}

/// LLVM registration
inline void llvm_reg_sys_time_us(llvm::orc::LLJIT* jit)
{
    llvm::cantFail(jit->getMainJITDylib().define(
        llvm::orc::absoluteSymbols({
            {
                jit->mangleAndIntern("system_time_us"),
                {llvm::orc::ExecutorAddr::fromPtr(impl_sys_time_us), llvm::JITSymbolFlags::Exported}
            }
        })
    ));
}

inline uint64_t impl_sys_time_ms()
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).
        count();
}

inline void symbol_def_sys_time_ms(
    const std::shared_ptr<stride::ast::Scope>& global_scope
)
{
    auto return_type = std::make_unique<stride::ast::AstPrimitiveFieldType>(
        nullptr,
        0,
        global_scope,
        stride::ast::PrimitiveType::UINT64,
        64
    );
    global_scope->define_function("system_time_ms", {}, std::move(return_type));
}

inline void llvm_reg_sys_time_ms(llvm::orc::LLJIT* jit)
{
    llvm::cantFail(jit->getMainJITDylib().define(
        llvm::orc::absoluteSymbols({
            {
                jit->mangleAndIntern("system_time_ms"),
                {llvm::orc::ExecutorAddr::fromPtr(impl_sys_time_ms), llvm::JITSymbolFlags::Exported}
            }
        })
    ));
}

inline void llvm_declare_extern_function_prototypes(llvm::Module* module, llvm::LLVMContext& context)
{
    // Signature: uint64_t system_time_*()
    auto* ret_ty = llvm::Type::getInt64Ty(context);
    auto* fn_ty = llvm::FunctionType::get(ret_ty, /*isVarArg=*/false);

    // getOrInsertFunction ensures the Function exists in the module symbol table.
    // This is what AstFunctionCall::codegen() later finds via module->getFunction(...).
    module->getOrInsertFunction("system_time_ns", fn_ty);
    module->getOrInsertFunction("system_time_us", fn_ty);
    module->getOrInsertFunction("system_time_ms", fn_ty);

    // Signature: int printf(const char* format, ...)
    auto* printf_ret_ty = llvm::Type::getInt32Ty(context);
    auto* printf_fn_ty = llvm::FunctionType::get(
        printf_ret_ty,
        {llvm::PointerType::get(context, 0)},
        /*isVarArg=*/true
    );
    module->getOrInsertFunction("printf", printf_fn_ty);
}

inline void llvm_predefine_symbols(const std::shared_ptr<stride::ast::Scope>& global_scope)
{
    symbol_def_sys_time_ns(global_scope);
    symbol_def_sys_time_us(global_scope);
    symbol_def_sys_time_ms(global_scope);
    symbol_def_printf(global_scope);
}

inline void llvm_define_extern_functions(llvm::orc::LLJIT* jit)
{
    // system_time_ns
    llvm_reg_sys_time_ns(jit);

    // system_time_us
    llvm_reg_sys_time_us(jit);

    // system_time_ms
    llvm_reg_sys_time_ms(jit);

    // printf
    llvm_reg_printf(jit);
}
