#include <acpi/acpi.h>
#include <acpi/madt.h>
#include <acpi/mcfg.h>
#include <panic.h>
#include <paging.h>
#include <kstring.h>
#include <debug.h>
#include <logging.h>
#include <lai/core.h>
#include <liballoc/liballoc.h>
#include <io.h>
#include <timer.h>
#include <kcpuid.h>
#include <apic.h>

namespace acpi {
    constexpr const char* FADT_SIGNATURE = "FACP";
    constexpr const char* DSDT_SIGNATURE = "DSDT";
    constexpr const char* MADT_SIGNATURE = "APIC";
    constexpr const char* MCFG_SIGNATURE = "MCFG";

    uint8_t processors[256];
    int processor_count = 1;

    acpi_xsdp_t* desc;
    acpi_rsdt_t* rsdt_header;
    acpi_xsdt_t* xsdt_header;
    acpi_fadt_t* fadt_header;
    mcfg_t* mcfg_header;

    list<int_source_override_t *> *isos;

    char oem[7];

    static void* find_sdt(const char* signature, int index) {
        int entries = desc->revision == 2
            ? (rsdt_header->header.length - sizeof(acpi_header_t)) / sizeof(uint64_t)
            : (rsdt_header->header.length - sizeof(acpi_header_t)) / sizeof(uint32_t);

        auto get_entry = [](unsigned index) {
            return desc->revision == 2
                ? xsdt_header->tables[index]
                : rsdt_header->tables[index];
        };

        if(memcmp(DSDT_SIGNATURE, signature, 4) == 0) {
            return (void *)memory::get_io_mapping(fadt_header->dsdt);
        }

        int cur_index = 0;
        for(int i = 0; i < entries; i++) {
            acpi_header_t *h = (acpi_header_t *)memory::get_io_mapping(get_entry(i));
            if(memcmp(h->signature, signature, 4) == 0 && cur_index++ == index) {
                return h;
            }
        }

        return nullptr;
    }

    static int read_madt() {
        void* madt = find_sdt(MADT_SIGNATURE, 0);
        if(!madt) {
            log::error("Could not find MADT");
            return 1;
        }

        uint64_t rax = 1, rbx, rcx, rdx;
        _cpuid(&rax, &rbx, &rcx, &rdx);
        uint8_t bspid = (rbx >> 24) & 0xFF;

        madt_t* madt_header = (madt_t *)madt;
        uint8_t* madt_end = reinterpret_cast<uint8_t *>(madt) + madt_header->h.length;
        uint8_t* madt_entry = madt_header->entries;
        while(madt_entry < madt_end) {
            madt_entry_t* entry = reinterpret_cast<madt_entry_t *>(madt_entry);
            switch(entry->type) {
                case madt::TYPE_PROCESSOR_LOCAL_APIC: {
                    proc_local_apic_t* local_apic = reinterpret_cast<proc_local_apic_t *>(entry);
                    if(local_apic->flags & 0x3) {
                        if(local_apic->apic_id == bspid) {
                            // BSP
                            break;
                        }

                        processors[processor_count++] = local_apic->apic_id;
                        log::debug(debug_level_acpi, debug::LEVEL_VERBOSE, "[ACPI] Found Processor, APIC ID: %d", local_apic->apic_id);
                    }

                    break;
                } case madt::TYPE_IO_APIC: {
                    io_apic_t* io_apic = reinterpret_cast<io_apic_t *>(entry);
                    log::debug(debug_level_acpi, debug::LEVEL_VERBOSE, "[ACPI] Found I/O APIC, Address: %x", io_apic->apic_address);
                    if(!io_apic->interrupt_base) {
                        apic::io::set_base(io_apic->apic_address);
                    }

                    break;
                } case madt::TYPE_INTERRUPT_SRC_OVERRIDE: {
                    int_source_override_t* iso = reinterpret_cast<int_source_override_t *>(entry);
                    isos->add(iso);
                    break;
                } case madt::TYPE_LOCAL_APIC_NMI: {
                    IF_DEBUG(debug_level_acpi >= debug::LEVEL_VERBOSE, {
                        local_apic_nmi_t* nmi = reinterpret_cast<local_apic_nmi_t *>(entry);
                        log::debug(debug_level_acpi, debug::LEVEL_VERBOSE, "[ACPI] Found NMI, LINT #%d", nmi->lint_number);
                    })

                    break;
                } case madt::TYPE_LOCAL_APIC_ADDR_OVERRIDE: {
                    break;
                } default: {
                    log::warning("Unrecognized MADT entry, type: %d", entry->type);
                    break;
                }
            }

            madt_entry += entry->length;
        }

        return 0;
    }

