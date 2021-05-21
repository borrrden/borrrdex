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
#include <symbols.h>
#include <scheduler.h>
#include <device.h>
#include <storage/ahci.h>

const char* version = "Borrrdex x86_64";

typedef void (*ctor_t)(void);
extern ctor_t _ctors_start[0];
extern ctor_t _ctors_end[0];
extern "C" void _init();

video_mode_t video_mode;

extern "C" [[noreturn]] void idle_process() {
    while(true) {
        asm("sti");
        asm("hlt");
    }
}

[[noreturn]] void kernel_process() {
    ahci::initialize();

    if(fs::fs_node* node = fs::resolve_path("/system/lib")) {
        fs::register_volume(new fs::link_volume(node, "lib"), false);
    } else {
        fs::fs_node* initrd = fs::resolve_path("/initrd");
        assert(initrd);

        fs::register_volume(new fs::link_volume(initrd, "lib"), false);
    }

    log::info("Loading init process...");
    fs::fs_node* initfs_node = nullptr;
    const char* argv[] = {"init"};
    int envc = 1;
    const char* envp[] = {"PATH=/initrd", nullptr};
    if(!(initfs_node = fs::resolve_path("/system/borrrdex/init"))) {
        initfs_node = fs::resolve_path("/initrd/fterm");
        if(!initfs_node) {
            const char* panic_reasons[] = { "Failed to load init or fterm" };
            kernel_panic(panic_reasons, 1);
            __builtin_unreachable();
        }
    }

    log::write("OK");
    void* init_elf = malloc(initfs_node->size);
    fs::read(initfs_node, 0, initfs_node->size, init_elf);
    process_t* init_proc = scheduler::create_elf_process(init_elf, 1, (char **)argv, envc, (char **)envp);
    strncpy(init_proc->working_dir, "/", 1);
    strncpy(init_proc->name, "init", 5);
    scheduler::start_process(init_proc);

    while(true) {
        scheduler::gc();
        scheduler::get_current_thread()->sleep(1000000);
    }
}

static void initialize_constructors() {
    unsigned ctor_count = ((uint64_t)&_ctors_end - (uint64_t)&_ctors_start) / sizeof(void *);
    for(unsigned i = 0; i < ctor_count; i++) {
        _ctors_start[i]();
    }
}

extern "C" [[noreturn]] void kmain() {
    fs::initialize();
    device_manager::initialize();
    log::late_initialize();

    initialize_constructors();

    log::enable_klog();

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
    } else {
        log::warning("Splash image not found in initrd!");
    }

    fs::fs_node* symbol_file = fs::find_dir(initrd, "kernel.map");
    if(!symbol_file) {
        kernel_panic((const char*[]){"Symbol file not found!"}, 1);
        __builtin_unreachable();
    }

    load_symbols(symbol_file);
    video::draw_string("Copyright 2021 Jim Borden", 2, video_mode.height - 20, 255, 255, 255);
    video::draw_string(version, 2, video_mode.height - 40, 255, 255, 255);

    log::info("Initializing Task Scheduler...");
    scheduler::initialize();
    log::write("OK");
    while(true) {
        asm("hlt");
    }
}