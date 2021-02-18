#pragma once

#ifndef __cplusplus
#error C++ Only
#endif

#include <cstdint>

constexpr uint8_t CONFIG_MAX_THREADS = 32;
constexpr uint16_t CONFIG_THREAD_STACKSIZE = 4096;
constexpr uint8_t CONFIG_MAX_CPUS = 1;
constexpr uint16_t CONFIG_MAX_SEMAPHORES = 128;
