#pragma once

#include <spinlock.h>
#include <kmove.h>

#include <frg/list.hpp>

struct register_context;

namespace timer {
    using timer_callback_t = void(*)(void *);
    void timer_handler(void *, register_context *);
    
    class timer_event final {
    public:
        timer_event(long us, timer_callback_t callback, void* context);
        ~timer_event();

        inline long get_ticks() { return _ticks; }

        ALWAYS_INLINE void lock() { acquire_lock(&_lock); }
        ALWAYS_INLINE void unlock() { release_lock(&_lock); }

        frg::default_list_hook<timer_event> hook;
    private:
        friend void timer::timer_handler(void *, register_context *);

        long _ticks {0};
        bool _dispatched {false};
        lock_t _lock {0};

        timer_callback_t _callback;
        void* _context {nullptr};

        void dispatch();
    };
}