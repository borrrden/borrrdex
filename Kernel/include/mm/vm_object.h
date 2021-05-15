#pragma once

#include <stddef.h>
#include <stdint.h>
#include <paging.h>
#include <ref_counted.hpp>
#include <lock.hpp>
#include <move.h>

namespace mm {
    constexpr uint64_t PHYS_BLOCK_MAX = (0xffffffff << memory::PAGE_SHIFT_4K);

    class vm_object {
        friend class address_space;

    public:
        vm_object(size_t size, bool anonymous, bool shared, bool cow);
        virtual ~vm_object() = default;

        virtual int hit(uintptr_t base, uintptr_t offset, page_map_t* map);
        virtual void map_allocated_blocks(uintptr_t base, page_map_t* map) = 0;

        virtual vm_object* clone() = 0;

        ALWAYS_INLINE size_t size() const { return _size; }
        virtual size_t used_physical_memory() const { return 0; }

        ALWAYS_INLINE bool is_anonymous() const { return _anonymous; }
        ALWAYS_INLINE bool is_shared() const { return _shared; }
        ALWAYS_INLINE bool is_copy_on_write() const { return _cow; }
        ALWAYS_INLINE void set_copy_on_write(bool cow) { _cow = cow; }
        ALWAYS_INLINE int use_count() const { return _use_count; }
        ALWAYS_INLINE void add_use() { _use_count++; }
        ALWAYS_INLINE void remove_use() { _use_count--; }
    protected:
        size_t _size;
        int _use_count;     // The number of objects currently using this (not the same as ref count)

        bool _anonymous:1;
        bool _shared:1;
        bool _cow:1;
    };

    class physical_vm_object : public vm_object {
    public:
        physical_vm_object(size_t size, bool anonymous, bool shared, bool cow);
        virtual ~physical_vm_object();

        int hit(uintptr_t base, uintptr_t offset, page_map_t* map) override;
        void map_allocated_blocks(uintptr_t base, page_map_t* map) override;

        vm_object* clone() override;

        size_t used_physical_memory() const override;
    protected:
        uint32_t* _physical_blocks {nullptr};
    };

    class process_image_vm_object final : public physical_vm_object {
    public:
        process_image_vm_object(uintptr_t base, size_t size, bool write);

        void map_allocated_blocks(uintptr_t base, page_map_t* map) override;
    private:
        bool _write;
        uintptr_t _base;
    };

    class mapped_region {
    public:
        ALWAYS_INLINE mapped_region(uintptr_t base, size_t size) 
            :mapped_region(base, size, nullptr)
        {

        }

        ALWAYS_INLINE mapped_region(uintptr_t base, size_t size, kstd::ref_counted<mm::vm_object> vm_obj)
            :_base(base)
            ,_size(size)
            ,_vm_object(vm_obj)
        {

        }

        ALWAYS_INLINE mapped_region(const mapped_region& other)
            :_base(other._base)
            ,_size(other._size)
            ,_vm_object(other._vm_object)
        {

        }

        ALWAYS_INLINE mapped_region& operator=(const mapped_region& other) {
            _base = other._base;
            _size = other._size;
            _vm_object = other._vm_object;
            _lock = kstd::read_write_lock();
            return *this;
        }

        ALWAYS_INLINE mapped_region(mapped_region&& other)
            :_vm_object(std::move(other._vm_object))
            ,_lock(std::move(other._lock))
        {
            other._base = other._size = 0;
        }

        ALWAYS_INLINE mapped_region& operator=(mapped_region&& other) {
            other._base = other._size = 0;
            _vm_object = std::move(other._vm_object);
            _lock = std::move(other._lock);

            return *this;
        }

        ALWAYS_INLINE uintptr_t base() const { return _base; }
        ALWAYS_INLINE size_t size() const { return _size; }
        ALWAYS_INLINE size_t end() const { return _base + _size; }
        ALWAYS_INLINE kstd::read_write_lock& lock() { return _lock; }
        ALWAYS_INLINE kstd::ref_counted<mm::vm_object> vm_object() const { return _vm_object; }
        ALWAYS_INLINE void set_vm_object(kstd::ref_counted<mm::vm_object> obj) { _vm_object = obj; }
    private:
        kstd::read_write_lock _lock {};
        uintptr_t _base;
        size_t _size;
        kstd::ref_counted<mm::vm_object> _vm_object;
    };
}