#include "xsdt.h"
#include "fadt.h"
#include "hpet.h"
#include "mcfg.h"
#include "../graphics/BasicRenderer.h"
#include "../Memory.h"
#include "../cstr.h"
#include "apic.h"

size_t xsdt_entry_count(XSDT* xsdt) {
    return (xsdt->h.length - sizeof(xsdt->h)) / 8;
}

void* xsdt_get_table(XSDT* xsdt, const char* signature) {
    size_t xsdtEntries = (xsdt->h.length - sizeof(xsdt->h)) / 8;
    for(size_t i = 0; i < xsdtEntries; i++) {
        ACPI_DESCRIPTION_HEADER* h = (ACPI_DESCRIPTION_HEADER *)xsdt->entries[i];
        if(memcmp(h->signature, signature, 4) == 0) {
            return h;
        }
    }

    return NULL;
}

bool xsdt_valid(XSDT* xsdt) {
    return memcmp(xsdt->h.signature, XSDT_SIGNATURE, 4) == 0
        && acpi_checksum_ok(xsdt, xsdt->h.length);
}