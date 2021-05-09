#pragma once

typedef volatile int lock_t;

#ifdef KERNEL_DEBUG
#include <kassert.h>

#define acquireLock(lock) ({ \
    volatile unsigned i = 0; \
    while(__sync_lock_test_and_set(lock, 1) && ++i < 0xFFFFFFF) asm("pause"); \
    if(i >= 0xFFFFFFF) { assert(!"Deadlock!"); } \
})
#else
#define acquireLock(lock) ({while(__sync_lock_test_and_set(lock, 1)) asm("pause"); })
#endif

#define releaseLock(lock) ({__sync_lock_release(lock); })

#define acquireTestLock(lock) ({int status; status = __sync_lock_test_and_set(lock, 1); status;})