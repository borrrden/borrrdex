#include <kassert.h>
#include <spinlock.h>
#include <system.h>
#include <stdarg.h>
#include <logging.h>
#include <debug.h>
#include <stddef.h>
#include <paging.h>
#include <physical_allocator.h>
#include <liballoc/liballoc.h>
#include <panic.h>

lock_t alloc_lock;

extern "C" {

    int liballoc_lock() {
        while(acquireTestLock(&alloc_lock)) {
            assert(check_interrupts());
        }

        return 0;
    }

    int liballoc_unlock() {
        releaseLock(&alloc_lock);
        return 0;
    }

    void liballoc_kprintf(const char* fmt, ...) {
        va_list args;
        va_start(args, fmt);
        IF_DEBUG(debug_level_malloc >= debug::LEVEL_VERBOSE, {
            log::write_f(fmt, args);
        })
        
        va_end(args);
    }

    void* liballoc_alloc(size_t pages) {
        void* addr = memory::kernel_allocate_4k_pages(pages);
        for(size_t i = 0; i < pages; i++) {
            uint64_t phys = memory::allocate_physical_block();
            memory::kernel_map_virtual_memory_4k(phys, (uint64_t)addr + i * memory::PAGE_SIZE_4K, 1);
        }

        return addr;
    }

    int liballoc_free(void* addr, size_t pages) {
        for(size_t i = 0; i < pages; i++) {
            uint64_t phys = memory::virtual_to_physical_addr((uintptr_t)addr + i * memory::PAGE_SIZE_4K);
            memory::free_physical_block(phys);
        }

        memory::kernel_free_4k_pages(addr, pages);
        return 0;
    }
}

void operator delete(void* addr) {
    free(addr);
}

void operator delete(void* address, size_t size) {
    operator delete(address);
}

void* operator new(unsigned long size) {
    return malloc(size);
}

void* operator new[](unsigned long size) {
    return malloc(size);
}

extern "C" void __cxa_pure_virtual() { 
    const char* reasons[] = { "Pure virtual function call!" };
    kernel_panic(reasons, 1);
}