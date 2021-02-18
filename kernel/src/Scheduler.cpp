#include "Scheduler.h"
#include "spinlock.h"
#include "config.h"
#include "thread.h"
#include "Panic.h"
#include "arch/x86_64/interrupt/interrupt.h"

extern Spinlock thread_table_spinlock;
extern thread_table_t thread_table[CONFIG_MAX_THREADS];

tid_t scheduler_current_thread[CONFIG_MAX_CPUS];

static struct {
    tid_t head;
    tid_t tail;
} ready_to_run = { -1, -1 };

static void add_to_ready_list(tid_t t) {
    KERNEL_ASSERT(t != IDLE_THREAD_TID);
    KERNEL_ASSERT(t >=0 && t < CONFIG_MAX_THREADS);

    if(ready_to_run.tail < 0) {
        /* ready queue was empty */
        ready_to_run.head = t;
        ready_to_run.tail = t;
        thread_table[t].next = -1;
    } else {
        thread_table[ready_to_run.tail].next = t;
        thread_table[t].next = -1;
        ready_to_run.tail = t;
    }
}

Scheduler::Scheduler() {
    for(int i = 0; i < CONFIG_MAX_CPUS; i++) {
        scheduler_current_thread[i] = 0;
    }
}

void Scheduler::add_ready(tid_t t) {
    WithInterrupts wi(false);

    thread_table_spinlock.acquire();
    
    add_to_ready_list(t);
    thread_table[t].state = THREAD_READY;

    thread_table_spinlock.release();
}

void Scheduler::schedule() {

}