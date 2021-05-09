#include <apic.h>
#include <kcpuid.h>
#include <logging.h>
#include <idt.h>
#include <paging.h>
#include <debug.h>
#include <acpi/acpi.h>

namespace apic {
    namespace local {
        constexpr uint64_t LOCAL_APIC_SIVR = 0xF0; // Spurious Interrupt Vector Register

        constexpr uint64_t LOCAL_APIC_BASE = 0xFFFFFFFFFF000;

        uintptr_t base;
        volatile uintptr_t virtual_base;

        static uint64_t read_base() {
            uint64_t low, high;
            asm("rdmsr" : "=a"(low), "=d"(high) : "c"(0x1B));
            return (high << 32) | low;
        }

        static void write_base(uint64_t val) {
            uint64_t low = val & 0xFFFFFFFF;
            uint64_t high = val >> 32;
            asm("wrmsr" :: "a"(low), "d"(high), "c"(0x1B));
        }

        static void spurious_handler(void*, register_context*) {
            log::warning("[APIC] Spurious interrupt");
        }

        void enable() {
            write_base(read_base() | (1UL << 11));
            write(LOCAL_APIC_SIVR, read(LOCAL_APIC_SIVR) | 0x1FF); // Spurious interrupt vector to 0xFF
        }

        void write(uint32_t off, uint32_t val) {
            *((volatile uint32_t *)(virtual_base + off)) = val;
        }

        uint32_t read(uint32_t off) {
            return *((volatile uint32_t *)(virtual_base + off));
        }

        int initialize() {
            base = read_base() & LOCAL_APIC_BASE;
            virtual_base = memory::get_io_mapping(base);
            log::debug(debug_level_interrupts, debug::LEVEL_NORMAL, "[APIC] Local APIC Base 0x%x (0x%x)", base, virtual_base);
            idt::register_interrupt_handler(0xFF, spurious_handler);

            enable();

            return 0;
        }
    }

    namespace io {
        constexpr uint8_t IO_APIC_REGSEL        = 0x00; // I/O APIC Register Select Address Offset
        constexpr uint8_t IO_APIC_WIN           = 0x10; // I/O APIC I/O Window Address offset

        constexpr uint8_t IO_APIC_REGISTER_ID   = 0x00; // ID Register
        constexpr uint8_t IO_APIC_REGISTER_VER  = 0x01; // Version Register

        constexpr uint32_t ICR_MESSAGE_TYPE_LOW_PRIORITY = 1 << 8;

        uintptr_t base = 0;
        uintptr_t virtual_base;

        volatile uint32_t* register_select;
        volatile uint32_t* io_window;

        uint32_t interrupts;
        uint32_t apic_id;

        constexpr uint32_t IO_APIC_RED_TABLE_ENT(uint8_t input) {
            return 0x10 + 2 * input;
        }

        void set_base(uintptr_t new_base) {
            base = new_base;
        }

        uint32_t read32(uint32_t reg) {
            *register_select = reg;
            return *io_window;
        }

        void write32(uint32_t reg, uint32_t val) {
            *register_select = reg;
            *io_window = val;
        }

        void write64(uint32_t reg, uint64_t val) {
            uint32_t low = val & 0xFFFFFFFF;
            uint32_t high = val >> 32;

            write32(reg, low);
            write32(reg + 1, high);
        }

        void redirect(uint8_t irq, uint8_t vector, uint32_t delivery) {
            write64(IO_APIC_RED_TABLE_ENT(irq), delivery | vector);
        }

        int initialize() {
            if(!base) {
                log::error("[APIC] Attempted to initialize I/O APIC without setting base");
                return 1;
            }

            virtual_base = memory::get_io_mapping(base);
            register_select = (volatile uint32_t *)(virtual_base + IO_APIC_REGSEL);
            io_window = (volatile uint32_t *)(virtual_base + IO_APIC_WIN);

            interrupts = read32(IO_APIC_REGISTER_VER) >> 16;
            apic_id = read32(IO_APIC_REGISTER_ID) >> 24;

            log::debug(debug_level_interrupts, debug::LEVEL_NORMAL, "[APIC] I/O APIC Base 0x%x (0x%x), Available Interrupts: %d, ID: %d ",
                base, virtual_base, interrupts, apic_id);
            
            const auto* isos = acpi::int_source_overrides();
            for(unsigned i = 0; i < isos->size(); i++) {
                int_source_override_t* iso = isos->get(i);
                log::debug(debug_level_interrupts, debug::LEVEL_VERBOSE, "[APIC] Interrupt Source Override, IRQ: %d, GSI: %d",
                    iso->source, iso->global_interrupt);
                redirect(iso->global_interrupt, iso->source + 0x20, ICR_MESSAGE_TYPE_LOW_PRIORITY);
            }

            return 0;
        }
    }

    int initialize() {
        CPUIDFeatures cpuid;
        if(!cpuid.APIC()) {
            log::error("APIC not present");
            return 1;
        }

        asm("cli");

        idt::disable_pic();

        int io_result = io::initialize();
        int local_result = local::initialize();

        asm("sti");

        if(io_result != 0) {
            return io_result;
        }

        return local_result ? local_result : 0;
    }
}

extern "C" void lapic_eoi() {
    apic::local::write(apic::local::REGISTER_EOI, 0);
}