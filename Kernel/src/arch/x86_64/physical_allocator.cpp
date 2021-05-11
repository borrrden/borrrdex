#include <physical_allocator.h>
#include <kstring.h>
#include <kassert.h>
#include <spinlock.h>
#include <logging.h>
#include <panic.h>

namespace memory {
    uint32_t phys_mem_bitmap[PHYS_BITMAP_SIZE_DWORDS];
    uint64_t used_blocks = PHYS_BITMAP_SIZE_DWORDS * 32;
    uint64_t max_blocks = PHYS_BITMAP_SIZE_DWORDS * 32;

    uint64_t next_chunk = 1;

    lock_t allocator_lock = 0;
    
    __attribute__((always_inline)) inline void clear_bit(uint64_t bit) {
        phys_mem_bitmap[bit >> 5] &= (~ (1 << (bit & 31)));
    }

    __attribute__((always_inline)) inline void set_bit(uint64_t bit) {
        phys_mem_bitmap[bit >> 5] |= (1 << (bit & 31));
    }

    void initialize_physical_allocator() {
        memset(phys_mem_bitmap, 0xFFFFFFFF, PHYS_BITMAP_SIZE_DWORDS * sizeof(uint32_t));
        max_blocks = PHYS_BITMAP_SIZE_DWORDS;
        used_blocks = max_blocks;
    }

    uint64_t get_first_free_block() {
        if(next_chunk == 0) {
            next_chunk = 1;
        }

        auto body = [](uint32_t i) {
            if(phys_mem_bitmap[i] != 0xFFFFFFFF) {
                for(uint32_t j = 0; j < 32; j++) {
                    if(!(phys_mem_bitmap[i] & (1 << j))) {
                        next_chunk = i;

                        #ifdef KERNEL_DEBUG
                        uint64_t value = (i << 5) + j;
                        assert(value > 0);

                        return value;
                        #else
                        return (uint64_t)(i << 5) + j;
                        #endif
                    }
                }
            }

            return 0UL;
        };

        for(uint32_t i = next_chunk; i < (max_blocks >> 5); i++) {
            uint64_t found = body(i);
            if(found) {
                return found;
            }
        }

        // Uh oh, didn't find one, try again from the beginning
        for(uint32_t i = 1; i < (max_blocks >> 5); i++) {
            uint64_t found = body(i);
            if(found) {
                return found;
            }
        }

        return 0;
    }

    uint64_t get_used_blocks() {
        return used_blocks;
    }

    void reset_used_blocks() {
        used_blocks = 0;
    }

    void mark_memory_region_free(uint64_t base, size_t size) {
        uint32_t blocks_to_clear = (size + (PHYS_BLOCK_SIZE - 1)) / PHYS_BLOCK_SIZE;
        uint32_t align = base / PHYS_BLOCK_SIZE;
        while(blocks_to_clear-- > 0) {
            clear_bit(align++);
            used_blocks--;
        }
    }

    uint64_t allocate_physical_block() {
        acquireLock(&allocator_lock);

        uint64_t index = get_first_free_block();
        if(!index) {
            asm("cli");
            log::error("Out of memory");
            kernel_panic((const char**)(&"Out of memory!"),1);
            __builtin_unreachable();
        }

        set_bit(index);
        used_blocks++;
        releaseLock(&allocator_lock);
        return index << PHYS_BLOCK_SHIFT;
    }

    void free_physical_block(uint64_t addr) {
        uint64_t index = addr >> PHYS_BLOCK_SHIFT;
        assert(index);

        uint64_t chunk = index >> 5;

        phys_mem_bitmap[chunk] = phys_mem_bitmap[chunk] & (~(1U << (index & 31)));
        used_blocks--;

        if(chunk < next_chunk) {
            next_chunk = chunk;
        }
    }
}