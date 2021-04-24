#include "timer.h"
#include "cpuid.h"
#include "io/io.h"
#include "drivers/x86_64/pit.h"

static uint64_t cpu_speed;
static bool has_rdtsc;

static volatile uint64_t rdtsc() {
    volatile uint64_t val;
    asm volatile("rdtsc" : "=A"(val));
    return val;
}

void timer_init() {
    uint64_t a = 1, b, c, d;
    _cpuid(&a, &b, &c, &d);
    if(d & 0x10) {
        // disable the speaker, enable the T2 gate.
        port_write_8(0x61, (port_read_8(0x61) & ~0x02) | 0x01);

        // Set the PIT to Mode 0, counter 2, word access.
        port_write_8(0x43, 0xB0);

        // Load the counter with 0xFFFF
        port_write_8(0x42, 0xFF);
        port_write_8(0x42, 0xFF);

        // Read the number of ticks during the period.
        uint64_t start = rdtsc();
        while (!(port_read_8(0x61) & 0x20)) {
            // wait 
        }

        uint64_t end = rdtsc();
        cpu_speed = ((end - start) * (uint64_t)PIT_BASE_FREQUENCY / 0xffffull);
        has_rdtsc = true;
    } else {
        has_rdtsc = false;
    }
}

bool timer_available() {
    return has_rdtsc;
}

static void _delay(uint64_t ticks) {
    volatile uint64_t timeout = (rdtsc() + ticks);
    while(rdtsc() < timeout) {
        // wait
    }
}

void delay(uint32_t seconds) {
    _delay((uint64_t)seconds * cpu_speed);
}

void mdelay(uint32_t msecs) {
    _delay((uint64_t)msecs * cpu_speed / 1000);
}

void udelay(uint32_t usecs) {
    _delay((uint64_t)usecs * cpu_speed / 1000000);
}

void ndelay(uint32_t nsecs) {
    _delay((uint64_t)nsecs * cpu_speed / 1000000000);
}