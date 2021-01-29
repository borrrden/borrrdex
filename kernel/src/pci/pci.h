#pragma once

#include <stdint.h>

static constexpr uint16_t INVALID_DEVICE = 0xFFFF;

typedef struct {
    uint16_t vendor_id;
    uint16_t device_id;
    uint16_t command;
    uint16_t status;
    uint8_t revision_id;
    uint8_t prog_interface;
    uint8_t subclass;
    uint8_t class_code;
    uint8_t cache_line_size;
    uint8_t latency_timer;
    uint8_t header_type;
    uint8_t bist;
} __attribute__((packed)) PCI_HEADER;

typedef struct
{
    PCI_HEADER common;
    uint32_t bar[6];
    uint32_t cardbus_cis_ptr;
    uint16_t subsystem_vendor_id;
    uint16_t subsystem_id;
    uint32_t expansion_rom_base;
    uint8_t capabilities_ptr;
    uint64_t reserved1:48;
    uint64_t reserved2;
    uint8_t interrupt_line;
    uint8_t interrupt_pin;
    uint8_t min_grant;
    uint8_t max_latency;
} __attribute__((packed)) PCI_DEVICE;


typedef struct {
    PCI_HEADER common;
    uint32_t bar[2];
    uint8_t primary_bus;
    uint8_t secondary_bus;
    uint8_t subordinate_bus;
    uint8_t secondary_latency_timer;
    uint8_t io_base;
    uint8_t io_limit;
    uint16_t secondary_status;
    uint16_t memory_base;
    uint16_t memory_limit;
    uint16_t prefetchable_memory_base;
    uint16_t prefetchable_memory_limit;
    uint32_t prefetchable_memory_base_upper;
    uint32_t prefetchable_memory_limit_upper;
    uint16_t io_base_upper;
    uint16_t io_limit_upper;
    uint8_t capabilities_pointer;
    uint64_t reserved:48;
    uint32_t expansion_rom_base;
    uint8_t interrupt_line;
    uint8_t interrupt_pin;
    uint16_t bridge_control;

} __attribute__((packed)) PCI_TO_PCI_BRIDGE;

PCI_HEADER* pci_get_device(void* cfgArea, uint8_t bus, uint8_t device, uint8_t function);
void pci_print_all_bar(PCI_HEADER* h);
void pci_print_bus(void* base_address, uint8_t bus);