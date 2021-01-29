#pragma once

#include "common.h"

static constexpr const char* RSDP_SIGNATURE = "RSD PTR ";

// The Root System Descriptor Pointer (looked up by UEFI)
// ACPI 6.4 p.141
typedef struct {
	char signature[8];
	uint8_t checksum;
	char OEMID[6];
	uint8_t revision;
	uint32_t rsdt_address;
	uint32_t length;
	uint64_t xdst_address;
	uint8_t extended_checksum;
	uint8_t reserved[3];
} __attribute__((packed)) RSDP;

bool rsdp_valid(RSDP* rsdp);