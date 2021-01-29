#include "serial.h"
#include "io.h"

#include <cstdint>

#define COM1 0x3f8

int is_transmit_empty() {
   return inb(COM1 + 5) & 0x20;
}

void putc(char c) {
    while(is_transmit_empty() == 0) {
        // no-op
    }

    outb(COM1, c);
}

int print_init() {
    outb(COM1 + 1, 0x00);
    outb(COM1 + 3, 0x80);
    outb(COM1, 0x03);
    outb(COM1 + 1, 0x00);
    outb(COM1 + 1, 0x03);
    outb(COM1 + 2, 0xC7);
    outb(COM1 + 4, 0x0B);
    outb(COM1 + 4, 0x1E);
    outb(COM1, 0xAE);

    if(inb(COM1) != 0xAE) {
        return 1;
    }

    outb(COM1 + 4, 0x0F);
    return 0;
}

void print(const char* s) {
    printn(s, __SIZE_MAX__);
}

void printn(const char* s, size_t len) {
    const char* cur = s;
    while(*s++ && len--) {
        putc(*s);
    }
}