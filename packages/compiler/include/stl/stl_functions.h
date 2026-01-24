#pragma once

#include <chrono>

inline long long int $__stride_stl_sys_time_ns()
{
    auto now = std::chrono::high_resolution_clock::now();

    return std::chrono::duration_cast<std::chrono::nanoseconds>(now).count();
}

extern "C" long long int stride_stl_sys_time_ns()
{
    return $__stride_stl_sys_time_ns();
}

inline void llvm_define_extern_functions(
    llvm::orc::LLJIT* jit
)
{
    // Manually register the fflush wrapper to handle fflush(1) and fflush(2)
    llvm::cantFail(jit->getMainJITDylib().define(
        llvm::orc::absoluteSymbols({
            {
                jit->mangleAndIntern("stride_stl_sys_time_ns"),
                {llvm::orc::ExecutorAddr::fromPtr(stride_stl_sys_time_ns), llvm::JITSymbolFlags::Exported}
            }
        })
    ));
}