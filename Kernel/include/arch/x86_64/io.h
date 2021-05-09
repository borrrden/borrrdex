#pragma once

#include <stdint.h>

extern "C" {
    uint8_t port_read_8(uint16_t port);
    uint16_t port_read_16(uint16_t port);
    uint32_t port_read_32(uint16_t port);

    void port_write_8(uint16_t port, uint8_t value);
    void port_write_16(uint16_t port, uint16_t value);
    void port_write_32(uint16_t port, uint32_t value);

    void port_read(uint16_t port, uint64_t count, uint8_t* buffer);
    void port_write(uint16_t port, uint64_t count, uint8_t* buffer);
}