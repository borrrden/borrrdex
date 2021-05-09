#pragma once

#include <stdint.h>

namespace io {
    // Interrupt Management Configuration Register (Intel MP 1.4 p .28)
    constexpr uint16_t CHIPSET_ADDRESS_REGISTER = 0x22;
    constexpr uint16_t CHIPSET_DATA_REGISTER    = 0x23;
    constexpr uint8_t  IMCR_REGISTER_ADDRESS    = 0x70;
    constexpr uint8_t  IMCR_8259_DIRECT         = 0x00;
    constexpr uint8_t  IMCR_VIA_APIC            = 0x01;
}

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