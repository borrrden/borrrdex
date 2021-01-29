#pragma once

#include "common.h"
#include <stdint.h>

static constexpr const char* XSDT_SIGNATURE = "XSDT";

// The Extended System Descriptor Table, which holds the addreses
// of all other system descriptor tables on the system (Root
// System Descriptor Table also exists, for 32-bit implementations)
// ACPI 6.4 p.146
typedef struct {
	ACPI_DESCRIPTION_HEADER h;
	void* entries[0];
} __attribute__((packed)) XSDT;

bool xsdt_valid(XSDT* xsdt);
size_t xsdt_entry_count(XSDT* xsdt);
void* xsdt_get_table_at(XSDT* xsdt, size_t index);
void* xsdt_get_table(XSDT* xsdt, const char* signature);