#include <borrrdex/graphics/graphics.h>
#include <stddef.h>

extern "C" void memcpy_optimized(void* dst, const void* src, size_t count);

inline void memset32_optimized(void* dst, uint32_t c, size_t count) {
    uint32_t* dest = (uint32_t *)dst;
    while(count--) {
        *(dest++) = c;
    }
}

namespace borrrdex::graphics {
    void draw_rect(int x, int y, int width, int height, uint8_t r, uint8_t g, uint8_t b, surface_t* surface) {
        if(x < 0) {
            width += x;
            x = 0;
        }

        if(width <= 0) {
            return;
        }

        if(y < 0) {
            height += y;
            y = 0;
        }

        if(height <= 0) {
            return;
        }

        int w = ((x + width) < surface->width) ? width : (surface->width - x);
        uint32_t color = 0xFF000000 | (r << 16) | g << 8 | b;
        uint32_t* buf = (uint32_t *)surface->buffer;
        for(int i = 0; i < height && (i + y) < surface->height; i++) {
            uint32_t y_off = (i + y) * surface->width;
            if(w > 0) {
                memset32_optimized((void *)(buf + (y_off + x)), color, w);
            }
        }
    }

    void surface_copy(surface_t* dest, const surface_t* src) {
        if(dest->height == src->height) {
            memcpy_optimized(dest->buffer, src->buffer, dest->width * dest->height);
            return;
        }
    }
}