#include <apic.h>

namespace apic {
    namespace local {
        uintptr_t base;
        volatile uintptr_t virtual_base;

        void write(uint32_t off, uint32_t val) {
            *((volatile uint32_t *)(virtual_base + off)) = val;
        }
    }
}

extern "C" void lapic_eoi() {
    apic::local::write(apic::local::REGISTER_EOI, 0);
}