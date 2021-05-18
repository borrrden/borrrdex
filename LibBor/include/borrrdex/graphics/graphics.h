#pragma once

#include <borrrdex/graphics/surface.h>

#include <stdint.h>

// https://docs.fileformat.com/image/bmp/
typedef struct {
    char magic[2];
    uint32_t size;
    uint32_t reserved;
    uint32_t offset;
} __attribute__((packed)) bitmap_file_header_t;

namespace borrrdex::graphics {
    void draw_rect(int x, int y, int width, int height, uint8_t r, uint8_t g, uint8_t b, surface_t* surface);
    void surface_copy(surface_t*, const surface_t*);
}