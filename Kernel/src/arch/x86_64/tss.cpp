#include <tss.h>
#include <kstring.h>
#include <paging.h>
#include <physical_allocator.h>
#include <cpu.h>

extern "C" void load_tss(uint64_t address, uint64_t gdt, uint8_t selector);

extern "C" tss_t* get_tss() {
    cpu *c = get_cpu_local();
    return &c->tss;
}

namespace tss {
    void initialize_tss(tss_t* tss, void* gdt) {
        load_tss((uintptr_t)tss, (uint64_t)gdt, 0x30);
        memset(tss, 0, sizeof(tss_t));

        tss->ist[0] = (uint64_t)memory::kernel_allocate_4k_pages(8);
        tss->ist[1] = (uint64_t)memory::kernel_allocate_4k_pages(8);
        tss->ist[2] = (uint64_t)memory::kernel_allocate_4k_pages(8);

        for(unsigned i = 0; i < 8; i++) {
            memory::kernel_map_virtual_memory_4k(memory::allocate_physical_block(), tss->ist[0] + 8 * memory::PAGE_SIZE_4K, 1);
            memory::kernel_map_virtual_memory_4k(memory::allocate_physical_block(), tss->ist[1] + 8 * memory::PAGE_SIZE_4K, 1);
            memory::kernel_map_virtual_memory_4k(memory::allocate_physical_block(), tss->ist[2] + 8 * memory::PAGE_SIZE_4K, 1);
        }

        memset((void *)tss->ist[0], 0, memory::PAGE_SIZE_4K);
        memset((void *)tss->ist[1], 0, memory::PAGE_SIZE_4K);
        memset((void *)tss->ist[2], 0, memory::PAGE_SIZE_4K);

        tss->ist[0] += 8 * memory::PAGE_SIZE_4K;
        tss->ist[1] += 8 * memory::PAGE_SIZE_4K;
        tss->ist[2] += 8 * memory::PAGE_SIZE_4K;

        asm volatile("mov %%rsp, %0" : "=r"(tss->rsp[0]));
        asm volatile("ltr %%ax" :: "a"(0x33));
    }
}