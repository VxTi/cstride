#include "stl/stl_functions.h"

using namespace stride::stl;

int impl_printf(const char* format, ...)
{
    va_list args;
    va_start(args, format);
    const int result = std::vprintf(format, args);
    va_end(args);
    return result;
}

uint64_t impl_sys_time_ns()
{
    return std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::high_resolution_clock::now().time_since_epoch())
       .count();
}

uint64_t impl_sys_time_us()
{
    return std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::high_resolution_clock::now().time_since_epoch())
       .count();
}

uint64_t impl_sys_time_ms()
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::high_resolution_clock::now().time_since_epoch())
       .count();
}

void stride::stl::llvm_insert_function_definitions(llvm::Module* module)
{
    // Signature: uint64_t system_time_*()
    auto* ret_ty = llvm::Type::getInt64Ty(module->getContext());
    auto* fn_ty = llvm::FunctionType::get(ret_ty, /*isVarArg=*/false);

    // getOrInsertFunction ensures the Function exists in the module symbol table.
    // This is what AstFunctionCall::codegen() later finds via module->getFunction(...).
    module->getOrInsertFunction("system_time_ns", fn_ty);
    module->getOrInsertFunction("system_time_us", fn_ty);
    module->getOrInsertFunction("system_time_ms", fn_ty);

    // Signature: int printf(const char* format, ...)
    auto* printf_ret_ty = llvm::Type::getInt32Ty(module->getContext());
    auto* printf_fn_ty = llvm::FunctionType::get(
        printf_ret_ty,
        {llvm::PointerType::get(module->getContext(), 0)},
        /*isVarArg=*/true
    );
    module->getOrInsertFunction("printf", printf_fn_ty);
}

void stride::stl::predefine_symbols(const std::shared_ptr<ast::SymbolRegistry>& global_scope)
{
    /// Printf definition
    global_scope->define_function(
        "printf", {}, std::make_unique<ast::AstPrimitiveType>(
            nullptr,
            SourcePosition(0, 0),
            global_scope,
            ast::PrimitiveType::INT32,
            32
        ));

    /// System time in nanoseconds definition
    global_scope->define_function(
        "system_time_ns", {},
        std::make_unique<ast::AstPrimitiveType>(
            nullptr,
            SourcePosition(0, 0),
            global_scope,
            ast::PrimitiveType::UINT64,
            64
        ));

    /// System time in microseconds definition
    global_scope->define_function(
        "system_time_us", {}, std::make_unique<ast::AstPrimitiveType>(
            nullptr,
            SourcePosition(0, 0),
            global_scope,
            ast::PrimitiveType::UINT64,
            64
        ));

    /// System time in milliseconds definition
    global_scope->define_function(
        "system_time_ms", {}, std::make_unique<ast::AstPrimitiveType>(
            nullptr,
            SourcePosition(0, 0),
            global_scope,
            ast::PrimitiveType::UINT64,
            64
        ));
}

void stride::stl::llvm_jit_define_functions(llvm::orc::LLJIT* jit)
{
    llvm::cantFail(jit->getMainJITDylib().define(
        llvm::orc::absoluteSymbols({
            {
                jit->mangleAndIntern("system_time_ns"),
                {
                    llvm::orc::ExecutorAddr::fromPtr(impl_sys_time_ns), llvm::JITSymbolFlags::Exported
                }
            },
            {
                jit->mangleAndIntern("system_time_us"),
                {
                    llvm::orc::ExecutorAddr::fromPtr(impl_sys_time_us), llvm::JITSymbolFlags::Exported
                }
            },
            {
                jit->mangleAndIntern("system_time_ms"),
                {
                    llvm::orc::ExecutorAddr::fromPtr(impl_sys_time_ms), llvm::JITSymbolFlags::Exported
                }
            },
            {
                jit->mangleAndIntern("printf"),
                {
                    llvm::orc::ExecutorAddr::fromPtr(impl_printf), llvm::JITSymbolFlags::Exported
                }
            }
        })
    ));
}
