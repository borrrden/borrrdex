#pragma once

#include <bootloader_defs.h>
#include <stddef.h>

extern void* kernel_end;

namespace memory {
    constexpr uint16_t PHYS_BLOCK_SIZE          = 4096;
    constexpr uint8_t  PHYS_BLOCK_SHIFT         = 12;
    constexpr uint8_t  PHYS_BLOCKS_PER_BYTE     = 8;
    constexpr uint32_t PHYS_BITMAP_SIZE_DWORDS  = 524488; // 64 GB

    void initialize_physical_allocator();

    uint64_t get_first_free_block();
    uint64_t get_used_blocks();
    void reset_used_blocks();

    void mark_memory_region_free(uint64_t base, size_t size);

    uint64_t allocate_physical_block();

    void free_physical_block(uint64_t addr);
}