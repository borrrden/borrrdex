#pragma once

#include <stdint.h>

typedef struct surface {
    int width {0};
    int height {0};
    uint8_t depth {32};
    uint8_t* buffer {nullptr};
} surface_t;