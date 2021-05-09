#pragma once

#include <stdint.h>

namespace apic {
    namespace local {
        constexpr uint16_t REGISTER_EOI = 0xB0;

        void enable();
        void write(uint32_t off, uint32_t val);
        uint32_t read(uint32_t off);
    }

    namespace io {
        void set_base(uintptr_t new_base);
    }

    int initialize();
}

extern "C" void lapic_eoi();