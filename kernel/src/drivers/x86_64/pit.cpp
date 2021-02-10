#include "pit.h"
#include "arch/x86_64/pic.h"
#include "arch/x86_64/interrupt/interrupt.h"
#include "arch/x86_64/io/io.h"

#include <cstddef>

volatile uint32_t pit_counter = 0;
static timer_chain_t* s_root_chain;

extern "C" void __pit_irq_handler();

extern "C" void pit_handle() {
    timer_chain_t* cur = s_root_chain;
    while(cur) {
        cur->cb();
        cur = cur->next;
    }

    pic_eoi(PIC_IRQ_TIMER);
}

void pit_senddata(uint8_t data, uint8_t counter) {
    uint8_t port;
    switch(counter) {
        case PIT_CW_MASK_COUNTER0:
            port = PIT_COUNTER0_REG;
            break;
        case PIT_CW_MASK_COUNTER1:
            port = PIT_COUNTER1_REG;
            break;
        case PIT_CW_MASK_COUNTER2:
            port = PIT_COUNTER2_REG;
            break;
        default:
            return;
    }

    port_write_8(port, data);
}

void pit_sendcommand(uint8_t data) {
    port_write_8(PIT_COMMAND_REG, data);
}

void pit_start_counter(uint32_t frequency, uint8_t counter, uint8_t mode) {
    if(!frequency) {
        return;
    }

    uint16_t divisor = (uint16_t)(PIT_BASE_FREQUENCY / frequency);

    uint8_t cw = mode | PIT_CW_MASK_DATA | counter;
    pit_sendcommand(cw);

    pit_senddata((uint8_t)(divisor & 0xFF), counter);
    pit_senddata((uint8_t)(divisor >> 8) & 0xFF, counter);
}

extern "C" void pit_init() {
    interrupt_register(PIC_IRQ_TIMER, __pit_irq_handler);
    pit_start_counter(PIT_FREQUENCY, PIT_CW_MASK_COUNTER0, PIT_CW_MASK_RATEGEN);
}

extern "C" uint32_t get_clock() {
    return pit_counter;
}

extern "C" void __attribute_noinline__ pit_sleepms(uint64_t ms) {
    // TODO: This needs task switching!

    // interrupt_status_t int_status = interrupt_enable();

    // uint32_t clocks = get_clock();
    // uint32_t ticks = (uint32_t)(ms / (1024 / PIT_FREQUENCY)) + clocks;
    // while(true) {
    //     if(clocks > ticks) {
    //         break;
    //     }

    //     clocks = get_clock();
    //     asm volatile("hlt");
    // }

    // interrupt_set_state(int_status);
}

extern "C" void register_timer_cb(timer_chain_t* entry) {
    if(!s_root_chain) {
        s_root_chain = entry;
        return;
    }

    timer_chain_t* cur = s_root_chain;
    while(cur->next) {
        cur = cur->next;
    }

    cur->next = entry;
}

extern "C" void unregister_timer_cb(timer_chain_t* entry) {
    timer_chain_t* cur = s_root_chain;
    timer_chain_t* prev = NULL;
    while(cur->cb && cur->cb != entry->cb) {
        prev = cur;
        cur = cur->next;
    }

    if(prev) {
        prev->next = cur->next;
    } else {
        s_root_chain = NULL;
    }
}