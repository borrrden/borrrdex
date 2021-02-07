#include "irq.h"
#include "Panic.h"
#include "graphics/BasicRenderer.h"

#define HALT while(true) { asm volatile("hlt"); }

extern "C" __attribute__((interrupt)) void isr_default_handler(struct interrupt_frame* frame) {
    // No-op
}

extern "C" __attribute__((interrupt)) void isr_handler0(struct interrupt_frame *) {
    Panic("Hardware exception #DE");
    HALT
}

extern "C" __attribute__((interrupt)) void isr_handler1(struct interrupt_frame *) {
    Panic("Hardware exception #DB");
    HALT
}

extern "C" __attribute__((interrupt)) void isr_handler2(struct interrupt_frame *) {
    Panic("NMI unhandled");
    HALT
}

extern "C" __attribute__((interrupt)) void isr_handler3(struct interrupt_frame *) {
    Panic("Hardware exception #BP");
    HALT
}

extern "C" __attribute__((interrupt)) void isr_handler4(struct interrupt_frame *) {
    Panic("Hardware exception #OF");
    HALT
}

extern "C" __attribute__((interrupt)) void isr_handler5(struct interrupt_frame *) {
    Panic("Hardware exception #BR");
    HALT
}

extern "C" __attribute__((interrupt)) void isr_handler6(struct interrupt_frame *){
    Panic("Hardware exception #UD");
    HALT
}

extern "C" __attribute__((interrupt)) void isr_handler7(struct interrupt_frame *){
    Panic("Hardware exception #NM");
    HALT
}

extern "C" __attribute__((interrupt)) void isr_handler8(struct interrupt_frame *, uint64_t e) {
    Panic("Hardware exception #DF");
    GlobalRenderer->Printf(" (code 0x%x [%llu])", e, e);
    HALT
}

extern "C" __attribute__((interrupt)) void isr_handler9(struct interrupt_frame *) {
    Panic("Ancient FPU based GPF, why?");
    HALT
}

extern "C" __attribute__((interrupt)) void isr_handler10(struct interrupt_frame *, uint64_t e) {
    Panic("Hardware exception #TS");
    GlobalRenderer->Printf(" (code 0x%x [%llu])", e, e);
    HALT
}

extern "C" __attribute__((interrupt)) void isr_handler11(struct interrupt_frame *, uint64_t e) {
    Panic("Hardware exception #NP");
    GlobalRenderer->Printf(" (code 0x%x [%llu])", e, e);
    HALT
}

extern "C" __attribute__((interrupt)) void isr_handler12(struct interrupt_frame *, uint64_t e) {
    Panic("Hardware exception #SS");
    GlobalRenderer->Printf(" (code 0x%x [%llu])", e, e);
    HALT
}

extern "C" __attribute__((interrupt)) void isr_handler13(struct interrupt_frame *, uint64_t e) {
    Panic("Hardware exception #GP");
    GlobalRenderer->Printf(" (code 0x%x [%llu])", e, e);
    HALT
}

extern "C" __attribute__((interrupt)) void isr_handler14(struct interrupt_frame *, uint64_t e) {
    Panic("Hardware exception #PF");
    GlobalRenderer->Printf(" (code 0x%x [%llu])", e, e);
    HALT
}

extern "C" __attribute__((interrupt)) void isr_handler15(struct interrupt_frame *) {
    Panic("Ancient FPU based GPF, why?");
    HALT
}

extern "C" __attribute__((interrupt)) void isr_handler16(struct interrupt_frame *) {
    Panic("Hardware exception #MF");
    HALT
}

extern "C" __attribute__((interrupt)) void isr_handler17(struct interrupt_frame *, uint64_t e) {
    Panic("Hardware exception #AC");
    GlobalRenderer->Printf(" (code 0x%x [%llu])", e, e);
    HALT
}

extern "C" __attribute__((interrupt)) void isr_handler18(struct interrupt_frame *) {
    Panic("Hardware exception #MC");
    HALT
}

extern "C" __attribute__((interrupt)) void isr_handler19(struct interrupt_frame *) {
    Panic("Hardware exception #XM");
    HALT
}


#undef HALT