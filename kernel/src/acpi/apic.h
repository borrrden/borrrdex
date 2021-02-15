#pragma once

#ifndef __cplusplus
#error C++ Only
#endif

#include "common.h"
#include <cstdint>

// The header for each entry in the MADT (see below) as described
// in ACPI 6.4 p.171
typedef struct {
    uint8_t type;
    uint8_t length;
} int_controller_header_t;

// The Processor Local APIC entries in MADT
// ACPI 6.4 p.172
typedef struct {
    int_controller_header_t h;
    uint8_t acpi_uid;
    uint8_t apic_id;
    uint32_t flags;
} __attribute__((packed)) proc_local_apic_t;

// The I/O APIC entry in MADT
// ACPI 6.4 p.173
typedef struct {
    int_controller_header_t h;
    uint8_t apic_id;
    uint8_t reserved;
    uint32_t apic_address;
    uint32_t interrupt_base;
} __attribute__((packed)) io_apic_t;

// The interrupt source override entry in MADT
// ACPI 6.4 p.174
typedef struct {
    int_controller_header_t h;
    uint8_t bus; // Always 0 (ISA)
    uint8_t source;
    uint32_t global_interrupt;
    uint16_t flags;
} __attribute__((packed)) int_source_override_t;

// The Local APIC interrupt input that each NMI is connected to
// for each of the processors that such a connection exists
// ACPI 6.4 p.176
typedef struct {
    int_controller_header_t h;
    uint8_t acpi_uid;
    uint16_t flags;
    uint8_t lint_number;
} __attribute__((packed)) local_apic_nmi_t;

// The Multiple APIC Description Table holds all
// interrupts for the interrupt model that is to be used
// by the system (hardware could support multiple, but
// only one is chosen and remains unchanged)
// ACPI 6.4 p.170
typedef struct {
    acpi_desc_header_t h;
    uint32_t lic_address;
    uint32_t flags;
    uint8_t entries[0];
} __attribute__((packed)) madt_t;

class MADT {
public:
    static constexpr const char* signature();

    MADT(void* data)
        :_data((madt_t *)data)
    {}

    size_t count() const;
    int_controller_header_t* get(size_t index) const;

    bool is_valid() const;

    const madt_t* data() const { return _data; }
private:
    madt_t* _data;
};