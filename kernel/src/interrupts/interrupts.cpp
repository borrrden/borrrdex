#include "interrupts.h"
#include "../Panic.h"
#include "../cstr.h"
#include "../io/io.h"
#include "../userinput/keyboard.h"
#include "../userinput/mouse.h"
#include "../graphics/BasicRenderer.h"

#include <cstddef>

timer_chain_t* root_chain;
rtc_chain* root_rtc_chain;

void EndTimerInterrupt() {
    PIC_EndMaster();
}

__attribute__((interrupt)) void PageFault_Handler(struct interrupt_frame* frame) {
    Panic("Page Fault Detected");    
    while(true);
}

__attribute__((interrupt)) void DoubleFault_Handler(struct interrupt_frame* frame) {
    Panic("Double Fault Detected");
    while(true);
}

__attribute__((interrupt)) void GPFault_Handler(struct interrupt_frame* frame) {
    Panic("General Protection Fault Detected");
    while(true);
}

__attribute__((interrupt)) void KeyboardInt_Handler(struct interrupt_frame* frame) {
    uint8_t scancode = inb(0x60);
    HandleKeyboard(scancode);
    PIC_EndMaster();
}

__attribute__((interrupt)) void MouseInt_Handler(struct interrupt_frame* frame) {
    uint8_t mouseData = inb(0x60);
    ps2_mouse_handle(mouseData);
    PIC_EndSlave();
}

__attribute__((interrupt)) void TimerInt_Handler(struct interrupt_frame* frame) {
    timer_chain_t* cur = root_chain;
    while(cur) {
        cur->cb();
        cur = cur->next;
    }

    PIC_EndMaster();
}

__attribute__((interrupt)) void RTCInt_Handler(struct interrupt_frame* frame) {
    rtc_chain_t* cur = root_rtc_chain;
    datetime_t nextDate;
    rtc_read(&nextDate);
    while(cur) {
        cur->cb(&nextDate, cur->context);
        cur = cur->next;
    }

    outb(0x70, 0xC);
    inb(0x71);
    PIC_EndSlave();
}

void register_timer_cb(timer_chain_t* entry) {
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

void unregister_timer_cb(timer_chain_t* entry) {
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

void register_rtc_cb(rtc_chain_t* entry) {
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

void unregister_rtc_cb(rtc_chain_t* entry) {
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

void PIC_EndMaster() {
    outb(PIC1_COMMAND, PIC_EOI);
}

void PIC_EndSlave() {
    outb(PIC2_COMMAND, PIC_EOI);
    outb(PIC1_COMMAND, PIC_EOI);
}

void RemapPIC() {
    uint8_t a1, a2;

    a1 = inb(PIC1_DATA);
    io_wait();
    a2 = inb(PIC2_DATA);
    io_wait();

    outb(PIC1_COMMAND, ICW1_INIT | ICW1_ICW4);
    io_wait();
    outb(PIC2_COMMAND, ICW1_INIT | ICW1_ICW4);
    io_wait();

    outb(PIC1_DATA, 0x20);
    io_wait();
    outb(PIC2_DATA, 0x28);
    io_wait();

    outb(PIC1_DATA, 4);
    io_wait();
    outb(PIC2_DATA, 2);
    io_wait();

    outb(PIC1_DATA, ICW4_8086);
    io_wait();
    outb(PIC2_DATA, ICW4_8086);
    io_wait();

    outb(PIC1_DATA, a1);
    io_wait();
    outb(PIC2_DATA, a2);
}