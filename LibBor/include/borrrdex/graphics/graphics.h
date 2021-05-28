#pragma once

#include <borrrdex/graphics/surface.h>
#include <borrrdex/graphics/types.h>

#include <stdint.h>

// https://docs.fileformat.com/image/bmp/
typedef struct {
    char magic[2];
    uint32_t size;
    uint32_t reserved;
    uint32_t offset;
} __attribute__((packed)) bitmap_file_header_t;

namespace borrrdex::graphics {
    class font;
    
    void draw_rect(int x, int y, int width, int height, uint8_t r, uint8_t g, uint8_t b, surface_t* surface);
    inline void draw_rect(int x, int y, int width, int height, rgba8888_t color, surface_t* surface) {
        draw_rect(x, y, width, height, color.r, color.g, color.b, surface);
    }

    int draw_char(char c, int x, int y, uint8_t r, uint8_t g, uint8_t b, surface_t* surface, font* font);
    inline void draw_char(char c, int x, int y, rgba8888_t color, surface_t* surface, font* font) {
        draw_char(c, x, y, color.r, color.g, color.b, surface, font);
    }

    int draw_string(const char* str, int x, int y, uint8_t r, uint8_t g, uint8_t b, surface_t* surface, font* font);
    inline void draw_string(const char* str, int x, int y, rgba8888_t color, surface_t* surface, font* font) {
        draw_string(str, x, y, color.r, color.g, color.b, surface, font);
    }
    
    void surface_copy(surface_t*, const surface_t*);

    static inline uint32_t alpha_blend(uint32_t old, uint8_t r, uint8_t g, uint8_t b, double opacity) {
        int old_b = old & 0xFF;
        int old_g = (old >> 8) & 0xFF;
        int old_r = (old >> 16) & 0xFF;
        return (uint32_t)(b * opacity + old_b * (1 - opacity)) 
            | (((uint32_t)(g * opacity + old_g * (1 - opacity)) << 8))
            | (((uint32_t)(r * opacity + old_r * (1 - opacity)) << 16));
    }
}