#pragma once

#include <cstdint>
#include <memory>

extern "C" {
int stride_printf(const char* format, ...);
uint64_t system_time_ns();
uint64_t system_time_us();
uint64_t system_time_ms();
}
