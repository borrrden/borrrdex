#pragma once

#include "arch/x86_64/irq.h"
#include "arch/x86_64/io/rtc.h"

#ifdef __cplusplus
extern "C" {
#endif

    typedef struct {
        uint32_t irq;
        int_handler_t handler;
    } interrupt_entry_t;

    void interrupt_init();
    void interrupt_register(uint32_t irq, int_handler_t handler);

    typedef void(*TimerCallback)();
    typedef struct timer_chain timer_chain_t;

    struct timer_chain {
        TimerCallback cb;
        timer_chain_t* next;
    };

    void register_timer_cb(timer_chain_t* chainEntry);
    void unregister_timer_cb(timer_chain_t* chainEntry);


    typedef void(*RTCCallback)(datetime_t* dt, void* context);
    typedef struct rtc_chain rtc_chain_t;

    struct rtc_chain {
        RTCCallback cb;
        void* context;
        rtc_chain_t* next;
    };

    void register_rtc_cb(rtc_chain_t* chainEntry);
    void unregister_rtc_cb(rtc_chain_t* chainEntry);

#ifdef __cplusplus
}
#endif