#pragma once

#ifndef __cplusplus
#error C++ Only
#endif

#include "common.h"
#include <cstdint>

namespace madt {
    // Interrupt Control Structure Types (ACPI 6.4 p.171)
    constexpr uint8_t TYPE_PROCESSOR_LOCAL_APIC     = 0x0;
    constexpr uint8_t TYPE_IO_APIC                  = 0x1;
    constexpr uint8_t TYPE_INTERRUPT_SRC_OVERRIDE   = 0x2;
    constexpr uint8_t TYPE_NMI_SOURCE               = 0x3;
    constexpr uint8_t TYPE_LOCAL_APIC_NMI           = 0x4;
    constexpr uint8_t TYPE_LOCAL_APIC_ADDR_OVERRIDE = 0x5;
    constexpr uint8_t TYPE_IO_SAPIC                 = 0x6;
    constexpr uint8_t TYPE_LOCAL_SAPIC              = 0x7;
    constexpr uint8_t TYPE_PLAT_INTERRUPT_SRC       = 0x8;
    constexpr uint8_t TYPE_PROCESSOR_LOCAL_X2APIC   = 0x9;
    constexpr uint8_t TYPE_LOCAL_X2APIC_NMI         = 0xA;
    constexpr uint8_t TYPE_GIC_CPU_INTERFACE        = 0xB;
    constexpr uint8_t TYPE_GIC_DISTRIBUTOR          = 0xC;
    constexpr uint8_t TYPE_GIC_MSI_FRAME            = 0xD;
    constexpr uint8_t TYPE_GIC_REDISTRIBUTOR        = 0xE;
    constexpr uint8_t TYPE_GIC_INT_TRANSLATION      = 0xF;
    constexpr uint8_t TYPE_MULTIPROCESSOR_WAKEUP    = 0x10;
}

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

// This optional structure supports 64-bit systems by providing an 
// override of the physical address of the local APIC inthe MADT’s 
// table header, which is defined as a 32-bit field
// ACPI 6.4 p.176
typedef struct {
    int_controller_header_t h;
    uint16_t reserved;
    uint64_t local_apic_addr;
} __attribute__((packed)) local_apic_addr_ovr_t;

// The I/O SAPIC structure is very similar to the I/O APIC structure.  
// If both I/O APIC and I/O SAPIC structures existfor a specific APIC ID, 
// the information in the I/O SAPIC structure must be used.
// ACPI 6.4 p.177
typedef struct {
    int_controller_header_t h;
    uint8_t io_apic_id;
    uint8_t reserved;
    uint32_t global_int_base;
    uint64_t io_sapic_addr;
} __attribute__((packed)) io_sapic_t;

// The Processor local SAPIC structure is very similar to the processor local APIC structure.  
// When using the SAPICinterrupt model, each processor in the system is required 
// to have a Processor Local SAPIC record in the MADT, anda processor device object in the DSDT.
// ACPI 6.4 p.177
typedef struct {
    int_controller_header_t h;
    uint8_t acpi_proc_id;
    uint8_t local_sapic_id;
    uint8_t local_sapic_eid;
    uint8_t reserved[3];
    uint32_t flags;
    uint32_t acpi_proc_uid;
    char acpi_proc_uid_str[0];
} __attribute__((packed)) local_sapic_t;

// The Platform Interrupt Source structure is used to communicate 
// which I/O SAPIC interrupt inputs are connected to
// the platform interrupt sources.
typedef struct {
    int_controller_header_t h;
    uint16_t flags;
    uint8_t int_type;
    uint8_t proc_id;
    uint8_t proc_eid;
    uint8_t io_sapic_vector;
    uint32_t global_sys_interrupt;
    uint32_t plat_int_src_flags;
} __attribute__((packed)) plat_int_src_t;

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
    static constexpr const char* signature = "APIC";

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

class LAPIC {
public:
    LAPIC(void* memoryAddress);

    uint32_t id() const;
    void set_id(uint32_t id);

    uint32_t version() const;

    uint32_t task_priority() const;
    void set_task_priority(uint32_t priority);

    uint32_t arbitration_priority() const;

    uint32_t processor_priority() const;

    void eoi(uint32_t val);

    uint32_t remote_read() const;

    uint32_t logical_destination() const;
    void set_logical_destination(uint32_t dest);

    uint32_t destination_format() const;
    void set_destination_format(uint32_t format);

    uint32_t spurious_interrupt_vector() const;
    void set_spurious_interrupt_vector(uint32_t vector);

    // uint64_t* in_service() const;

    // uint64_t* trigger_mode() const;

    // uint64_t* interrupt_request() const;

    uint32_t error_status() const;

    uint32_t cmci() const;
    void set_cmci(uint32_t cmci);

    void interrupt_command(uint32_t* lower, uint32_t* upper) const;
    void set_interrupt_command(uint32_t lower, uint32_t upper);

    uint32_t lvt_timer() const;
    void set_lvt_timer(uint32_t val);

    uint32_t lvt_thermal_sensor() const;
    void set_lvt_thermal_sensor(uint32_t val);

    uint32_t lvt_perf_monitor() const;
    void set_lvt_perf_monitor(uint32_t val);

    uint32_t lvt_lint0() const;
    void set_lvt_lint0(uint32_t val);

    uint32_t lvt_lint1() const;
    void set_lvt_lint1(uint32_t val);

    uint32_t lvt_error() const;
    void set_lvt_error(uint32_t val);

    uint32_t initial_count() const;
    void set_initial_count(uint32_t val);

    uint32_t current_count() const;

    uint32_t divide_config() const;
    void set_divide_config(uint32_t val);

private:
    volatile uint8_t* _memoryAddress;
};