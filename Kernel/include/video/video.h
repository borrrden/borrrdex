#pragma once

#include <bootloader_defs.h>
#include <stdint.h>
#include <ref_counted.hpp>
#include <mm/vm_object.h>

namespace video {
    void initialize(video_mode_t video_mode , module::psf1_font_t* font);

    void draw_rect(unsigned int x, unsigned int y, unsigned int width, unsigned int height, uint8_t r, uint8_t g, uint8_t b);

    void draw_char(char c, unsigned int x, unsigned int y, uint8_t r, uint8_t g, uint8_t b);
    void draw_string(const char* str, unsigned int x, unsigned int y, uint8_t r, uint8_t g, uint8_t b, bool fallback = false);
    void draw_bitmap(unsigned int x, unsigned int y, unsigned int w, unsigned int h, uint8_t *data);

    video_mode_t get_video_mode();
    kstd::ref_counted<mm::vm_object> get_framebuffer_vmo();
}