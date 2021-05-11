#pragma once

#include <stdint.h>

namespace debug {
    constexpr uint8_t LEVEL_QUIET   = 0;
    constexpr uint8_t LEVEL_NORMAL  = 1;
    constexpr uint8_t LEVEL_VERBOSE = 2;
}

#ifdef KERNEL_DEBUG
    #define IF_DEBUG(a, b) if((a)) b
#else
    #define IF_DEBUG(a, b)
#endif

extern const int debug_level_syscalls;
extern const int debug_level_hal;
extern const int debug_level_malloc;
extern const int debug_level_acpi;
extern const int debug_level_interrupts;
extern const int debug_level_misc;
extern const int debug_level_symbols;