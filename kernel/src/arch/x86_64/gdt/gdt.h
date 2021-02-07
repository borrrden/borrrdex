#pragma once

#include <stdint.h>

constexpr uint8_t GDT_MAX_DESCRIPTORS   = 16;

constexpr uint8_t GDT_DESC_ACCESS       = 0x01;
constexpr uint8_t GDT_DESC_READWRITE    = 0x02;
constexpr uint8_t GDT_DESC_EXPANSION    = 0x04;
constexpr uint8_t GDT_DESC_EXECUTABLE   = 0x08;
constexpr uint8_t GDT_DESC_CODEDATA     = 0x10;
constexpr uint8_t GDT_DESC_DPL          = 0x60;
constexpr uint8_t GDT_DESC_MEMORY       = 0x80;

constexpr uint8_t GDT_GRAN_LIMITHIMAST  = 0x0F;
constexpr uint8_t GDT_GRAN_OS           = 0x10;
constexpr uint8_t GDT_GRAN_64BIT        = 0x20;
constexpr uint8_t GDT_GRAN_32BIT        = 0x40;
constexpr uint8_t GDT_GRAN_4K           = 0x80;

constexpr uint8_t GDT_SELECTOR_KERNEL_CODE  = (0x01 << 3);
constexpr uint8_t GDT_SELECTOR_KERNEL_DATA  = (0x02 << 3);
constexpr uint8_t GDT_SELECTOR_USER_CODE    = (0x03 << 3);
constexpr uint8_t GDT_SELECTOR_USER_DATA    = (0x04 << 3);

#ifdef __cplusplus
extern "C" {
#endif

typedef struct gdt_descriptor {
    uint16_t limit;
    uint16_t base_low;
    uint8_t base_mid;
    uint8_t flags;
    uint8_t granularity;
    uint8_t base_high;
} __attribute__((packed)) gdt_desc_t;

typedef struct gdt {
    uint16_t limit;
    uint64_t base;
} __attribute__((packed)) gdt_t;

void gdt_init();
void gdt_install_descriptor(uint64_t base, uint64_t limit, uint8_t access, uint8_t granularity);

#ifdef __cplusplus
}
#endif
