#pragma once

#include <cstdarg>
#include <cstdint>
#include <memory>

extern "C" {
int _printf_internal(const char* format, va_list args);
uint64_t _system_time_ns_internal();
uint64_t _system_time_us_internal();
uint64_t _system_time_ms_internal();
char* _read_in_internal(int amount);
}
