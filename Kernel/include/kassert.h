#pragma once

extern "C" [[noreturn]] void kernel_assertion_failed(const char* msg, const char* file, int line);

#define assert(expr) (void)((expr) || (kernel_assertion_failed(#expr, __FILE__, __LINE__), 0))