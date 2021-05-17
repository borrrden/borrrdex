#pragma once

#include <stdarg.h>
#include <stdint.h>

namespace log {
    void write_f(const char* __restrict format, va_list args);
    void write(const char* str, uint8_t r = 255, uint8_t g = 255, uint8_t b = 255);
    void info(const char* __restrict format, ...);
    void error(const char* __restrict format, ...);
    void warning(const char* __restrict format, ...);

    void late_initialize();
    void enable_klog();
    void disable_klog();

    #ifdef KERNEL_DEBUG
    __attribute__((always_inline)) inline static void debug(const int& var, const int lvl, const char* __restrict fmt, ...) {
        if(var >= lvl) {
            write("\r\n[DEBUG]  ");
            va_list args;
            va_start(args, fmt);
            write_f(fmt, args);
            va_end(args);
        }
    }
    #else
    __attribute__((always_inline, unused)) inline static void debug(...) {}
    #endif
}