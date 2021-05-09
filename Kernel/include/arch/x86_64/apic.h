#pragma once

#include <stdint.h>

namespace apic {
    namespace local {
        constexpr uint16_t REGISTER_EOI = 0xB0;

        void write(uint32_t off, uint32_t val);
    }
}

extern "C" void lapic_eoi();