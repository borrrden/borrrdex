#pragma once

#include <stdint.h>
#include <borrrdex/system/abi/framebuffer.h>

#ifndef __borrrdex__
#error "Borrrdex only"
#endif

long borrdex_map_framebuffer(void** ptr, fb_info_t& fb_info);