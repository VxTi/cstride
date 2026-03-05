#include "../../include/runtime/stride_runtime.h"

#include <chrono>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>

extern "C" {

int _printf_internal(const char* format, ...)
{
    va_list args;
    va_start(args, format);
    int r = vprintf(format, args);
    va_end(args);
    return r;
}

uint64_t _system_time_ns_internal()
{
    return std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::high_resolution_clock::now().time_since_epoch())
       .count();
}

uint64_t _system_time_us_internal()
{
    return std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::high_resolution_clock::now().time_since_epoch())
       .count();
}

uint64_t _system_time_ms_internal()
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::high_resolution_clock::now().time_since_epoch())
       .count();
}

char* _read_in_internal(const int amount)
{
    const auto buffer = static_cast<char*>(malloc((amount + 1) * sizeof(char)));

    if (buffer == nullptr)
        return nullptr;

    const size_t bytesRead = fread(buffer, 1, amount, stdin);

    buffer[bytesRead] = '\0';

    return buffer;
}
}
