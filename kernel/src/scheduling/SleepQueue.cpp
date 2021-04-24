#include "SleepQueue.h"
#include "thread.h"
#include "config.h"
#include "Panic.h"
#include "arch/x86_64/spinlock.h"
#include "arch/x86_64/interrupt/interrupt.h"
#include <stdint.h>

constexpr uint8_t SLEEPQ_HASHTABLE_SIZE = 127;

extern Spinlock thread_table_spinlock;
extern thread_table_t thread_table[CONFIG_MAX_THREADS];

static Spinlock s_sleepq_spinlock;
static tid_t sleepq_hashtable[SLEEPQ_HASHTABLE_SIZE];
static bool s_initialized;

constexpr uint8_t SLEEPQ_HASH(void* resource) {
    return ((uint64_t)resource) % SLEEPQ_HASHTABLE_SIZE;
}

void sleepq_init() {
    for(int i = 0; i < SLEEPQ_HASHTABLE_SIZE; i++) {
        sleepq_hashtable[i] = -1;
    }

    s_sleepq_spinlock.reset();
}

void sleepq_add(void* resource) {
    if(interrupt_get_state() != 0) {
        return;
    }

    uint8_t hash = SLEEPQ_HASH(resource);
    tid_t my_tid = thread_get_current();
    thread_table[my_tid].next = -1;
    thread_table[my_tid].sleeps_on = (uint64_t)resource;

    KERNEL_ASSERT(my_tid != IDLE_THREAD_TID);

    s_sleepq_spinlock.acquire();
    if(sleepq_hashtable[hash] < 0) {
        sleepq_hashtable[hash] = my_tid;
    } else {
        tid_t prev = sleepq_hashtable[hash];
        while(thread_table[prev].next > 0) {
            prev = thread_table[prev].next;
        }

        thread_table[prev].next = my_tid;
    }

    s_sleepq_spinlock.release();
}

void scheduler_add_ready(tid_t t);

void sleepq_wake(void* resource) {
    uint8_t hash = SLEEPQ_HASH(resource);

    WithInterrupts wi(false);
    s_sleepq_spinlock.acquire();

    tid_t prev = -1;
    tid_t first = sleepq_hashtable[hash];
    while(first > 0 && thread_table[first].sleeps_on != (uint64_t)resource) {
        prev = first;
        first = thread_table[first].next;
    }

    if(first > 0) {
        if(prev <= 0) {
            sleepq_hashtable[hash] = thread_table[first].next;
        } else {
            thread_table[prev].next = thread_table[first].next;
        }

        thread_table_spinlock.acquire();
        thread_table[first].sleeps_on = 0;
        thread_table[first].next = -1;

        if(thread_table[first].state == THREAD_SLEEPING) {
            thread_table[first].state = THREAD_READY;
            scheduler_add_ready(first);
        }

        thread_table_spinlock.release();
    }

    s_sleepq_spinlock.release();
}

void sleepq_wake_all(void* resource) {
    uint8_t hash = SLEEPQ_HASH(resource);

    WithInterrupts wi(false);
    s_sleepq_spinlock.acquire();

    tid_t prev = -1;
    tid_t first = sleepq_hashtable[hash];
    while(first > 0) {
        while(first > 0 && thread_table[first].sleeps_on != (uint64_t)resource) {
            prev = first;
            first = thread_table[first].next;
        }

        tid_t wake;
        if(first > 0) {
            wake = first;
            if(prev <= 0) {
                first = sleepq_hashtable[hash] = thread_table[wake].next;
            } else {
                first = thread_table[prev].next = thread_table[wake].next;
            }

            thread_table_spinlock.acquire();
            thread_table[wake].sleeps_on = 0;
            thread_table[wake].next = -1;
            if(thread_table[wake].state == THREAD_SLEEPING) {
                thread_table[wake].state = THREAD_READY;
                scheduler_add_ready(wake);
            }

            thread_table_spinlock.release();
        }
    }

    s_sleepq_spinlock.release();
}