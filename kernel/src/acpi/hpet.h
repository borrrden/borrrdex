#pragma once

#include "common.h"

constexpr const char* HPET_SIGNATURE = "HPET";

// High Precision Event Timer
// Intel IA-PC HPET spec p.30
typedef struct {
    ACPI_DESCRIPTION_HEADER h;
    uint32_t evt_tmr_blk_id;
    ACPI_GENERIC_ADDRESS base_address;
    uint8_t hpet_no;
    uint16_t min_clock_tick;
    uint8_t oem_attributes:4;
    uint8_t page_protection:4;
} __attribute__((packed)) HPET;

bool hpet_valid(HPET* hpet);