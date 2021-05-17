#include <timer.h>
#include <timer_event.h>
#include <kassert.h>
#include <idt.h>
#include <io.h>
#include <scheduler.h>

namespace timer {
    int frequency;
    long ticks = 0;
    long pending_ticks = 0;
    long long uptime = 0;

    // In the sleep queue, all waiting threads have a counter as an offset from the previous waiting thread.
    // For example there are 2 threads, thread 1 is waiting for 10 ticks and thread 2 is waiting for 15 ticks.
    // Thread 1's ticks value will be 10, and as 15 - 10 is 5, thread 2's value will be 5 so it waits 5 ticks after thread 1
    using sleep_list_t = frg::intrusive_list<timer_event, frg::locate_member<timer_event, frg::default_list_hook<timer_event>, &timer_event::hook>>;
    sleep_list_t sleeping;
    lock_t sleep_list_lock = 0;

    void timer_handler(void*, register_context* regs) {
        ticks++;
        if(ticks >= frequency) {
            uptime++;
            ticks -= frequency;
        }

        pending_ticks++;
        if(acquire_test_lock(&sleep_list_lock) == 0) {
            while(!sleeping.empty() && pending_ticks-- > 0) {
                timer_event* evt = sleeping.front();
                assert(evt);
                evt->_ticks--;
                while(evt && evt->_ticks <= 0) {
                    evt->dispatch();
                    evt = sleeping.front();
                }
            }

            pending_ticks = 0;
            release_lock(&sleep_list_lock);
        }

        scheduler::tick(regs);
    }

    uint64_t get_system_uptime(timeval* t) {
        if(t) {
            t->tv_sec = uptime;
            t->tv_usec = ticks * 1000000 / frequency;
        }

        return uptime;
    }

    uint32_t get_ticks() {
        return ticks;
    }

    uint32_t get_frequency() {
        return frequency;
    }

    static inline uint64_t us_to_ticks(long us) {
        return us * frequency / 1000000;
    }

    long time_difference(const timeval& old_time, const timeval& new_time) {
        long seconds = new_time.tv_sec - old_time.tv_sec;
        int microseconds = new_time.tv_usec - old_time.tv_usec;

        return seconds * 1000000 + microseconds;
    }

    void wait(long ms) {
        assert(ms > 0);

        uint64_t start = uptime * 1000 + (ticks * frequency / 1000);
        while((uptime * 1000 + (ticks * frequency / 1000)) - start <= static_cast<unsigned long>(ms)) {
            // Spin
        }
    }

    void initialize(uint32_t freq) {
        idt::register_interrupt_handler(IRQ0, timer_handler);

        frequency = freq;
        uint32_t divisor = PIT_BASE_FREQUENCY / freq;
        port_write_8(PIT_COMMAND_REG, PIT_CW_MASK_DATA | PIT_CW_MASK_SQRRATEGEN);

        uint8_t l = divisor & 0xff;
        uint8_t h = divisor >> 8;
        port_write_8(PIT_COUNTER0_REG, l);
        port_write_8(PIT_COUNTER0_REG, h);
    }

    timer_event::timer_event(long us, timer_callback_t callback, void* context)
        :_ticks(us * frequency / 1000000)
        ,_callback(callback)
        ,_context(context)
    {
        if(ticks <= 0) {
            callback(context);
            return;
        }

        acquire_lock(&sleep_list_lock);

        timer_event* ev = sleeping.front();
        while(ev) {
            if(ev->_ticks >= _ticks) {
                ev->_ticks -= _ticks;
                sleeping.insert(sleeping.iterator_to(ev), this);
                release_lock(&sleep_list_lock);
                return;
            }

            _ticks -= ev->_ticks;
            ev = ev->hook.next;
        }

        sleeping.push_back(this);
        release_lock(&sleep_list_lock);
    }

    timer_event::~timer_event() {
        acquire_lock(&sleep_list_lock);
        lock();

        if(!_dispatched) {
            _dispatched = true;
            if(hook.next) {
                if(_ticks >= 0) {
                    hook.next->_ticks += _ticks;
                }

                sleeping.erase(sleeping.iterator_to(this));
            }
        }

        release_lock(&sleep_list_lock);
        unlock();
    }

    void timer_event::dispatch() {
        kstd::lock l(_lock);
        if(!_dispatched) {
            _dispatched = true;
            sleeping.erase(sleeping.iterator_to(this));
            _callback(_context);
        }
    }
}