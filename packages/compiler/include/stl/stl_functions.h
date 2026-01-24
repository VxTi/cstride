#pragma once

#include <chrono>
#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <llvm/ExecutionEngine/Orc/LLJIT.h>

/// Standard library functions implemented using C++ STL and exposed to Stride programs.


inline uint64_t cpp_stride_stl_sys_time_ns()
{
    return std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now().time_since_epoch()).
        count();
}

inline void define_sys_time_ns_function(
    llvm::orc::LLJIT* jit,
    const std::shared_ptr<stride::ast::Scope>& global_scope
)
{
    const auto fn_name = "system_time_ns";
    llvm::cantFail(jit->getMainJITDylib().define(
        llvm::orc::absoluteSymbols({
            {
                jit->mangleAndIntern(fn_name),
                {llvm::orc::ExecutorAddr::fromPtr(cpp_stride_stl_sys_time_ns), llvm::JITSymbolFlags::Exported}
            }
        })
    ));

    const auto return_type = stride::ast::AstPrimitiveFieldType(
        nullptr,
        0,
        global_scope,
        stride::ast::PrimitiveType::UINT64,
        64
    );

    global_scope->define_function(fn_name, {}, &return_type);
}


inline uint64_t cpp_stride_stl_time_us()
{
    return std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()).
        count();
}

inline void define_sys_time_us_function(
    llvm::orc::LLJIT* jit,
    const std::shared_ptr<stride::ast::Scope>& global_scope
)
{
    const auto fn_name = "system_time_us";
    llvm::cantFail(jit->getMainJITDylib().define(
        llvm::orc::absoluteSymbols({
            {
                jit->mangleAndIntern(fn_name),
                {llvm::orc::ExecutorAddr::fromPtr(cpp_stride_stl_time_us), llvm::JITSymbolFlags::Exported}
            }
        })
    ));

    const auto return_type = stride::ast::AstPrimitiveFieldType(
        nullptr,
        0,
        global_scope,
        stride::ast::PrimitiveType::UINT64,
        64
    );

    global_scope->define_function(fn_name, {}, &return_type);
}

inline uint64_t cpp_stride_stl_time_ms()
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).
        count();
}

inline void define_sys_time_ms_function(
    llvm::orc::LLJIT* jit,
    const std::shared_ptr<stride::ast::Scope>& global_scope
)
{
    const auto fn_name = "system_time_ms";
    llvm::cantFail(jit->getMainJITDylib().define(
        llvm::orc::absoluteSymbols({
            {
                jit->mangleAndIntern(fn_name),
                {llvm::orc::ExecutorAddr::fromPtr(cpp_stride_stl_time_ms), llvm::JITSymbolFlags::Exported}
            }
        })
    ));

    const auto return_type = stride::ast::AstPrimitiveFieldType(
        nullptr,
        0,
        global_scope,
        stride::ast::PrimitiveType::UINT64,
        64
    );
    global_scope->define_function(fn_name, {}, &return_type);
}

inline void llvm_define_extern_functions(
    llvm::orc::LLJIT* jit,
    const std::shared_ptr<stride::ast::Scope>& global_scope
)
{
    define_sys_time_ns_function(jit, global_scope);
    define_sys_time_us_function(jit, global_scope);
    define_sys_time_ms_function(jit, global_scope);
}
