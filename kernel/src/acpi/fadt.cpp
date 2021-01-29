#include "fadt.h"
#include "../Memory.h"

bool fadt_valid(FADT* fadt) {
    return memcmp(fadt->h.signature, FADT_SIGNATURE, 4) == 0
        && acpi_checksum_ok(fadt, fadt->h.length);
}