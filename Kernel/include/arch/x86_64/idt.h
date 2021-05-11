#pragma once

#include <stdint.h>
#include <system.h>

constexpr uint8_t IDT_DESC_TASK32   = 0x05; ///< 32-bit task gate
constexpr uint8_t IDT_DESC_INT16    = 0x06; ///< 16-bit interrupt gate
constexpr uint8_t IDT_DESC_TRAP16   = 0x07; ///< 16-bit trap gate
constexpr uint8_t IDT_DESC_INT32    = 0x0E; ///< 32-bit interrupt gate
constexpr uint8_t IDT_DESC_TRAP32   = 0x0F; ///< 32-bit trap gate (most common)

constexpr uint8_t IDT_DESC_RING1    = 0x40; ///< Ring 0 or 1 can use
constexpr uint8_t IDT_DESC_RING2    = 0x20; ///< Ring 0-2 can use
constexpr uint8_t IDT_DESC_RING3    = 0x60; ///< Ring 0-3 can use
constexpr uint8_t IDT_DESC_PRESENT  = 0x80; ///< Must be 1 on all valid entries

constexpr uint8_t IRQ0              = 32;

typedef struct idt_descriptor {
    uint16_t base_low;  ///< The interrupt handler's address (bits 0 - 15)
    uint16_t selector;  ///< The selector of the code segment this interrupt function is in (basically always GDT_SELECTOR_KERNEL_CODE)
    uint8_t ist;        ///< Bits 0..2 hold interrupt stack table offset, rest zero
    uint8_t flags;      ///< The flags on this entry (see IDT_DESC_*)
    uint16_t base_mid;  ///< The interrupt handler's address (bits 16 - 31)
    uint32_t base_high; ///< The interrupt handler's address (bits 32 - 63);
    uint32_t zero;      ///< Reserved
} __attribute__((packed)) idt_desc_t;

// Basically, this structure is an array of the above,
// defined as a block of memory
typedef struct id_table {
    uint16_t limit;     ///< The size in memory of all the idt_desc_t entries (laid out sequentially)
    uint64_t base;      ///< The memory address of the first idt_desc_t entry 
} __attribute__((packed)) idt_t;

typedef void(*isr_t)(void*, register_context*);

extern "C" void idt_flush();

namespace idt {
    void initialize();

    void register_interrupt_handler(uint8_t interrupt, isr_t handler, void* data = nullptr);

    void disable_pic();

    int get_err_code();
}
