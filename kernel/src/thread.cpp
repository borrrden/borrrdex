#include "thread.h"
#include "Panic.h"
#include "spinlock.h"
#include "config.h"

#include <cstddef>

extern "C" void __idle_thread_wait_loop();

Spinlock thread_table_spinlock;

thread_table_t thread_table[CONFIG_MAX_THREADS];

char thread_stack_areas[CONFIG_THREAD_STACKSIZE * CONFIG_MAX_THREADS];

void thread_table_init() {
    KERNEL_ASSERT(sizeof(thread_table_t) == 64);

    thread_table_spinlock.release();
    for(int i = 0; i < CONFIG_MAX_THREADS; i++) {
        thread_table[i].context = (ThreadContext *)(thread_stack_areas + CONFIG_THREAD_STACKSIZE * i + CONFIG_THREAD_STACKSIZE - sizeof(ThreadContext));
        thread_table[i].user_context = NULL;
        thread_table[i].state = THREAD_FREE;
        thread_table[i].sleeps_on = 0;
        thread_table[i].pagetable = NULL;
        thread_table[i].attribs      = 0;
        thread_table[i].next         = -1;
    }

    thread_table[IDLE_THREAD_TID].context->set_ip((uint64_t)__idle_thread_wait_loop);
    thread_table[IDLE_THREAD_TID].context->set_sp((uint64_t)thread_stack_areas + CONFIG_THREAD_STACKSIZE - 4 - sizeof(ThreadContext));
    thread_table[IDLE_THREAD_TID].context->enable_interrupts();
}