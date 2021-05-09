#pragma once

#include <acpi/common.h>

// High Precision Event Timer
// Intel IA-PC HPET spec p.30
typedef struct {
    acpi_desc_header_t h;
    uint32_t evt_tmr_blk_id;
    acpi_generic_addr_t base_address;
    uint8_t hpet_no;
    uint16_t min_clock_tick;
    uint8_t oem_attributes:4;
    uint8_t page_protection:4;
} __attribute__((packed)) hpet_t;