#pragma once
#include <cstdint>
#include <memory>

namespace llvm::orc
{
    class LLJIT;
}

namespace stride::ast
{
    class ParsingContext;
}

extern "C" {
int stride_printf(const char* format, ...);
uint64_t system_time_ns();
uint64_t system_time_us();
uint64_t system_time_ms();
}

namespace stride::runtime
{
    void register_runtime_symbols(const std::shared_ptr<ast::ParsingContext>& context);

    void register_jit_symbols(llvm::orc::LLJIT* jit);
}
