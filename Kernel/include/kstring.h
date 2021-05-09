#pragma once

#include <stddef.h>

extern "C" {
    void memset(void* dst, int c, size_t n);

    int strcmp(const char *a, const char *b);
    char* strncpy(char* dst, const char* src, size_t max);
    size_t strnlen(const char*, size_t);
    char* strtok(char* __restrict str, const char* __restrict delim);
    char* strtok_r(char* __restrict str, const char* __restrict delim, char** __restrict saveptr);
    char *strchr(const char *s, int c);

    char* itoa(unsigned long long num, char* str, int base);
}