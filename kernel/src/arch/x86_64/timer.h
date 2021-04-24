#pragma once
#include <stdint.h>

void timer_init();

bool timer_available();

void delay(uint32_t seconds);
void mdelay(uint32_t msecs);
void udelay(uint32_t usecs);
void ndelay(uint32_t nsecs);