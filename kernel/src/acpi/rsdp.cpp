#include "rsdp.h"
#include "../Memory.h"

bool rsdp_valid(RSDP* rsdp) {
	return memcmp(rsdp->signature, RSDP_SIGNATURE, 8) == 0
		&& acpi_checksum_ok(rsdp, rsdp->length);
}