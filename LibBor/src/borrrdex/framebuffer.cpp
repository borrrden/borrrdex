#include <borrrdex/core/framebuffer.h>
#include <assert.h>
#include <stdlib.h>
#include <borrrdex/syscall.h>

long borrdex_map_framebuffer(void** ptr, fb_info_t& fb_info) {
    syscalln2(SYSCALL_MAP_FB, (uintptr_t)ptr, (uint64_t)&fb_info);
}

surface_t* create_framebuffer_surface() {
    surface_t* surface = (surface_t *)malloc(sizeof(surface_t));
    create_framebuffer_surface(*surface);
    return surface;
}

void create_framebuffer_surface(surface_t &surface) {
    fb_info_t fb_info;
    long err = borrdex_map_framebuffer((void **)&surface.buffer, fb_info);
    assert(!err && surface.buffer);

    surface.width = fb_info.width;
    surface.height = fb_info.height;
    surface.depth = fb_info.bpp;
}