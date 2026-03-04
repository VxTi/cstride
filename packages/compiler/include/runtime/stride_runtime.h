#pragma once

#include <cstdint>
#include <memory>

extern "C" {
int __vprintf_internal(const char* format, ...);
uint64_t __system_time_ns_internal();
uint64_t __system_time_us_internal();
uint64_t __system_time_ms_internal();
}
