#include "rtc.h"
#include "cmos.h"
#include "io.h"

uint8_t century_register = 0x00;

constexpr uint16_t CMOS_ADDRESS = 0x70;
constexpr uint16_t CMOS_DATA = 0x71;

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

void read_rtc() {
    while(get_update_in_progress_flag()) {
        // wait
    }

    unsigned char second = get_rtc_register(0x00);
    unsigned char minute = get_rtc_register(0x02);
    unsigned char hour = get_rtc_register(0x04);
    unsigned char day = get_rtc_register(0x07);
    unsigned char month = get_rtc_register(0x08);
    unsigned char year = get_rtc_register(0x09);
    unsigned char century;
    if(century_register != 0x00) {
        century = get_rtc_register(century_register);
    }

    unsigned char last_second;
    unsigned char last_minute;
    unsigned char last_hour;
    unsigned char last_day;
    unsigned char last_month;
    unsigned char last_year;
    unsigned char last_century = century;

    do {
        last_second = second;
        last_minute = minute;
        last_hour = hour;
        last_day = day;
        last_month = month;
        last_year = year;

        while (get_update_in_progress_flag());           // Make sure an update isn't in progress
        second = get_rtc_register(0x00);
        minute = get_rtc_register(0x02);
        hour = get_rtc_register(0x04);
        day = get_rtc_register(0x07);
        month = get_rtc_register(0x08);
        year = get_rtc_register(0x09);
        if(century_register != 0x00) {
            century = get_rtc_register(century_register);
        }
    } while( (last_second != second) || (last_minute != minute) || (last_hour != hour) ||
            (last_day != day) || (last_month != month) || (last_year != year) ||
            (last_century != century) );

    unsigned char registerB = get_rtc_register(0x0B);
    if (!(registerB & 0x04)) {
            second = (second & 0x0F) + ((second / 16) * 10);
            minute = (minute & 0x0F) + ((minute / 16) * 10);
            hour = ( (hour & 0x0F) + (((hour & 0x70) / 16) * 10) ) | (hour & 0x80);
            day = (day & 0x0F) + ((day / 16) * 10);
            month = (month & 0x0F) + ((month / 16) * 10);
            year = (year & 0x0F) + ((year / 16) * 10);
            if(century_register != 0) {
                  century = (century & 0x0F) + ((century / 16) * 10);
            }
      }
}