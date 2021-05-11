#pragma once

#include <stdint.h>
#include <gdt.h>
#include <tss.h>

struct cpu {
    cpu* self;
    uint64_t id;
    void* gdt;
    gdt_t gdt_ptr;
    tss_t tss __attribute__((aligned(16)));
};

static inline void set_cpu_local(cpu* val) {
    val->self = val;
    uintptr_t low = (uintptr_t)val & 0xFFFFFFFF;
    uintptr_t high = (uintptr_t)val >> 32;

    // For more information on this, see the SWAPGS assembly instruction
    // Intel SDM Vol. 2B   4-685
    
    // Set Kernel GS Base
    asm volatile("wrmsr" :: "a"(low), "d"(high), "c"(0xC0000102));

    // Set regular GS Base
    asm volatile("wrmsr" :: "a"(low), "d"(high), "c"(0xC0000101));
}