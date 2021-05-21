#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <borrrdex/core/framebuffer.h>
#include <borrrdex/graphics/font.h>
#include <borrrdex/graphics/graphics.h>
#include <borrrdex/syscall.h>
#include <unistd.h>

using namespace borrrdex::graphics;

surface_t fb;
surface_t render;
int pty;

int main(int argc, char** argv) {
    create_framebuffer_surface(fb);
    render = fb;
    render.buffer = (uint8_t *)malloc(fb.width * fb.height * 4);

    syscalln1(SYSCALL_GRANT_PTY, (uintptr_t)&pty);
    printf("Hello, World\n");

    char buf[256];
    int len = read(pty, buf, 256);

    font f = default_font();
    draw_rect(0, 0, render.width, render.height, 128, 128, 128, &render);
    draw_string(buf, 10, 10, 0, 0, 0, &render, &f);
    surface_copy(&fb, &render);

    return 0;
}