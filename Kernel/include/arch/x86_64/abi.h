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

constexpr uint8_t SYSCALL_LOG           = 0;
constexpr uint8_t SYSCALL_OPEN          = 1;
constexpr uint8_t SYSCALL_READ          = 2;
constexpr uint8_t SYSCALL_WRITE         = 3;
constexpr uint8_t SYSCALL_SEEK          = 4;
constexpr uint8_t SYSCALL_CLOSE         = 5;
constexpr uint8_t SYSCALL_EXIT          = 6;
constexpr uint8_t SYSCALL_MMAP          = 7;
constexpr uint8_t SYSCALL_SET_FSBASE    = 8;
constexpr uint8_t SYSCALL_MAP_FB        = 9;
constexpr uint8_t NUM_SYSCALLS          = 10;