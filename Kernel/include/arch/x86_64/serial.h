#pragma once

#include <stddef.h>

namespace uart {
    int init();
    void putc(char);
    void print(const char*);
    void print_n(const char*, size_t);
}