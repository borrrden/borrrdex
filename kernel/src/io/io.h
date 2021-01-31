#pragma once

#include <stdint.h>
#include <stdarg.h>


void outb(uint16_t port, uint8_t value);
uint8_t inb(uint16_t port);
void io_wait();

typedef void(*putc_func_t)(char, void *);

int __vprintf(putc_func_t putc, void* context, const char* fmt, va_list ap);