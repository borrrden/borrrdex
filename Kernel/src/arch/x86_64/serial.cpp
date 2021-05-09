#include <serial.h>
#include <stdint.h>
#include <io.h>
#include <string.h>

constexpr uint16_t COM1 = 0x3f8;

static int is_transmit_empty() {
   return port_read_8(COM1 + 5) & 0x20;
}

int uart::init() {
    port_write_8(COM1 + 1, 0x00);
    port_write_8(COM1 + 3, 0x80);
    port_write_8(COM1, 0x03);
    port_write_8(COM1 + 1, 0x00);
    port_write_8(COM1 + 3, 0x03);
    port_write_8(COM1 + 2, 0xC7);
    port_write_8(COM1 + 4, 0x0B);

    return 0;
}

void uart::putc(char c) {
    while(is_transmit_empty() == 0) {
        // no-op
    }

    port_write_8(COM1, c);
}

void uart::print(const char* s) {
    print_n(s, strnlen(s, 1024));
}

void uart::print_n(const char* s, size_t len) {
    const char* cur = s;
    while(*s && len--) {
        putc(*s++);
    }
}