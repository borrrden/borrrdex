#pragma once

#ifndef __cplusplus
#error C++ Only
#endif

#include <stdint.h>

void stalloc_init();
uint64_t* stalloc(int bytes);
void stalloc_disable();