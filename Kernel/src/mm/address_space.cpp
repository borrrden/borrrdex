#include <mm/address_space.h>

#include <debug.h>
#include <logging.h>
#include <lock.hpp>
#include <kassert.h>

namespace mm {
    address_space::address_space(page_map_t* pm) 
        :_page_map(pm)
    {
    }

    address_space::~address_space() {
        log::debug(debug_user_mm, debug::LEVEL_NORMAL, "Destroying address space with %u regions.", _regions.size());
        _regions.clear();
        memory::destroy_page_map(_page_map);
    }

    mapped_region* address_space::address_to_region(uintptr_t address) {
        kstd::lock l(_lock);
        for(mapped_region& region : _regions) {
            if(!region.vm_object()) {
                continue;
            }

            if(address >= region.base() && address <= region.end()) {
                region.lock().acquire_read();
                return &region;
            }
        }

        return nullptr;
    }

    bool address_space::range_in_region(uintptr_t base, size_t size) {
        uintptr_t end = base + size;
        for(mapped_region& region : _regions) {
            if(base < region.base()) {
                log::warning("Range (0x%llx-0x%llx) not in region (0x%llx-0x%llx)!", 
                    base, base + size, region.base(), region.end());
                return false;
            }

            if(base >= region.base() && base <= region.end()) {
                base = region.end();
            }

            if(base >= region.end()) {
                return true;
            }
        }

        return false;
    }

    mapped_region* address_space::map_vmo(kstd::ref_counted<vm_object> obj, uintptr_t base, bool fixed) {
        assert(!(obj->size() & (memory::PAGE_SIZE_4K - 1)));
        assert(!(base & (memory::PAGE_SIZE_4K - 1)));

        kstd::lock l(_lock);
        mapped_region* region;
        if(base && (region = allocate_region_at(base, obj->size()))) {
            region->set_vm_object(nullptr);
        } else if(fixed) {
            log::warning("Fixed region (0x%llx - 0x&llx) was in use, cannot overwrite a fixed mapping!", base, obj->size());
            return nullptr;
        } else {
            region = find_available_region(obj->size());
        }

        assert(region && region->base());
        region->set_vm_object(obj);
        obj->map_allocated_blocks(region->base(), _page_map);
        
        return region;
    }

    mapped_region* address_space::allocate_anonymous_vmo(size_t size, uintptr_t base, bool fixed) {
        assert(!(size & (memory::PAGE_SIZE_4K - 1)));
        assert(!(base & (memory::PAGE_SIZE_4K - 1)));

        kstd::lock l(_lock);
        mapped_region* region;
        if(base && (region = allocate_region_at(base, size))) {
            region->set_vm_object(nullptr);
        } else if(fixed) {
            log::warning("Fixed region (0x%llx - 0x&llx) was in use, cannot overwrite a fixed mapping!", base, size);
            return nullptr;
        } else {
            region = find_available_region(size);
        }

        assert(region && region->base());
        physical_vm_object* vmo = new physical_vm_object(size, true, false, false);
        region->set_vm_object(vmo);

        vmo->map_allocated_blocks(region->base(), _page_map);
        return region;
    }

    mapped_region* address_space::find_available_region(size_t size) {
        uintptr_t base = memory::PAGE_SIZE_4K;
        uintptr_t end = base + size;

        for(auto i = _regions.begin(); i != _regions.end(); i++) {
            if(i->base() >= end) {
                return &_regions.insert(mapped_region(base, size), i);
            }

            if(base >= i->base() && base < i->end()) {
                base = i->end();
                end = base + size;
            }

            if(end > i->base() && end <= i->end()) {
                base = i->end();
                end = base + size;
            }

            if(base < i->base() && end > i->end()) {
                base = i->end();
                end = base + size;
            }
        }

        if(end < memory::KERNEL_VIRTUAL_BASE) {
            return &_regions.add(mapped_region(base, size));
        }

        return nullptr;
    }

    mapped_region* address_space::allocate_region_at(uintptr_t base, size_t size) {
        uintptr_t end = base + size;

        for(auto i = _regions.begin(); i != _regions.end(); i++) {
            if(i->end() <= base) {
                continue;
            }

            if(i->base() >= end) {
                return &_regions.insert(mapped_region(base, size), i);
            }

            log::error("allocate_region_at: failed at 0x%llx - 0x%llx", i->base(), i->end());
            return nullptr;
        }

        if(!_regions.size() || _regions.get(_regions.size() - 1).end() <= base) {
            return &_regions.add(mapped_region(base, size));
        }

        return nullptr;
    }
}