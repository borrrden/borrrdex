#pragma once

#include <acpi/common.h>

// The data contained in each entry in the MCFG table
// PCI Firmware Specification 3.0 p.43
typedef struct {
    uint64_t base_address;
    uint16_t segment_group_no;
    uint8_t start_bus_no;
    uint8_t end_bus_no;
    uint32_t reserved;
} __attribute__((packed)) mcfg_config_entry_t;

// The MCFG table is an ACPI table that is used to communicate the base addresses
// corresponding to the non-hot removable PCI Segment Groups range within a 
// PCI Segment Group available to the operating system at boot. 
// PCI Firmware Specification 3.0 p.41
typedef struct {
    acpi_desc_header_t h;
    uint64_t reserved;
    mcfg_config_entry_t entries[0];
} __attribute__((packed)) mcfg_t;