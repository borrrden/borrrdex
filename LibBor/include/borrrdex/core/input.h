#pragma once

#include <stdint.h>
#include <unistd.h>

namespace borrrdex {
    ssize_t poll_keyboard(uint8_t* buffer, size_t count);
}