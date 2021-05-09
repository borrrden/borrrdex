#include <io.h>

extern "C" {
    uint8_t port_read_8(uint16_t port) {
        uint8_t retVal;
        asm volatile ("inb %1, %0" : "=a"(retVal) : "dN"(port));
        return retVal;
    }

    uint16_t port_read_16(uint16_t port) {
        uint16_t retVal;
        asm volatile ("inw %1, %0" : "=a"(retVal) : "dN"(port));
        return retVal;
    }

    uint32_t port_read_32(uint16_t port) {
        uint32_t retVal;
        asm volatile ("inl %1, %0" : "=a"(retVal) : "dN"(port));
        return retVal;
    }

    void port_write_8(uint16_t port, uint8_t value) {
        asm volatile ("outb %1, %0" :: "dN"(port), "a"(value));
    }

    void port_write_16(uint16_t port, uint16_t value) {
        asm volatile ("outw %1, %0" :: "dN"(port), "a"(value));
    }

    void port_write_32(uint16_t port, uint32_t value) {
        asm volatile ("outl %1, %0" :: "dN"(port), "a"(value));
    }

    void port_read(uint16_t port, uint64_t count, uint8_t* buffer) {
        asm volatile("rep;insw"
                : "=D"(buffer), "=c"(count)
                : "D"(buffer), "c"(count), "d"((uint64_t)port)
                : "memory");
    }

    void port_write(uint16_t port, uint64_t count, uint8_t* buffer) {
        asm volatile("rep;outsw"
                : "=S"(buffer), "=c"(count)
                : "S"(buffer), "c"(count), "d"((uint64_t)port)
                : "memory");
    }
}