#pragma once

#include <list>
#include <stdint.h>

typedef struct {
    int x;
    int y;
} vector2i_t;

inline vector2i_t operator+(const vector2i_t& l, const vector2i_t& r) {
    return {l.x + r.x, l.y + r.y};
}

inline void operator+=(vector2i_t& l, const vector2i_t& r) {
    l = l + r;
}

inline vector2i_t operator-(const vector2i_t& l, const vector2i_t& r) {
    return {l.x - r.x, l.y - r.y};
}

inline void operator-=(vector2i_t& l, const vector2i_t& r) {
    l = l - r;
}

inline bool operator==(const vector2i_t& l, const vector2i_t& r) {
    return l.x == r.x && l.y == r.y;
}

typedef struct rgba8888 {
    union {
        struct {
            uint8_t r;
            uint8_t g;
            uint8_t b;
            uint8_t a;
        };
        uint32_t val;
    };

    inline static constexpr rgba8888 from_rgb(uint32_t rgb) {
        return { .val = __builtin_bswap32((rgb << 8) | 0xff) };
    }

    inline static constexpr rgba8888 from_argb(uint32_t argb) {
        return { .val = __builtin_bswap32((argb << 8) | ((argb >> 24) & 0xff)) };
    }
} rgba8888_t;

