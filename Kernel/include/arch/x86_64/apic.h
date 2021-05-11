#pragma once

#include <stdint.h>

namespace apic {
    constexpr uint16_t ICR_MESSAGE_TYPE_FIXED           = 0;
    constexpr uint16_t ICR_MESSAGE_TYPE_LOW_PRIORITY    = 1 << 8;
    constexpr uint16_t ICR_MESSAGE_TYPE_SMI             = 2 << 8;
    constexpr uint16_t ICR_MESSAGE_TYPE_REMOTE_READ     = 3 << 8;
    constexpr uint16_t ICR_MESSAGE_TYPE_NMI             = 4 << 8;
    constexpr uint16_t ICR_MESSAGE_TYPE_INIT            = 5 << 8;
    constexpr uint16_t ICR_MESSAGE_TYPE_STARTUP         = 6 << 8;
    constexpr uint16_t ICR_MESSAGE_TYPE_EXTERNAL        = 7 << 8;

    constexpr uint32_t ICR_DSH_DEST     = 0; // Use destination field
    constexpr uint32_t ICR_DSH_SELF     = 1 << 18; // Send to self
    constexpr uint32_t ICR_DSH_ALL      = 2 << 18; // Send to all APIC
    constexpr uint32_t ICR_DSH_OTHER    = 3 << 18; // Send to other APIC

    namespace local {
        constexpr uint16_t REGISTER_EOI = 0xB0;

        void enable();
        void write(uint32_t off, uint32_t val);
        uint32_t read(uint32_t off);

        void send_ipi(uint8_t apic_id, uint32_t dsh, uint32_t type, uint8_t vector);
    }

    namespace io {
        void set_base(uintptr_t new_base);
    }

    int initialize();
}

extern "C" void lapic_eoi();