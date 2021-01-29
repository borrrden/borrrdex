#include "apic.h"
#include "../Memory.h"

bool madt_valid(MADT* madt) {
	return memcmp(madt->h.signature, MADT_SIGNATURE, 4) == 0
        && acpi_checksum_ok(madt, madt->h.length);
}

INTERRUPT_CONTROLLER_STRUCTURE_HEADER* madt_entry_at(MADT* madt, size_t index) {
    uint8_t* start = madt->entries;
    uint8_t* current = start;
    size_t len = madt->h.length - sizeof(madt->h);
    size_t currentIdx = 0;
    while(current - start < len) {
        INTERRUPT_CONTROLLER_STRUCTURE_HEADER* h = (INTERRUPT_CONTROLLER_STRUCTURE_HEADER *)current;
        if(currentIdx++ == index) {
            return h;
        }

        current += h->length;
    }

    return NULL;
}

size_t madt_entry_count(MADT* madt) {
    uint8_t* start = madt->entries;
    uint8_t* current = start;
    size_t len = madt->h.length - sizeof(madt->h);

    size_t total = 0;
    while(current - start < len) {
        INTERRUPT_CONTROLLER_STRUCTURE_HEADER* h = (INTERRUPT_CONTROLLER_STRUCTURE_HEADER *)current;
        total++;
        current += h->length;
    }

    return total;
}