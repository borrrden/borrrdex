#pragma once

#include "common.h"

static constexpr const char* MCFG_SIGNATURE = "MCFG";

// The data contained in each entry in the MCFG table
// PCI Firmware Specification 3.0 p.43
typedef struct {
    uint64_t base_address;
    uint16_t segment_group_no;
    uint8_t start_bus_no;
    uint8_t end_bus_no;
    uint32_t reserved;
} __attribute__((packed)) MCFG_CONFIGURATION_ENTRY;

// The MCFG table is an ACPI table that is used to communicate the base addresses
// corresponding to the non-hot removable PCI Segment Groups range within a 
// PCI Segment Group available to the operating system at boot. 
// PCI Firmware Specification 3.0 p.41
typedef struct {
    ACPI_DESCRIPTION_HEADER h;
    uint64_t reserved;
    MCFG_CONFIGURATION_ENTRY entries[0];
} __attribute__((packed)) MCFG;

size_t mcfg_entry_count(MCFG* mcfg);
bool mcfg_valid(MCFG* mcfg);
void mcfg_print(MCFG* mcfg);