#include "spinlock.h"

extern "C" void __spinlock_acquire(int* handle);

void Spinlock::acquire() {
    __spinlock_acquire(&_handle);
}

void Spinlock::release() {
    _handle = 0;
}