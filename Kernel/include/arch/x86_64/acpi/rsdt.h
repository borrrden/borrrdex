#pragma once

#include <acpi/common.h>

// The Root System Descriptor Table, which holds the addreses
// of all other system descriptor tables on the system
// ACPI 6.4 p.145
typedef struct {
    acpi_desc_header_t h;
    uint32_t entries[0];
} __attribute__((packed)) rsdt_t;