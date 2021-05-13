#pragma once

#include <stdint.h>
#include <gdt.h>
#include <tss.h>
#include <idt.h>
#include <spinlock.h>
#include <thread.h>

#include <frg/list.hpp>

typedef struct proc process_t;

// Model specific registers
constexpr uint32_t IA32_FS_BASE         = 0xC0000100;
constexpr uint32_t IA32_GS_BASE         = 0xC0000101;
constexpr uint32_t IA32_KERNEL_GS_BASE  = 0xC0000102;

using run_queue_t = frg::intrusive_list<threading::thread, frg::locate_member<threading::thread, 
    frg::default_list_hook<threading::thread>, &threading::thread::hook>>;

struct cpu {
    cpu* self;
    uint64_t id;

    void* gdt;
    gdt_t gdt_ptr;

    threading::thread* current_thread;
    process_t* idle_process;

    lock_t run_queue_lock;
    size_t run_queue_count;
    run_queue_t run_queue;

    tss_t tss __attribute__((aligned(16)));
};

static inline void set_cpu_local(cpu* val) {
    val->self = val;
    uintptr_t low = (uintptr_t)val & 0xFFFFFFFF;
    uintptr_t high = (uintptr_t)val >> 32;

    // For more information on this, see the SWAPGS assembly instruction
    // Intel SDM Vol. 2B   4-685
    
    // Set Kernel GS Base
    asm volatile("wrmsr" :: "a"(low), "d"(high), "c"(IA32_KERNEL_GS_BASE));

    // Set regular GS Base
    asm volatile("wrmsr" :: "a"(low), "d"(high), "c"(IA32_GS_BASE));
}

__attribute__((always_inline)) static inline cpu* get_cpu_local() {
    cpu* ret;
    idt::with_interrupts wi(false);

    // liballoc requires 16 byte alignment, but that's ok this already is
    asm volatile("swapgs; movq %%gs:0, %0; swapgs;" : "=r"(ret));

    return ret;
}