#include <timer.h>
#include <kassert.h>
#include <idt.h>
#include <io.h>
#include <scheduler.h>

namespace timer {
    int frequency;
    long ticks = 0;
    long pending_ticks = 0;
    long long uptime = 0;

    static void timer_handler(void*, register_context* regs) {
        ticks++;
        if(ticks >= frequency) {
            uptime++;
            ticks -= frequency;
        }

        pending_ticks++;
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
}