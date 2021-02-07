#pragma once

#include <stdint.h>

constexpr uint8_t IDT_DESC_BIT16    = 0x07;
constexpr uint8_t IDT_DESC_BIT32    = 0x0F;
constexpr uint8_t IDT_DESC_RING1    = 0x40;
constexpr uint8_t IDT_DESC_RING2    = 0x20;
constexpr uint8_t IDT_DESC_RING3    = 0x60;
constexpr uint8_t IDT_DESC_PRESENT  = 0x80;

constexpr uint8_t IDT_MAX_INTERRUPTS = 0xFF;

#ifdef __cplusplus
extern "C" {
#endif

    typedef union {
        void (*type1)(struct interrupt_frame*);
        void (*type2)(struct interrupt_frame*, uint64_t);
    } irq_handler;

    typedef struct idt_descriptor {
        uint16_t base_low;
        uint16_t selector;
        uint8_t reserved0;
        uint8_t flags;
        uint16_t base_mid;
        uint32_t base_high;
        uint32_t reserved1;
    } __attribute__((packed)) idt_desc_t;

    typedef struct id_table {
        uint16_t limit;
        uint64_t base;
    } __attribute__((packed)) idt_t;

    void idt_init();
    void idt_install_gate(uint32_t index, uint16_t flags, uint16_t selector, irq_handler irq);

#ifdef __cplusplus
}
#endif