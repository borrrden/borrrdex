#pragma once

#include <stdint.h>

struct register_context {
    uint64_t r15;
    uint64_t r14;
    uint64_t r13;
    uint64_t r12;
    uint64_t r11;
    uint64_t r10;
    uint64_t r9;
    uint64_t r8;
    uint64_t rbp;
    uint64_t rdi;
    uint64_t rsi;
    uint64_t rdx;
    uint64_t rcx;
    uint64_t rbx;
    uint64_t rax;
    uint64_t rip;
    uint64_t cs;
    uint64_t rflags;
    uint64_t rsp;
    uint64_t ss;
};

typedef struct {
    uint8_t val[10];
    uint8_t rsv[6];
} mm_reg_t;

static_assert(sizeof(mm_reg_t) == 16, "Incorrect mm_reg_t size");

// Intel SDM Vol. 2A [Table 3-46.  Layout of the 64-bit-mode FXSAVE64 Map]
typedef struct {
    uint16_t fcw;           // FPU control word
    uint16_t fsw;           // FPU status word
    uint8_t ftw;            // FPU tag word
    uint8_t rsv0;    
    uint16_t fop;           // FPU opcode High 5 bits reserved
    uint64_t fip;           // FPU instruction pointer
    uint64_t fdp;           // FPU data pointer
    uint32_t mxcsr;         // MXCSR register state
    uint32_t mxcsr_mask;    // MXCSR register mask
    uint16_t mm0[5];
    uint32_t rsv1;
    mm_reg_t mm[8];
    uint64_t xmm[16];
} __attribute__((packed)) fx_state_t;

constexpr uint32_t EFLAGS_INTERRUPT_FLAG = 1 << 9;

static inline bool check_interrupts() {
    unsigned long flags;
    asm volatile (  "pushfq;"
                    "pop %%rax;"
                    : "=a"(flags) :: "cc" );
    return (flags & EFLAGS_INTERRUPT_FLAG);
}