    void set_rsdp(acpi_xsdp_t* p) {
        desc = p;
    }

    void initialize() {
        if(!desc) {
            // TODO: Search in low memory
            const char* panicReasons[]{"System not ACPI Compliant."};
            kernel_panic(panicReasons, 1);
            __builtin_unreachable();
        }

        isos = new list<int_source_override_t *>();
        if(desc->revision == 2) {
            rsdt_header = ((acpi_rsdt_t *)memory::get_io_mapping(desc->xsdt));
            xsdt_header = ((acpi_xsdt_t *)memory::get_io_mapping(desc->xsdt));
        } else {
            rsdt_header = ((acpi_rsdt_t *)memory::get_io_mapping(desc->rsdt));
        }

        memcpy(oem, rsdt_header->header.oem, 6);
        oem[6] = 0;

        log::debug(debug_level_acpi, debug::LEVEL_NORMAL, "[ACPI] Revision: %d", desc->revision);
        fadt_header = reinterpret_cast<acpi_fadt_t *>(find_sdt(FADT_SIGNATURE, 0));

        asm("cli");

        lai_set_acpi_revision(rsdt_header->header.revision);
        lai_create_namespace();

        read_madt();
        mcfg_header = reinterpret_cast<mcfg_t *>(find_sdt(MCFG_SIGNATURE, 0));

        asm("sti");
    }

    const mcfg_t* get_mcfg() {
        return mcfg_header;
    }

    const list<int_source_override_t *>* int_source_overrides() {
        return isos;
    }

    void disable_smp() {
        processor_count = 1;
    }

    const uint8_t* get_processors() {
        return processors;
    }

    int get_proc_count() {
        return processor_count;
    }
}

extern "C" {
    void* laihost_scan(const char* signature, size_t index) {
        return acpi::find_sdt(signature, index);
    }

    void laihost_log(int level, const char* msg) {
        switch(level) {
            case LAI_WARN_LOG:
                log::warning(msg);
                break;
            default:
                log::debug(debug_level_acpi, debug::LEVEL_NORMAL, msg);
                break;
        }
    }

    void laihost_panic(const char* msg) {
        const char* panic_reasons[]{"ACPI Error:", msg};
        kernel_panic(panic_reasons, 2);
        __builtin_unreachable();
    }

    void* laihost_malloc(size_t sz) {
        return malloc(sz);
    }

    void* laihost_realloc(void* ptr, size_t new_sz, size_t old_sz) {
        return realloc(ptr, new_sz);
    }

    void laihost_free(void* addr, size_t sz) {
        free(addr);
    }

    void* laihost_map(size_t address, size_t count) {
        void* virt = memory::kernel_allocate_4k_pages(count / memory::PAGE_SIZE_4K + 1);
        memory::kernel_map_virtual_memory_4k(address, (uintptr_t)virt, count / memory::PAGE_SIZE_4K + 1);
        return virt;
    }

    void laihost_unmap(void* ptr, size_t count) {
        // TODO
    }

    void laihost_outb(uint16_t port, uint8_t val) {
        port_write_8(port, val);
    }

    void laihost_outw(uint16_t port, uint16_t val) {
        port_write_16(port, val);
    }

    void laihost_outd(uint16_t port, uint32_t val) {
        port_write_32(port, val);
    }

    uint8_t laihost_inb(uint16_t port){
        return port_read_8(port);
    }

    uint16_t laihost_inw(uint16_t port){
        return port_read_16(port);
    }

    uint32_t laihost_ind(uint16_t port){
        return port_read_32(port);
    }

    void laihost_sleep(uint64_t ms) {
        uint64_t freq = timer::get_frequency();
        uint64_t delay_ticks = (freq/1000)*ms;
        uint64_t seconds = timer::get_system_uptime();
        uint64_t ticks = timer::get_ticks();
        uint64_t total_ticks = seconds * freq + ticks;

        while(true) {
            uint64_t total_ticks_new = timer::get_system_uptime() * freq + timer::get_ticks();
            if(total_ticks_new - total_ticks >= delay_ticks) {
                break;
            }
        }
    }
}