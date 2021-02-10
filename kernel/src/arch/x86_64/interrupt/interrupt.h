#pragma once

#include "arch/x86_64/irq.h"
#include "arch/x86_64/io/rtc.h"

typedef uint32_t interrupt_status_t;

#ifdef __cplusplus
extern "C" {
#endif

    /**
     * Initializes PIC based interrupts (as part of the process, sets up GDT and IDT).
     * Do not call this more than once.
     */
    void interrupt_init();

    /**
     * Registers a handler for a PIC based interrupt
     * @param irq       The IRQ number to use with the callback
     * @param handler   The callback to use
     */
    void interrupt_register(uint32_t irq, int_handler_t handler);
    
    interrupt_status_t interrupt_enable();
    interrupt_status_t interrupt_disable();
    interrupt_status_t interrupt_set_state(interrupt_status_t state);
    interrupt_status_t interrupt_get_state();

#ifdef __cplusplus
}
#endif