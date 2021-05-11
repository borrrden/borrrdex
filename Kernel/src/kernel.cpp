#include <fs/filesystem.h>
#include <fs/tar.h>
#include <fs/fs_node.h>
#include <video/video.h>
#include <debug.h>
#include <logging.h>
#include <physical_allocator.h>
#include <kassert.h>
#include <panic.h>
#include <hal.h>
#include <liballoc/liballoc.h>

typedef void (*ctor_t)(void);
extern ctor_t _ctors_start[0];
extern ctor_t _ctors_end[0];
extern "C" void _init();

video_mode_t video_mode;

static void initialize_constructors() {
    unsigned ctor_count = ((uint64_t)&_ctors_end - (uint64_t)&_ctors_start) / sizeof(void *);
    for(unsigned i = 0; i < ctor_count; i++) {
        _ctors_start[i]();
    }
}

extern "C" [[noreturn]] void kmain() {
    fs::initialize();

    initialize_constructors();

    video_mode = video::get_video_mode();
    log::debug(debug_level_misc, debug::LEVEL_VERBOSE, "Video Resolution: %dx%dx%d", 
        video_mode.width, video_mode.height, video_mode.bpp);

    if(video_mode.height < 600) {
        log::warning("Small Resolution (%dx%d), it is recommended to use a higher resoulution if possible.",
            video_mode.width, video_mode.height);
    }

    if(video_mode.bpp < 32) {
        log::warning("Unsupported Color Depth (%d)", video_mode.bpp);
    }

    video::draw_rect(0, 0, video_mode.width, video_mode.height, 0, 0, 0);
    log::info("Used RAM: %d MB", memory::get_used_blocks() * 0x1000 / 1024 / 1024);

    assert(fs::get_root());

    log::info("Initializing Ramdisk...");
    auto* boot_modules = hal::get_boot_modules();
    auto* tar = new fs::tar::tar_volume(boot_modules[0].base, boot_modules[0].size, "initrd");
    fs::register_volume(tar);
    log::write("OK");

    fs::fs_node* initrd = fs::find_dir(fs::get_root(), "initrd");
    if(!initrd) {
        kernel_panic((const char*[]){"initrd not mounted!"}, 1);
        __builtin_unreachable();
    }

    fs::fs_node* splash_file = fs::find_dir(initrd, "splash.bmp");
    if(splash_file) {
        uint32_t size = splash_file->size;
        uint8_t* buf = (uint8_t *)malloc(size);
        if(fs::read(splash_file, 0, size, buf)) {
            video::draw_bitmap(video_mode.width / 2 - 530 / 2, video_mode.height / 2 - 178 / 2, 530, 178, buf);
        }

        free(buf);
    }

    while(true) {
        asm("hlt");
    }
}