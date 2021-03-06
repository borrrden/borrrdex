#include <mm/vm_object.h>
#include <kassert.h>
#include <paging.h>
#include <physical_allocator.h>
#include <kstring.h>
#include <cpu.h>

namespace mm {
    vm_object::vm_object(size_t size, bool anonymous, bool shared, bool cow)
        :_size(size)
        ,_anonymous(anonymous)
        ,_shared(shared)
        ,_cow(cow)
    {
        assert(!(size & (memory::PAGE_SIZE_4K - 1)));
    }

    int vm_object::hit(uintptr_t, uintptr_t, page_map_t*) {
        return 1; // Fatal page fault, kill process
    }

    physical_vm_object::physical_vm_object(size_t size, bool anonymous, bool shared, bool cow)
        :vm_object(size, anonymous, shared, cow)
    {
        size_t block_count = memory::PAGE_COUNT_4K(size);
        _physical_blocks = new uint32_t[block_count];

        if(anonymous) {
            memset(_physical_blocks, 0, sizeof(uint32_t) * block_count);
        } else {
            void* mapping = memory::kernel_allocate_4k_pages(1);
            for(unsigned i = 0; i < block_count; i++) {
                uintptr_t phys = memory::allocate_physical_block();
                _physical_blocks[i] = phys >> memory::PAGE_SHIFT_4K;
                memory::kernel_map_virtual_memory_4k(phys, (uintptr_t)mapping, 1);
                memset(mapping, 0, memory::PAGE_SIZE_4K);
            }

            memory::kernel_free_4k_pages(mapping, 1);
        }
    }

    void physical_vm_object::map_allocated_blocks(uintptr_t base, page_map_t* map) {
        uintptr_t virt = base;
        for(unsigned i = 0; i < (_size >> memory::PAGE_SHIFT_4K); i++) {
            uint64_t block = _physical_blocks[i];
            if(block) {
                uint32_t flags = memory::PAGE_USER | memory::TABLE_PRESENT;
                if(!_cow) {
                    flags |= memory::TABLE_WRITEABLE;
                }

                memory::map_virtual_memory_4k(block << memory::PAGE_SHIFT_4K, virt, 1, map, flags);
            } else {
                // Not allocated, no write allowed
                memory::map_virtual_memory_4k(0, virt, 1, map, memory::PAGE_USER);
            }

            virt += memory::PAGE_SIZE_4K;
        }
    }

    vm_object* physical_vm_object::clone() {
        assert(!_shared);
        physical_vm_object* new_vmo = new physical_vm_object(_size, _anonymous, _shared, false);

        // Temporary mapping to make our copy
        uint8_t* virt_buffer = (uint8_t *)memory::kernel_allocate_4k_pages(2);
        uint8_t* virt_dest_buffer = virt_buffer + memory::PAGE_SIZE_4K;

        for(unsigned i = 0; i < (_size >> memory::PAGE_SHIFT_4K); i++) {
            uintptr_t block = _physical_blocks[i];
            if(block) {
                uintptr_t new_block = memory::allocate_physical_block();
                new_vmo->_physical_blocks[i] = new_block >> memory::PAGE_SHIFT_4K;

                memory::kernel_map_virtual_memory_4k(block << memory::PAGE_SHIFT_4K, (uintptr_t)virt_buffer, 1);
                memory::kernel_map_virtual_memory_4k(new_block, (uintptr_t)virt_dest_buffer, 1);
                memcpy(virt_dest_buffer, virt_buffer, memory::PAGE_SIZE_4K);
            }
        }

        memory::kernel_free_4k_pages(virt_buffer, 2);

        new_vmo->add_use();

        return new_vmo;
    }

    size_t physical_vm_object::used_physical_memory() const {
        if(!_anonymous) {
            return _size;
        }

        unsigned block_count = 0;
        for(unsigned i = 0; i < _size >> memory::PAGE_SHIFT_4K; i++) {
            if(_physical_blocks[i]) {
                block_count++;
            }
        }

        return block_count << memory::PAGE_SHIFT_4K;
    }

    physical_vm_object::~physical_vm_object() {
        assert(_use_count <= 1);
        
        if(_physical_blocks) {
            for(unsigned i = 0; i < _size >> memory::PAGE_SHIFT_4K; i++) {
                if(_physical_blocks[i]) {
                    memory::free_physical_block((uintptr_t)_physical_blocks[i] << memory::PAGE_SHIFT_4K);
                }
            }

            delete[] _physical_blocks;
        }
    }

    int physical_vm_object::hit(uintptr_t base, uintptr_t offset, page_map_t* map) {
        unsigned block_index = offset >> memory::PAGE_SHIFT_4K;
        assert(block_index < (_size >> memory::PAGE_SHIFT_4K));

        uint32_t& block = _physical_blocks[block_index];
        if(block) {
            // Already allocated by another object, just map for this one too
            memory::map_virtual_memory_4k((uintptr_t)block << memory::PAGE_SHIFT_4K, base + offset, 1, map);
        } else {
            // Allocate the physical memory as well
            assert(_anonymous);

            uintptr_t phys = memory::allocate_physical_block();
            assert(phys < PHYS_BLOCK_MAX);

            if(!phys) {
                // Failed to allocate (out of memory?)
                return 1;
            }

            block = phys >> memory::PAGE_SHIFT_4K;
            memory::map_virtual_memory_4k(phys, base + offset, 1, map);
            if(get_cr3() == map->pml4_phys) {
                // PML4 is already correctly loaded, zero the block directly
                memset((void *)((base + offset) & ~(memory::PAGE_SIZE_4K - 1)), 0, memory::PAGE_SIZE_4K);
            } else {
                // Create temporary mapping to set the physical memory to zero
                void* mapping = memory::kernel_allocate_4k_pages(1);
                memory::kernel_map_virtual_memory_4k(phys, (uintptr_t)mapping, 1);
                memset(mapping, 0, memory::PAGE_SIZE_4K);
                memory::kernel_free_4k_pages(mapping, 1);
            }
        }

        return 0;
    }

    process_image_vm_object::process_image_vm_object(uintptr_t base, size_t size, bool write)
        :physical_vm_object(size, false, false, false)
        ,_write(write)
        ,_base(base)
    {

    }

    void process_image_vm_object::map_allocated_blocks(uintptr_t requested_base, page_map_t* map) {
        assert(requested_base == _base);

        uintptr_t virt = _base;
        for(unsigned i = 0; i < (_size >> memory::PAGE_SHIFT_4K); i++) {
            uint64_t block = _physical_blocks[i];
            assert(block);

            uint64_t flags = memory::PAGE_USER | memory::TABLE_PRESENT;
            if(_write && !_cow) {
                flags |= memory::TABLE_WRITEABLE;
            }

            memory::map_virtual_memory_4k(block << memory::PAGE_SHIFT_4K, virt, 1, map, flags);
            virt += memory::PAGE_SIZE_4K;
        }
    }
}