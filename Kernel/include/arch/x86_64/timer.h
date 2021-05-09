#pragma once

#include <stdint.h>
#include <bits/posix/timeval.h>

typedef long time_t;

static inline bool operator<(timeval l, timeval r) {
    return (l.tv_sec < r.tv_sec) || (l.tv_sec == r.tv_sec && l.tv_usec < r.tv_usec);
}

typedef struct {
    time_t tv_sec;
    long tv_nsec;
} timespec_t;

struct thread;

namespace timer {
    constexpr uint8_t PIT_COUNTER0_REG  = 0x40;
    constexpr uint8_t PIT_COUNTER1_REG  = 0x41;
    constexpr uint8_t PIT_COUNTER2_REG  = 0x42;
    constexpr uint8_t PIT_COMMAND_REG   = 0x43;

    // Control Word Bits
    constexpr uint8_t PIT_CW_MASK_BINCOUNT  = 0x01;
    constexpr uint8_t PIT_CW_MASK_MODE      = 0x0E;
    constexpr uint8_t PIT_CW_MASK_RL        = 0x30;
    constexpr uint8_t PIT_CW_MASK_COUNTER   = 0xC0;

    // Bit 0, Binary Counter Format               XXXXXXX0
    constexpr uint8_t PIT_CW_MASK_BINARY        = 0;
    constexpr uint8_t PIT_CW_MASK_BCS           = 1;

    // Bits 1 - 3, Operation Mode                 XXXX000X
    constexpr uint8_t PIT_CW_MASK_COUNTDOWN     = 0x0;
    constexpr uint8_t PIT_CW_MASK_ONESHOT       = 0x2;
    constexpr uint8_t PIT_CW_MASK_RATEGEN       = 0x4;
    constexpr uint8_t PIT_CW_MASK_SQRRATEGEN    = 0x6;
    constexpr uint8_t PIT_CW_MASK_SWTRIGGER     = 0x8;
    constexpr uint8_t PIT_CW_MASK_HWTRIGGER     = 0xA;

    // Bits 4 - 5, Read / Load Mode               XX00XXXX
    constexpr uint8_t PIT_CW_MASK_LATCH         = 0x00;
    constexpr uint8_t PIT_CW_MASK_LSB           = 0x10;
    constexpr uint8_t PIT_CW_MASK_MSB           = 0x20;
    constexpr uint8_t PIT_CW_MASK_DATA          = 0x30;

    // Bits 6 - 7, Counter Selection              00XXXXXX
    constexpr uint8_t PIT_CW_MASK_COUNTER0      = 0x00;
    constexpr uint8_t PIT_CW_MASK_COUNTER1      = 0x40;
    constexpr uint8_t PIT_CW_MASK_COUNTER2      = 0x80;
    constexpr uint8_t PIT_CW_MASK_COUNTERINV    = 0x90;

    constexpr uint32_t PIT_BASE_FREQUENCY   = 1193181;  // True oscillation rate of the timer (Hz)

    uint64_t get_system_uptime(timeval* t = nullptr);
    uint32_t get_ticks();
    uint32_t get_frequency();

    long time_difference(const timeval& new_time, const timeval& old_time);

    void wait(long ms);

    void initialize(uint32_t freq);
}

inline long operator-(const timeval& l, const timeval& r) {
    return timer::time_difference(l, r);
}