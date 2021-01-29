#pragma once

#define PIC1_COMMAND 0x20
#define PIC1_DATA 0x21
#define PIC2_COMMAND 0xA0
#define PIC2_DATA 0xA1
#define PIC_EOI 0x20

#define ICW1_INIT 0x10
#define ICW1_ICW4 0x01
#define ICW4_8086 0x01

struct interrupt_frame;
__attribute__((interrupt)) void PageFault_Handler(struct interrupt_frame* frame);
__attribute__((interrupt)) void DoubleFault_Handler(struct interrupt_frame* frame);
__attribute__((interrupt)) void GPFault_Handler(struct interrupt_frame* frame);
__attribute__((interrupt)) void KeyboardInt_Handler(struct interrupt_frame* frame);
__attribute__((interrupt)) void TimerInt_Handler(struct interrupt_frame* frame);

typedef void(*TimerCallback)();
typedef struct timer_chain timer_chain_t;

struct timer_chain {
    TimerCallback cb;
    timer_chain_t* next;
};

void register_timer_cb(timer_chain_t* chainEntry);
void unregister_timer_cb(timer_chain_t* chainEntry);

void RemapPIC();
void PIC_EndMaster();
void PIC_EndSlave();