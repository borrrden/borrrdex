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

constexpr uint8_t SYSCALL_LOG               = 0;
constexpr uint8_t SYSCALL_OPEN              = 1;
constexpr uint8_t SYSCALL_READ              = 2;
constexpr uint8_t SYSCALL_WRITE             = 3;
constexpr uint8_t SYSCALL_SEEK              = 4;
constexpr uint8_t SYSCALL_CLOSE             = 5;
constexpr uint8_t SYSCALL_IOCTL             = 6;
constexpr uint8_t SYSCALL_EXIT              = 7;
constexpr uint8_t SYSCALL_MMAP              = 8;
constexpr uint8_t SYSCALL_SET_FSBASE        = 9;
constexpr uint8_t SYSCALL_MAP_FB            = 10;
constexpr uint8_t SYSCALL_GRANT_PTY         = 11;
constexpr uint8_t SYSCALL_UPTIME            = 12;
constexpr uint8_t SYSCALL_GETUID            = 13;
constexpr uint8_t SYSCALL_SETUID            = 14;
constexpr uint8_t SYSCALL_GETGID            = 15;
constexpr uint8_t SYSCALL_SETGID            = 16;
constexpr uint8_t SYSCALL_GETEUID           = 17;
constexpr uint8_t SYSCALL_SETEUID           = 18;
constexpr uint8_t SYSCALL_GETEGID           = 19;
constexpr uint8_t SYSCALL_SETEGID           = 20;
constexpr uint8_t SYSCALL_GETPID            = 21;
constexpr uint8_t SYSCALL_GETPPID           = 22;
constexpr uint8_t SYSCALL_DUP               = 23;
constexpr uint8_t SYSCALL_GET_FSTAT_FLAGS   = 24;
constexpr uint8_t SYSCALL_SET_FSTAT_FLAGS   = 25;
constexpr uint8_t SYSCALL_PIPE              = 26;
constexpr uint8_t SYSCALL_FSTAT             = 27;
constexpr uint8_t SYSCALL_STAT              = 28;
constexpr uint8_t NUM_SYSCALLS              = 29;