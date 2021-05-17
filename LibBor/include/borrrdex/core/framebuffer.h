#pragma once

#include <borrrdex/graphics/surface.h>

#ifdef __borrrdex__
#include <borrrdex/system/framebuffer.h>
#endif

surface_t* create_framebuffer_surface();
void create_framebuffer_surface(surface_t& surface);