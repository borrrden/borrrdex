#pragma once

#include <bootloader_defs.h>

namespace hal {
    constexpr uint8_t VIDEO_MODE_INDEXED    = 0;
    constexpr uint8_t VIDEO_MODE_RGB        = 1;
    constexpr uint8_t VIDEO_MODE_TEXT       = 2;

    const memory_info_t* get_mem_info();
    const video_mode_t* get_video_mode();
    
    bool is_debug_mode();
    bool smp_disabled();
    const boot_module_t* get_boot_modules();

    void init_stivale2(stivale2_info_header_t* st2_info);
}