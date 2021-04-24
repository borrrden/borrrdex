#pragma once

#include "memory/heap.h"

template<typename T>
class SmartPointer {
public:
    typedef void(*destructor_t)(T* val);

    SmartPointer(T* val) : SmartPointer(val, kfree) {}
    SmartPointer(T* val, destructor_t);
    ~SmartPointer();

    T* get() { return _val; }

    explicit operator T*() {
        return _val;
    }
private:
    T* _val;
    destructor_t _destructor;
};