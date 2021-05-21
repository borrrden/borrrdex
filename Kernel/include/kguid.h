#pragma once

#include <stdint.h>

#define CHECK_ZERO(x) if((x) != 0) return false;

typedef struct guid {
    uint32_t d1;
    uint16_t d2;
    uint16_t d3;
    uint8_t d4[8];

    bool operator==(const guid& other) const {
        int32_t r, *g1, *g2;
        g1 = (int32_t *)this;
        g2 = (int32_t *)&other;

        CHECK_ZERO(g1[0] - g2[0]);
        CHECK_ZERO(g1[1] - g2[1]);
        CHECK_ZERO(g1[2] - g2[2]);
        CHECK_ZERO(g1[3] - g2[3]);

        return true;
    }
} __attribute__((packed)) guid_t;

#undef CHECK_ZERO

constexpr guid_t nullguid = { 0, 0, 0, { 0, 0, 0, 0, 0, 0, 0, 0 }};