#pragma once

typedef volatile int lock_t;

#ifdef KERNEL_DEBUG
#include <kassert.h>

#define acquire_lock(lock) ({ \
    volatile unsigned i = 0; \
    while(__sync_lock_test_and_set(lock, 1) && ++i < 0xFFFFFFF) asm("pause"); \
    if(i >= 0xFFFFFFF) { assert(!"Deadlock!"); } \
})
#else
#define acquire_lock(lock) ({while(__sync_lock_test_and_set(lock, 1)) asm("pause"); })
#endif

#define release_lock(lock) ({__sync_lock_release(lock); })

#define acquire_test_lock(lock) ({int status; status = __sync_lock_test_and_set(lock, 1); status;})