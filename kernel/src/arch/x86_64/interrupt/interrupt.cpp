#include "interrupt.h"
#include "idt.h"
#include "arch/x86_64/gdt/gdt.h"
#include "userinput/keyboard.h"
#include "userinput/mouse.h"
#include "arch/x86_64/io/io.h"
#include "arch/x86_64/pic.h"
#include <cstddef>

timer_chain_t* root_chain;
rtc_chain* root_rtc_chain;

__attribute__((interrupt)) void irq_handler_keyboard(struct interrupt_frame *) {
    uint8_t scancode = port_read_8(0x60);
    HandleKeyboard(scancode);
    pic_eoi(PIC_IRQ_KEYBOARD);
}

__attribute__((interrupt)) void irq_handler_mouse(struct interrupt_frame* frame) {
    uint8_t mouseData = port_read_8(0x60);
    ps2_mouse_handle(mouseData);
    pic_eoi(PIC_IRQ_PS2MOUSE);
}

__attribute__((interrupt)) void irq_handler_timer(struct interrupt_frame* frame) {
    timer_chain_t* cur = root_chain;
    while(cur) {
        cur->cb();
        cur = cur->next;
    }

    pic_eoi(PIC_IRQ_TIMER);
}

__attribute__((interrupt)) void irq_handler_cmostimer(struct interrupt_frame* frame) {
    rtc_chain_t* cur = root_rtc_chain;
    datetime_t nextDate;
    rtc_read(&nextDate);
    while(cur) {
        cur->cb(&nextDate, cur->context);
        cur = cur->next;
    }

    port_write_8(0x70, 0xC);
    port_read_8(0x71);
    pic_eoi(PIC_IRQ_CMOSTIMER);
}

void setup_irq_defaults() {
    interrupt_register(PIC_IRQ_TIMER, irq_handler_timer);
    interrupt_register(PIC_IRQ_KEYBOARD, irq_handler_keyboard);
    interrupt_register(PIC_IRQ_CMOSTIMER, irq_handler_cmostimer);
    interrupt_register(PIC_IRQ_PS2MOUSE, irq_handler_mouse);
}

extern "C" void interrupt_init() {
    gdt_init();
    idt_init();

    setup_irq_defaults();
    pic_init(0b11111001, 0b11101110);
}

extern "C" void interrupt_register(uint32_t irq, int_handler_t handler) {
    idt_install_gate(0x20 + irq, IDT_DESC_PRESENT | IDT_DESC_BIT32, GDT_SELECTOR_KERNEL_CODE, {.type1 = handler});
}

extern "C" void register_timer_cb(timer_chain_t* entry) {
    if(!root_chain) {
        root_chain = entry;
        return;
    }

    timer_chain_t* cur = root_chain;
    while(cur->next) {
        cur = cur->next;
    }

    cur->next = entry;
}

extern "C" void unregister_timer_cb(timer_chain_t* entry) {
    timer_chain_t* cur = root_chain;
    timer_chain_t* prev = NULL;
    while(cur->cb && cur->cb != entry->cb) {
        prev = cur;
        cur = cur->next;
    }

    if(prev) {
        prev->next = cur->next;
    } else {
        root_chain = NULL;
    }
}

extern "C" void register_rtc_cb(rtc_chain_t* entry) {
    if(!root_rtc_chain) {
        root_rtc_chain = entry;
        return;
    }

    rtc_chain_t* cur = root_rtc_chain;
    while(cur->next) {
        cur = cur->next;
    }

    cur->next = entry;
}

extern "C" void unregister_rtc_cb(rtc_chain_t* entry) {
    rtc_chain_t* cur = root_rtc_chain;
    rtc_chain_t* prev = NULL;
    while(cur->cb && cur->cb != entry->cb) {
        prev = cur;
        cur = cur->next;
    }

    if(prev) {
        prev->next = cur->next;
    } else {
        root_chain = NULL;
    }
}