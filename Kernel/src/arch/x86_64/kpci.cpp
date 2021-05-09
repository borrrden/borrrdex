#include <kpci.h>
#include <klist.hpp>
#include <acpi/acpi.h>
#include <logging.h>
#include <paging.h>

__attribute__((always_inline)) inline bool device_exists(pci_header_t* h) {
    return h->device_id != 0xffff && h->device_id != 0;
}

namespace pci {
    const mcfg_t* mcfg;
    list<pci_header_t *>* devices;

    static void scan_bus(void* base) {
        uint64_t base_addr = (uint64_t)base;
        for(int i = 0; i < 32; i++, base_addr += 0x8000) {
            pci_header_t* h = (pci_header_t *)memory::get_io_mapping(base_addr);
            if(!device_exists(h)) {
                continue;
            }

            devices->add(h);

            if(h->header_type & 0x80) {
                for(int f = 1; f < 8; f++) {
                    pci_header_t* subh = (pci_header_t *)memory::get_io_mapping(base_addr + f * 0x1000);
                    if(!device_exists(subh)) {
                        continue;
                    }

                    if(subh->class_code == 0x6 && subh->subclass == 0x4) {
                        pci_to_pci_bridge_t* b = (pci_to_pci_bridge_t *)subh;
                        scan_bus((void *)((uint64_t)base + b->secondary_bus * 0x100000));
                    } else {
                        devices->add(subh);
                    }
                }
            }
        }
    }

    void initialize() {
        mcfg = acpi::get_mcfg();
        devices = new list<pci_header_t *>();
        if(!mcfg) {
            log::error("No support for legacy PCI");
            return;
        }

        for(int i = 0; i < (mcfg->h.length - sizeof(mcfg_t)) / sizeof(mcfg_config_entry_t); i++) {
            const mcfg_config_entry_t& base = mcfg->entries[i];
            if(base.start_bus_no == base.end_bus_no) {
                continue;
            }

            if(base.segment_group_no > 0) {
                log::warning("No support for PCI Express segment group > 0");
                continue;
            }

            uint64_t base_addr = base.base_address;
            scan_bus((void *)base_addr);
        } 
    }
}