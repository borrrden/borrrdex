#pragma once

#include <stddef.h>
#include <sys/cdefs.h>

__BEGIN_DECLS

void* memcpy(void* __restrict, const void* __restrict, size_t);
int memcmp(const void*, const void*, size_t) __attribute_pure__;
void memset(void*, int, size_t);

size_t strnlen(const char*, size_t) __attribute_pure__;

__END_DECLS