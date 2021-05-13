#pragma once

#include <paging.h>
#include <ref_counted.hpp>
#include <klist.hpp>

#include <mm/vm_object.h>

namespace mm {
    class address_space final {
    public:
        address_space(page_map_t* pm);
        ~address_space();

        mapped_region* address_to_region(uintptr_t address);

        bool range_in_region(uintptr_t base, size_t size);

        mapped_region* map_vmo(kstd::ref_counted<vm_object> obj, uintptr_t base, bool fixed);
        mapped_region* allocate_anonymous_vmo(size_t size, uintptr_t base, bool fixed);
        mapped_region* allocate_region_at(uintptr_t base, size_t size);
        mapped_region* find_available_region(size_t size);

        long unmap_memory(uintptr_t base, size_t size);
        void unmap_all();

        size_t used_physical_mem() const;

        __attribute__((always_inline)) inline page_map_t* get_page_map() { return _page_map; }
    private:
        page_map_t* _page_map;
        list<mapped_region> _regions;
        lock_t _lock{0};
    };
}