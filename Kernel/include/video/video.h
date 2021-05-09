#pragma once

#include <bootloader_defs.h>
#include <stdint.h>

namespace video {
    void initialize(video_mode_t video_mode , module::psf1_font_t* font);

    void draw_rect(unsigned int x, unsigned int y, unsigned int width, unsigned int height, uint8_t r, uint8_t g, uint8_t b);

    void draw_char(char c, unsigned int x, unsigned int y, uint8_t r, uint8_t g, uint8_t b);
    void draw_string(const char* str, unsigned int x, unsigned int y, uint8_t r, uint8_t g, uint8_t b, bool fallback = false);

    video_mode_t get_video_mode();
}