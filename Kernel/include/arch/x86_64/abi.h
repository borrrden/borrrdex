#pragma once

#include <stdint.h>

typedef struct {
    uint64_t a_type;
    uint64_t a_val;
} auxv_t;

constexpr uint8_t AT_PHDR       = 3;
constexpr uint8_t AT_PHENT      = 4;
constexpr uint8_t AT_PHNUM      = 5;
constexpr uint8_t AT_ENTRY      = 9;
constexpr uint8_t AT_EXECPATH   = 15;
constexpr uint8_t AT_RANDOM     = 25;
constexpr uint8_t AT_EXECFN     = 31;