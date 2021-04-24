#pragma once

#ifndef __cplusplus
#error C++ only
#endif

#include <stdint.h>

constexpr uint16_t PAGE_SIZE = 4096;
constexpr uint64_t PAGE_SIZE_MASK = 0xfffffffffffff000;
constexpr uint64_t PAGE_SIZE_OFFSET = ~(PAGE_SIZE_MASK);