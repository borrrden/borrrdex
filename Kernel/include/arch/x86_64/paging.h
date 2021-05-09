#pragma once

#include <stdint.h>
#include <system.h>

namespace memory {
    constexpr uint64_t KERNEL_VIRTUAL_BASE  = 0xFFFFFFFF80000000ULL;
    constexpr uint64_t IO_VIRTUAL_BASE      = (KERNEL_VIRTUAL_BASE - 0x100000000ULL);

    constexpr uint32_t PML4_GET_INDEX(uint64_t addr) { return (addr >> 39) & 0x1FF; }
    constexpr uint32_t PDPT_GET_INDEX(uint64_t addr) { return (addr >> 30) & 0x1FF; }
    constexpr uint32_t PDE_GET_INDEX(uint64_t addr) { return (addr >> 21) & 0x1FF; }
    constexpr uint32_t PT_GET_INDEX(uint64_t addr) { return (addr >> 12) & 0x1FF; }

    constexpr uint8_t  TABLE_PRESENT        = 1;
    constexpr uint8_t  TABLE_WRITEABLE      = 1 << 1;

    constexpr uint64_t PML4_FRAME           = 0xFFFFFFFFFF000;

    constexpr uint8_t  PDPT_1G              = 1 << 7;
    constexpr uint8_t  PDPT_USER            = 1 << 2;
    constexpr uint64_t PDPT_FRAME           = 0xFFFFFFFFFF000;

    constexpr uint8_t  PDE_USER             = 1 << 2;
    constexpr uint8_t  PDE_CACHE_DISABLED   = 1 << 4;
    constexpr uint8_t  PDE_2M               = 1 << 7;
    constexpr uint64_t PDE_FRAME            = 0xFFFFFFFFFF000;
    constexpr uint16_t PDE_PAT              = 1 << 12;

    constexpr uint8_t  PAGE_USER            = 1 << 2;
    constexpr uint8_t  PAGE_WRITETHROUGH    = 1 << 3;
    constexpr uint8_t  PAGE_CACHE_DISABLED  = 1 << 4;
    constexpr uint64_t PAGE_FRAME           = 0xFFFFFFFFFF000;
    constexpr uint16_t PAGE_PAT             = 1 << 7;
    constexpr uint8_t  PAGE_PAT_WRITE_COMB  = PAGE_PAT | PAGE_CACHE_DISABLED | PAGE_WRITETHROUGH;

    constexpr uint16_t PAGE_SIZE_4K         = 0x1000;
    constexpr uint32_t PAGE_SIZE_2M         = 0x200000;
    constexpr uint32_t PAGE_SIZE_1G         = 0x40000000;
    constexpr uint64_t PDPT_SIZE            = 0x8000000000;

    constexpr uint16_t PAGES_PER_TABLE      = 512;
    constexpr uint16_t TABLES_PER_DIR       = 512;
    constexpr uint16_t DIRS_PER_PDPT        = 512;
    constexpr uint16_t PDPTS_PER_PML4       = 512;

    constexpr uint16_t MAX_PDPT_INDEX       = 511;

    constexpr uint8_t  PAGE_SHIFT_4K        = 12;
    constexpr uint32_t PAGE_COUNT_4K(uint64_t size) { return (size + PAGE_SIZE_4K - 1) >> 12; }
}

typedef uint64_t page_t;
typedef uint64_t pd_entry_t;
typedef uint64_t pdpt_entry_t;
typedef uint64_t pml4_entry_t;

typedef struct {
    uint64_t phys;
    page_t* virt;
} __attribute__((packed)) page_table_t;

using page_dir_t = pd_entry_t[memory::TABLES_PER_DIR];
using pdpt_t = pdpt_entry_t[memory::DIRS_PER_PDPT];
using pml4_t = pml4_entry_t[memory::PDPTS_PER_PML4];

typedef struct {
    pdpt_entry_t* pdpt;
    pd_entry_t** page_dirs;
    uint64_t* page_dirs_phys;
    page_t*** page_tables;
    pml4_entry_t* pml4;
    uint64_t pdpt_phys;
    uint64_t pml4_phys;
} __attribute__((packed)) page_map_t;

class address_space;
namespace memory {
    extern pml4_t kernel_pml4;

    void initialize_virtual_memory();

    bool check_kernel_pointer(uintptr_t addr, uint64_t len);

    void* kernel_allocate_4k_pages(uint64_t amount);
    void kernel_free_4k_pages(void* addr, uint64_t amount);

    void kernel_map_virtual_memory_4k(uint64_t phys, uint64_t virt, uint64_t amount, uint64_t flags = TABLE_PRESENT|TABLE_WRITEABLE);

    uint64_t virtual_to_physical_addr(uint64_t addr);

    inline void set_page_frame(uint64_t* page, uint64_t addr) {
        *page = (*page & ~PAGE_FRAME) | (addr & PAGE_FRAME);
    }

    inline uint32_t get_page_frame(uint64_t p) {
        return (p & PAGE_FRAME) >> 12;
    }

    inline void invlpg(uintptr_t addr) {
        asm("invlpg (%0)" :: "r"(addr));
    }
}

