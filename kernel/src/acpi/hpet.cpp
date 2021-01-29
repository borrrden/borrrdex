#include "hpet.h"
#include "../Memory.h"

bool hpet_valid(HPET* hpet) {
    return memcmp(hpet->h.signature, HPET_SIGNATURE, 4) == 0
        && acpi_checksum_ok(hpet, hpet->h.length);
}