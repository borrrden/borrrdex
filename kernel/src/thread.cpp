#include "thread.h"
#include "Panic.h"
#include "spinlock.h"
#include "config.h"
#include "arch/x86_64/interrupt/interrupt.h"

#include <cstddef>

extern "C" void __idle_thread_wait_loop();

// External for scheduler
Spinlock thread_table_spinlock;
thread_table_t thread_table[CONFIG_MAX_THREADS];

extern tid_t scheduler_current_thread[CONFIG_MAX_CPUS];

char thread_stack_areas[CONFIG_THREAD_STACKSIZE * CONFIG_MAX_THREADS];

static void thread_finish() {

}

void thread_table_init() {
    KERNEL_ASSERT(sizeof(thread_table_t) == 64);

    for(int i = 0; i < CONFIG_MAX_THREADS; i++) {
        thread_table[i].context = (ThreadContext *)(thread_stack_areas + CONFIG_THREAD_STACKSIZE * i + CONFIG_THREAD_STACKSIZE - sizeof(ThreadContext));
        thread_table[i].user_context = NULL;
        thread_table[i].state = THREAD_FREE;
        thread_table[i].sleeps_on = 0;
        thread_table[i].pageTableManager = NULL;
        thread_table[i].attribs      = 0;
        thread_table[i].next         = -1;
    }

    thread_table[IDLE_THREAD_TID].context->set_ip((uint64_t)__idle_thread_wait_loop);
    thread_table[IDLE_THREAD_TID].context->set_sp((uint64_t)thread_stack_areas + CONFIG_THREAD_STACKSIZE - 4 - sizeof(ThreadContext));
    thread_table[IDLE_THREAD_TID].context->enable_interrupts();
    thread_table[IDLE_THREAD_TID].state = THREAD_READY;
    thread_table[IDLE_THREAD_TID].context->set_prev_context(thread_table[IDLE_THREAD_TID].context);
}

tid_t thread_create(void(*func)(uint32_t), uint32_t arg) {
    static tid_t next_tid = 0;
    tid_t tid = -1;
    
    interrupt_status_t intr_status = interrupt_disable();
    thread_table_spinlock.acquire();

    for(tid_t i = 0; i < CONFIG_MAX_THREADS; i++) {
        tid_t t = (i + next_tid) % CONFIG_MAX_THREADS;
        if(t == IDLE_THREAD_TID) {
            continue;
        }

        if(thread_table[t].state == THREAD_FREE) {
            tid = t;
            break;
        }
    }

    if(tid < 0) {
        thread_table_spinlock.release();
        interrupt_set_state(intr_status);
        return tid;
    }

    next_tid = (tid + 1) % CONFIG_MAX_THREADS;
    thread_table[tid].state = THREAD_NONREADY;
    thread_table_spinlock.release();

    thread_table[tid].context = (ThreadContext *)(thread_stack_areas + CONFIG_THREAD_STACKSIZE * tid + CONFIG_THREAD_STACKSIZE - sizeof(ThreadContext));
    for(int i = 0; i < sizeof(ThreadContext) / 4; i++) {
        *(((uint64_t *)thread_table[tid].context) + i) = 0;
    }

    thread_table[tid].user_context = nullptr;
    thread_table[tid].pageTableManager = nullptr;
    thread_table[tid].sleeps_on = 0;
    thread_table[tid].attribs = 0;
    thread_table[tid].next = -1;

    thread_table[tid].context->set_prev_context(thread_table[tid].context);
    thread_table[tid].context->initialize((uint64_t)func, (uint64_t)thread_finish, (uint64_t)thread_stack_areas + (CONFIG_THREAD_STACKSIZE * tid)
                + CONFIG_THREAD_STACKSIZE - 4 - sizeof(ThreadContext), arg);

    return tid;
}

tid_t thread_get_current() {
    WithInterrupts wi(false);
    tid_t t = scheduler_current_thread[interrupt_get_cpu()];
}

thread_table_t* thread_get_thread_entry(tid_t thread) {
    return &thread_table[thread];
}

void thread_switch() {
    WithInterrupts wi(true);
    //interrupt_yield();
}

