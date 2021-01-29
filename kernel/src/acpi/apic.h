#pragma once

#include "common.h"
#include <stdint.h>

static constexpr const char* MADT_SIGNATURE = "APIC";

// The header for each entry in the MADT (see below) as described
// in ACPI 6.4 p.171
typedef struct {
    uint8_t type;
    uint8_t length;
} INTERRUPT_CONTROLLER_STRUCTURE_HEADER;

// The Processor Local APIC entries in MADT
// ACPI 6.4 p.172
typedef struct {
    INTERRUPT_CONTROLLER_STRUCTURE_HEADER h;
    uint8_t acpi_uid;
    uint8_t apic_id;
    uint32_t flags;
} __attribute__((packed)) PROCESSOR_LOCAL_APIC;

// The I/O APIC entry in MADT
// ACPI 6.4 p.173
typedef struct {
    INTERRUPT_CONTROLLER_STRUCTURE_HEADER h;
    uint8_t apic_id;
    uint8_t reserved;
    uint32_t apic_address;
    uint32_t interrupt_base;
} __attribute__((packed)) IO_APIC;

// The interrupt source override entry in MADT
// ACPI 6.4 p.174
typedef struct {
    INTERRUPT_CONTROLLER_STRUCTURE_HEADER h;
    uint8_t bus; // Always 0 (ISA)
    uint8_t source;
    uint32_t global_interrupt;
    uint16_t flags;
} __attribute__((packed)) INTERRUPT_SOURCE_OVERRIDE;

// The Local APIC interrupt input that each NMI is connected to
// for each of the processors that such a connection exists
// ACPI 6.4 p.176
typedef struct {
    INTERRUPT_CONTROLLER_STRUCTURE_HEADER h;
    uint8_t acpi_uid;
    uint16_t flags;
    uint8_t lint_number;
} __attribute__((packed)) LOCAL_APIC_NMI;

// The Multiple APIC Description Table holds all
// interrupts for the interrupt model that is to be used
// by the system (hardware could support multiple, but
// only one is chosen and remains unchanged)
// ACPI 6.4 p.170
typedef struct {
    ACPI_DESCRIPTION_HEADER h;
    uint32_t lic_address;
    uint32_t flags;
    uint8_t entries[0];
} __attribute__((packed)) MADT;

// MADT functions
size_t madt_entry_count(MADT* madt);
INTERRUPT_CONTROLLER_STRUCTURE_HEADER* madt_entry_at(MADT* madt, size_t index);
bool madt_valid(MADT* madt);