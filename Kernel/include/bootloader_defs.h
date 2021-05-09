#pragma once

#include <stdint.h>

typedef struct {
    char brand[64];
    char version[64];

    uint64_t tags;
} __attribute__((packed)) stivale2_info_header_t;

namespace stivale2 {
    constexpr uint64_t TAG_COMMAND_LINE         = 0xe5e76a1b4597a781;
    constexpr uint64_t TAG_MEMORY_MAP           = 0x2187f79e8612de07;
    constexpr uint64_t TAG_FRAMEBUFFER_INFO     = 0x506461d2950408fa;
    constexpr uint64_t TAG_MODULES              = 0x4b6fe466aade04ce;
    constexpr uint64_t TAG_ACPI_RSDP            = 0x9e1786930a375e78;

    constexpr uint16_t MEMORY_MAP_USABLE                    = 1;
    constexpr uint16_t MEMORY_MAP_RESERVED                  = 2;
    constexpr uint16_t MEMORY_MAP_ACPI_RECLAIMABLE          = 3;
    constexpr uint16_t MEMORY_MAP_ACPI_INVS                 = 4;
    constexpr uint16_t MEMORY_MAP_BAD_MEMORY                = 5;
    constexpr uint16_t MEMORY_MAP_BOOTLOADER_RECLAIMABLE    = 0x1000;
    constexpr uint16_t MEMORY_MAP_KERNEL_OR_MODULE          = 0x1001;
}

typedef struct {
    uint64_t id;
    uint64_t next_tag;
} __attribute__((packed)) stivale2_tag_t;

typedef struct {
    uint64_t id;
    uint64_t next_tag;
    uint64_t cmdline;
} __attribute__((packed)) stivale2_tag_cmdline_t;

typedef struct {
    uint64_t base;
    uint64_t length;
    uint32_t type;
    uint32_t unused;
} __attribute__((packed)) stivale2_memory_map_entry_t;

typedef struct {
    uint64_t id;
    uint64_t next_tag;
    uint64_t entry_count;
    stivale2_memory_map_entry_t entries[];
} __attribute__((packed)) stivale2_tag_memory_map_t;

typedef struct {
    uint64_t id;
    uint64_t next_tag;
    uint64_t buffer_address;
    uint16_t buffer_width;
    uint16_t buffer_height;
    uint16_t buffer_pitch;
    uint16_t buffer_bpp;
    uint8_t memory_model;
    uint8_t red_mask_size;
    uint8_t red_mask_shift;
    uint8_t green_mask_size;
    uint8_t green_mask_shift;
    uint8_t blue_mask_size;
    uint8_t blue_mask_shift;
} __attribute__((packed)) stivale2_tag_framebuffer_info_t;

typedef struct {
    uint64_t begin;
    uint64_t end;
    char string[128];
} __attribute__((packed)) stivale2_module_t;

typedef struct {
    uint64_t id;
    uint64_t next_tag;
    uint64_t module_count;
    stivale2_module_t modules[];
} __attribute__((packed)) stivale2_tag_modules_t;

typedef struct {
    uint64_t id;
    uint64_t next_tag;
    uint64_t rsdp;
} __attribute__((packed)) stivale2_tag_rsdp_t;

typedef struct {
    uint32_t width;
    uint32_t height;
    uint16_t bpp;
    uint32_t pitch;
    
    void* address;
    uintptr_t physical_address;
    int type;
} video_mode_t;

typedef struct {
    uintptr_t total_memory;
} memory_info_t;

typedef struct {
    uintptr_t base;
    uintptr_t size;
    char name[16];
} boot_module_t;

namespace module {
    constexpr const char* SIGNATURE_FONT = "font";

    typedef struct {
        unsigned char magic[2];
        unsigned char mode;
        unsigned char char_size;
    } psf1_font_header_t;

    typedef struct {
        psf1_font_header_t header;
        void* glyphs;
    } psf1_font_t;
}