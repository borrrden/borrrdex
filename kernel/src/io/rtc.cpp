#include "rtc.h"
#include "io.h"

extern "C" {
    void _rtc_init_interrupt();
    void _rtc_set_interrupt_frequency(uint8_t rate);
}

uint8_t century_register = 0x00;
uint8_t rtc_frequency = 6;

constexpr uint16_t CMOS_ADDRESS = 0x70;
constexpr uint16_t CMOS_DATA = 0x71;

bool binary_encoding, no_pm;

int get_update_in_progress_flag() {
    outb(CMOS_ADDRESS, 0xA);
    io_wait();
    return (inb(CMOS_DATA) & 0x80);
}

unsigned char get_rtc_register(uint8_t reg) {
    outb(CMOS_ADDRESS, reg);
    io_wait();
    return inb(CMOS_DATA);
}

void rtc_init_interrupt() {
    _rtc_init_interrupt();
    rtc_set_interrupt_frequency(7);
}

inline uint8_t decode_date_byte(uint8_t b) {
    return binary_encoding 
        ? b 
        : (b & 0x0F) + ((b / 16) * 10);
}

uint8_t decode_hour_byte(uint8_t b) {
    b = decode_date_byte(b);
    if(!no_pm || !(b & 0x80)) {
        return b;
    }

    return ((b & 0x7F) + 12) % 24;
}

datetime_t* rtc_read(datetime_t* dt) {
    while(get_update_in_progress_flag()) {
        // wait
    }

    unsigned char second = get_rtc_register(0x00);
    unsigned char minute = get_rtc_register(0x02);
    unsigned char hour = get_rtc_register(0x04);
    unsigned char weekday = get_rtc_register(0x06);
    unsigned char day = get_rtc_register(0x07);
    unsigned char month = get_rtc_register(0x08);
    unsigned char year = get_rtc_register(0x09);
    unsigned char century;
    if(century_register != 0x00) {
        century = get_rtc_register(century_register);
    }

    unsigned char b = get_rtc_register(0x0B);
    binary_encoding = b & 0x4;
    no_pm = b & 0x2;

    dt->seconds = decode_date_byte(second);
    dt->minutes = decode_date_byte(minute);
    dt->hours = decode_hour_byte(hour);
    dt->weekday = decode_date_byte(weekday);
    dt->day = decode_date_byte(day);
    dt->month = decode_date_byte(month);
    if(century_register != 0x00) {
        dt->year = decode_date_byte(century) * 100 + decode_date_byte(year);
    } else if(year >= 70) {
        dt->year = 1900 + dt->year;
    } else {
        dt->year = 2000 + dt->year;
    }

    return dt;
}

void rtc_set_interrupt_frequency(uint8_t frequency) {
    rtc_frequency = frequency;
    _rtc_set_interrupt_frequency(frequency);
}

uint8_t rtc_get_interrupt_frequency() {
    return rtc_frequency;
}

uint16_t rtc_get_interrupt_frequency_hz() {
    return 1 << (16 - rtc_frequency); 
}