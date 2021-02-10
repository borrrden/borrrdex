#include "interrupt.h"
#include "idt.h"
#include "arch/x86_64/gdt/gdt.h"
#include "arch/x86_64/io/io.h"
#include "arch/x86_64/pic.h"
#include <cstddef>

extern "C" void __enable_irq();
extern "C" void __disable_irq();
extern "C" uint64_t __getflags();

extern "C" void interrupt_init() {
    gdt_init();
    idt_init();

    pic_init(0b11111111, 0b11111111);
}

extern "C" void interrupt_register(uint32_t irq, int_handler_t handler) {
    idt_install_gate(0x20 + irq, IDT_DESC_PRESENT | IDT_DESC_TRAP32, GDT_SELECTOR_KERNEL_CODE, handler);
    pic_unmask_interrupt(irq);
    if(irq >= 8) {
        pic_unmask_interrupt(2);
    }
}

extern "C" interrupt_status_t interrupt_enable() {
    interrupt_status_t current = interrupt_get_state();
    __enable_irq();
    return current;
}

extern "C" interrupt_status_t interrupt_disable() {
    interrupt_status_t current = interrupt_get_state();
    __disable_irq();
    return current;
}

extern "C" interrupt_status_t interrupt_set_state(interrupt_status_t state) {
    interrupt_status_t current = interrupt_get_state();
    if(state != 0) {
        interrupt_enable();
    } else {
        interrupt_disable();
    }

    return current;
}

extern "C" interrupt_status_t interrupt_get_state() {
    interrupt_status_t status = (interrupt_status_t)__getflags();
    if(status & EFLAGS_INTERRUPT_FLAG) {
        return 1;
    }

    return 0;
}
