#include "mcfg.h"
#include "../pci/pci.h"
#include "../graphics/BasicRenderer.h"
#include "../cstr.h"
#include "../paging/PageTableManager.h"
#include "../Memory.h"

void mcfg_print(MCFG* mcfg) {
    if(memcmp(mcfg->h.signature, MCFG_SIGNATURE, 4) != 0 || !acpi_checksum_ok(mcfg, mcfg->h.length)) {
        GlobalRenderer->Printf(" [ERROR Corrupted]");
        return;
    }

    GlobalRenderer->Next();
    GlobalRenderer->Printf("        PCI Devices:");
    GlobalRenderer->Next();
    size_t entries = mcfg_entry_count(mcfg);
    for(int i = 0; i < entries; i++) {
        MCFG_CONFIGURATION_ENTRY e = mcfg->entries[i];
        pci_print_bus((uint8_t *)e.base_address, 0);
    }
}

size_t mcfg_entry_count(MCFG* mcfg) {
    return (mcfg->h.length - (sizeof(mcfg->h) + sizeof(mcfg->reserved))) / sizeof(MCFG_CONFIGURATION_ENTRY);
}

bool mcfg_valid(MCFG* mcfg) {
    return memcmp(mcfg->h.signature, MCFG_SIGNATURE, 4) == 0
        && acpi_checksum_ok(mcfg, mcfg->h.length);
}