#include "../../include/runtime/stride_runtime.h"

#include <chrono>
#include <cstdarg>
#include <cstdio>

extern "C" {

int stride_printf(const char* format, ...)
{
    va_list args;
    va_start(args, format);
    const int result = std::vprintf(format, args);
    va_end(args);
    return result;
}

uint64_t system_time_ns()
{
    return std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::high_resolution_clock::now().time_since_epoch())
       .count();
}

uint64_t system_time_us()
{
    return std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::high_resolution_clock::now().time_since_epoch())
       .count();
}

uint64_t system_time_ms()
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::high_resolution_clock::now().time_since_epoch())
       .count();
}

}
