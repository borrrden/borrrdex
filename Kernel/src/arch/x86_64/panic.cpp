#include <panic.h>
#include <video/video.h>
#include <kstring.h>

void kernel_panic(const char** reasons, int reason_count) {
    asm("cli");

    video_mode_t v = video::get_video_mode();
    video::draw_rect(0, 0, v.width, v.height, 255, 0, 0);
    int pos = 20;
    for(int i = 0; i < reason_count; i++){
        video::draw_string(reasons[i], v.width / 2 - strnlen(reasons[i], 1024) * 8 / 2, pos, 0, 0, 0, true);
        pos += 10;
    }

    video::draw_string("Borrrdex kernel panic!", 0, v.height - 200, 0, 0, 0, true);
    video::draw_string("The system has been halted.", 0, v.height - 200 + 8, 0, 0, 0, true);

    while(true) {
        asm("hlt");
    }
}