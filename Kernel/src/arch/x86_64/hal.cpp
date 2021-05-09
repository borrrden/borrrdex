#include <hal.h>
#include <serial.h>
#include <logging.h>
#include <idt.h>
#include <paging.h>
#include <physical_allocator.h>
#include <timer.h>
#include <debug.h>
#include <kstring.h>
#include <video/video.h>

extern void* _end;

namespace hal {
    video_mode_t video_mode;
    module::psf1_font_t* font;
    memory_info_t mem_info;
    boot_module_t boot_modules[128];
    int boot_module_count;
    bool debug_mode = false;
}

static void init_core() {
    uart::init();
    log::info("Initializing Borrrdex...\r\n");

    asm("cli");

    idt::initialize();

    memory::initialize_virtual_memory();
    memory::initialize_physical_allocator();

    log::info("Initializing System Timer...");
    timer::initialize(1600);
    log::write("OK", 0, 0, 255);
}

static void init_video() {
    for(int i = 0; i < hal::boot_module_count; i++) {
        const boot_module_t &m = hal::boot_modules[i];
        if(strcmp(module::SIGNATURE_FONT, m.name) == 0) {
            auto* font = reinterpret_cast<module::psf1_font_t *>(m.base);
            font->glyphs = reinterpret_cast<uint8_t *>(font) + sizeof(module::psf1_font_header_t);
            hal::font = font;
        }
    }

    video::initialize(hal::video_mode, hal::font);
    video::draw_string("Starting Borrrdex x64...", 0, 0, 255, 255, 255);
}

#define PAGE_COUNT_OF(x) (((x) + memory::PAGE_SIZE_4K - 1) / memory::PAGE_SIZE_4K)

void hal::init_stivale2(stivale2_info_header_t* st2_info) {
    uintptr_t tag_phys = st2_info->tags;

    init_core();

    char* cmd_line = nullptr;

    while(tag_phys) {
        stivale2_tag_t* tag = reinterpret_cast<stivale2_tag_t *>(tag_phys);
        log::debug(debug_level_hal, debug::LEVEL_VERBOSE, "[HAL] [stivale2] Found tag with ID: 0x%x", tag->id);
        switch(tag->id) {
            case stivale2::TAG_COMMAND_LINE: {
                stivale2_tag_cmdline_t* cmdline_tag = reinterpret_cast<stivale2_tag_cmdline_t *>(tag_phys);
                cmd_line = reinterpret_cast<char *>(cmdline_tag->cmdline);
                break;
            } case stivale2::TAG_MEMORY_MAP: {
                stivale2_tag_memory_map_t* mm_tag = reinterpret_cast<stivale2_tag_memory_map_t *>(tag_phys);
                if(mm_tag->entry_count > (memory::PAGE_SIZE_4K * 2) / sizeof(stivale2_tag_memory_map_t)) {
                    mm_tag->entry_count = (memory::PAGE_SIZE_4K * 2) / sizeof(stivale2_tag_memory_map_t);
                }

                for(unsigned i = 0; i < mm_tag->entry_count; i++) {
                    stivale2_memory_map_entry_t& entry = mm_tag->entries[i];
                    switch(entry.type) {
                        case stivale2::MEMORY_MAP_USABLE:
                        case stivale2::MEMORY_MAP_BOOTLOADER_RECLAIMABLE:
                            log::debug(debug_level_hal, debug::LEVEL_VERBOSE, "Memory region [0x%x-0x%x] available", entry.base, entry.base + entry.length);
                            memory::mark_memory_region_free(entry.base, entry.length);
                            mem_info.total_memory += entry.length;
                            break;
                        default:
                            log::debug(debug_level_hal, debug::LEVEL_VERBOSE, "Memory region [0x%x-0x%x] claimed", entry.base, entry.base + entry.length);
                            break;
                    }
                }

                memory::used_blocks = 0;
                break;
            } case stivale2::TAG_FRAMEBUFFER_INFO: {
                stivale2_tag_framebuffer_info_t* fb_tag = reinterpret_cast<stivale2_tag_framebuffer_info_t *>(tag_phys);
                video_mode.address = reinterpret_cast<void *>(memory::kernel_allocate_4k_pages(PAGE_COUNT_OF(fb_tag->buffer_pitch * fb_tag->buffer_height)));
                memory::kernel_map_virtual_memory_4k(fb_tag->buffer_address, (uintptr_t)video_mode.address, 
                    PAGE_COUNT_OF(fb_tag->buffer_pitch * fb_tag->buffer_height),
                    memory::PAGE_PAT_WRITE_COMB | memory::TABLE_WRITEABLE | memory::TABLE_PRESENT);
                
                video_mode.width = fb_tag->buffer_width;
                video_mode.height = fb_tag->buffer_height;
                video_mode.pitch = fb_tag->buffer_pitch;
                video_mode.bpp = fb_tag->buffer_bpp;
                video_mode.physical_address = fb_tag->buffer_address;
                if(fb_tag->memory_model == 1) {
                    video_mode.type = VIDEO_MODE_RGB;
                }

                break;
            } case stivale2::TAG_MODULES: {
                stivale2_tag_modules_t* modules_tag = reinterpret_cast<stivale2_tag_modules_t *>(tag_phys);
                for(unsigned i = 0; i < modules_tag->module_count; i++) {
                    stivale2_module_t& mod = modules_tag->modules[i];
                    uintptr_t base = (uintptr_t)memory::kernel_allocate_4k_pages(PAGE_COUNT_OF(mod.end - mod.begin));
                    boot_modules[boot_module_count] = { .base = base, .size = mod.end - mod.begin };
                    strncpy(boot_modules[boot_module_count].name, mod.string, 16);
                    memory::kernel_map_virtual_memory_4k(mod.begin, boot_modules[boot_module_count].base, PAGE_COUNT_OF(mod.end - mod.begin));
                    boot_module_count++;
                }
                
                break;
            } case stivale2::TAG_ACPI_RSDP: {
                stivale2_tag_rsdp_t* rsdp_tag = reinterpret_cast<stivale2_tag_rsdp_t *>(tag_phys);
            } default:
                break;
        }

        tag_phys = tag->next_tag;
    }

    if(cmd_line) {
        cmd_line = strtok(cmd_line, " ");
        while(cmd_line) {
            if(strcmp(cmd_line, "debug") == 0) {
                debug_mode = true;
            }

            cmd_line = strtok(cmd_line, " ");
        }
    }

    asm("sti");

    init_video();
}

extern "C" 
[[noreturn]] void kinit_stivale2(stivale2_info_header_t* st2_info) {
    hal::init_stivale2(st2_info);

    while(true) {
        asm volatile("hlt");
    }
}