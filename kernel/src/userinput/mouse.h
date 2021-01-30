#pragma once

#include "../math.h"
#include <stdint.h>

void ps2_mouse_init();
void ps2_mouse_handle(uint8_t data);
void ps2_mouse_process_packet();

extern Point MousePosition;