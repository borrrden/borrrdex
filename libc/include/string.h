#pragma once

#include "__config.h"
#include "stddef.h"

__BEGIN_DECLS

void* memcpy(void*, const void*, size_t);
int memcmp(const void*, const void*, size_t) __attribute__((pure));
void memset(void*, int, size_t);

size_t strnlen(const char*, size_t) __attribute__((pure));
char *strncpy (char *__restrict, const char *__restrict, size_t);
int strncmp (const char *, const char *, size_t);

__END_DECLS