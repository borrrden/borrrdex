#pragma once

#include <bootloader_defs.h>

namespace hal {
    constexpr uint8_t VIDEO_MODE_INDEXED    = 0;
    constexpr uint8_t VIDEO_MODE_RGB        = 1;
    constexpr uint8_t VIDEO_MODE_TEXT       = 2;

    extern memory_info_t mem_info;
    extern video_mode_t video_mode;
    extern bool debug_mode;

    void init_stivale2(stivale2_info_header_t* st2_info);
}