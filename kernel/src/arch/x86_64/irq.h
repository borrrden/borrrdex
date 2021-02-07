#pragma once

#include <stdint.h>

constexpr uint32_t EFLAGS_INTERRUPT_FLAG = 1 << 9;

#ifdef __cplusplus
extern "C" {
#endif

struct interrupt_frame;
typedef void (*int_handler_t)(struct interrupt_frame *);

__attribute__((interrupt)) void isr_default_handler(struct interrupt_frame* frame);

__attribute__((interrupt)) void isr_handler0(struct interrupt_frame *);
__attribute__((interrupt)) void isr_handler1(struct interrupt_frame *);
__attribute__((interrupt)) void isr_handler2(struct interrupt_frame *);
__attribute__((interrupt)) void isr_handler3(struct interrupt_frame *);
__attribute__((interrupt)) void isr_handler4(struct interrupt_frame *);
__attribute__((interrupt)) void isr_handler5(struct interrupt_frame *);
__attribute__((interrupt)) void isr_handler6(struct interrupt_frame *);
__attribute__((interrupt)) void isr_handler7(struct interrupt_frame *);
__attribute__((interrupt)) void isr_handler8(struct interrupt_frame *, uint64_t);
__attribute__((interrupt)) void isr_handler9(struct interrupt_frame *);
__attribute__((interrupt)) void isr_handler10(struct interrupt_frame *, uint64_t);
__attribute__((interrupt)) void isr_handler11(struct interrupt_frame *, uint64_t);
__attribute__((interrupt)) void isr_handler12(struct interrupt_frame *, uint64_t);
__attribute__((interrupt)) void isr_handler13(struct interrupt_frame *, uint64_t);
__attribute__((interrupt)) void isr_handler14(struct interrupt_frame *, uint64_t);
__attribute__((interrupt)) void isr_handler15(struct interrupt_frame *);
__attribute__((interrupt)) void isr_handler16(struct interrupt_frame *);
__attribute__((interrupt)) void isr_handler17(struct interrupt_frame *, uint64_t);
__attribute__((interrupt)) void isr_handler18(struct interrupt_frame *);
__attribute__((interrupt)) void isr_handler19(struct interrupt_frame *);

#ifdef __cplusplus
}
#endif