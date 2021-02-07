#pragma once

#include <cstdint>

extern uint8_t century_register;

typedef struct {
    uint8_t seconds;
    uint8_t minutes;
    uint8_t hours;
    uint8_t weekday;
    uint8_t day;
    uint8_t month;
    uint16_t year;
} datetime_t;

void rtc_init_interrupt();
datetime_t* rtc_read(datetime_t* dt);

void rtc_set_interrupt_frequency(uint8_t frequency);
uint8_t rtc_get_interrupt_frequency();
uint16_t rtc_get_interrupt_frequency_hz();