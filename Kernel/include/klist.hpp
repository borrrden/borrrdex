#pragma once

#include <stddef.h>
#include <liballoc/liballoc.h>
#include <panic.h>

template<typename T>
class list {
public:
    list() 
        :_size(0)
        ,_capacity(8)
        ,_mem((T*)malloc(sizeof(T) * 8))
    {

    }

    ~list() {
        free(_mem);
    }

    void add(T item) {
        if(_size > _capacity) {
            _capacity *= 2;
            void* new_mem = realloc(_mem, _capacity);
            if(!new_mem) {
                const char* reasons[] = { "Out of memory!" };
                kernel_panic(reasons, 1);
                __builtin_unreachable();
            }

            _mem = (T*)new_mem;
        }

        _mem[_size++] = item;
    }

    void remove(size_t index) {
        for(size_t i = index; i < _size - 1; i++) {
            _mem[i] = _mem[i + 1];
        }

        _size--;
    }

    T get(size_t index) const {
        if(index >= _size) {
            const char* reasons[] = { "Out of bounds list access" };
            kernel_panic(reasons, 1);
            __builtin_unreachable();
        }

        return _mem[index];
    }

    size_t size() const { return _size; }

private:
    size_t _size;
    size_t _capacity;
    T* _mem;
};