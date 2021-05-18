#include <abi.h>
#include <idt.h>
#include <stddef.h>
#include <kerrno.h>
#include <scheduler.h>
#include <abi-bits/vm-flags.h>
#include <fcntl.h>
#include <logging.h>
#include <pty.h>
#include <debug.h>
#include <kstring.h>
#include <fs/fs_node.h>
#include <video/video.h>
#include <borrrdex/core/framebuffer.h>

using thread_state = threading::thread::thread_state;

typedef long(*syscall_t)(register_context*);

constexpr inline uint64_t SC_ARG0(register_context* r) { return r->rdi; }
constexpr inline uint64_t SC_ARG1(register_context* r) { return r->rsi; }
constexpr inline uint64_t SC_ARG2(register_context* r) { return r->rdx; }
constexpr inline uint64_t SC_ARG3(register_context* r) { return r->r10; }

long sys_read(register_context* regs) {
    process_t* proc = scheduler::get_current_process();
    fs::fs_fd_t* handle = proc->get_file_desc(SC_ARG0(regs));
    if(!handle) {
        log::warning("sys_read: invalid file descriptor: %d", SC_ARG0(regs));
        return -EBADF;
    }

    uint8_t* buffer = (uint8_t *)SC_ARG1(regs);
    uint64_t count = SC_ARG2(regs);

    if(!memory::check_usermode_pointer(SC_ARG1(regs), count, proc->address_space)) {
        log::warning("sys_read: invalid memory buffer: 0x%llx", SC_ARG1(regs));
        return -EFAULT;
    }

    ssize_t ret = fs::read(handle, count, buffer);
    return ret;
}

long sys_write(register_context* regs) {
    process_t* proc = scheduler::get_current_process();
    fs::fs_fd_t* handle = proc->get_file_desc(SC_ARG0(regs));
    if(!handle) {
        log::warning("sys_read: invalid file descriptor: %d", SC_ARG0(regs));
        return -EBADF;
    }

    uint8_t* buffer = (uint8_t *)SC_ARG1(regs);
    uint64_t count = SC_ARG2(regs);

    if(!memory::check_usermode_pointer(SC_ARG1(regs), count, proc->address_space)) {
        log::warning("sys_read: invalid memory buffer: 0x%llx", SC_ARG1(regs));
        return -EFAULT;
    }

    ssize_t ret = fs::write(handle, count, buffer);
    return ret;
}

long sys_exit(register_context* regs) {
    int64_t code = SC_ARG0(regs);
    log::info("Process %s (PID: %d) exiting with code %d", 
        scheduler::get_current_process()->name, scheduler::get_current_process()->pid, code);

    IF_DEBUG(debug_level_syscalls >= debug::LEVEL_VERBOSE, {
        log::info("rip: 0x%llx", regs->rip);
    })

    scheduler::end_process(scheduler::get_current_process());
    return 0;
}

long sys_open(register_context* regs) {
    const char* arg0 = (const char*)SC_ARG0(regs);
    size_t arg0_len = strnlen(arg0, fs::PATH_MAX) + 1;
    char* filepath = (char *)malloc(arg0_len);
    strncpy(filepath, arg0, arg0_len);
    uint64_t flags = SC_ARG1(regs);

    fs::fs_node* root = fs::get_root();
    process_t* proc = scheduler::get_current_process();
    IF_DEBUG(debug_level_syscalls >= debug::LEVEL_VERBOSE, {
        log::info("Opening: %s (flags: %u)", filepath, flags);
    })

    if(strncmp(filepath, "/", 2) == 0) {
        return proc->allocate_file_desc(fs::open(root, 0));
    }

open:
    fs::fs_node* node = fs::resolve_path(filepath, proc->working_dir, !(flags & O_NOFOLLOW));
    if(!node) {
        if(flags & O_CREAT) {
            return -EROFS; // Take care of writing later
        } else {
            IF_DEBUG(debug_level_syscalls >= debug::LEVEL_NORMAL, {
                log::warning("sys_open (flags: 0x%llx): Failed to open file %s", flags, filepath);
            })

            return -ENOENT;
        }
    }

    if((flags & O_DIRECTORY) && !node->is_dir()) {
        return -ENOTDIR;
    }

    if((flags & O_TRUNC) && ((flags & O_ACCMODE) == O_RDWR || (flags & O_ACCMODE) == O_WRONLY)) {
        return -EROFS; // Take care of writing later
    }

    fs::fs_fd_t* handle = fs::open(node, flags);
    if(!handle) {
        log::warning("sys_open: error retrieving file handle for node.  Dangling symlink?");
        return -ENOENT;
    }

    if(flags & O_APPEND) {
        handle->pos = handle->node->size;
    }

    return proc->allocate_file_desc(handle);
}

long sys_log(register_context* regs) {
    const char* msg = (const char*)SC_ARG0(regs);
    log::info("(%s): %s", scheduler::get_current_process()->name, msg);
    return 0;   
}

long sys_mmap(register_context* regs) {
    uint64_t* address = (uint64_t *)SC_ARG0(regs);
    size_t size = SC_ARG1(regs);
    uintptr_t hint = SC_ARG2(regs);
    uint64_t flags = SC_ARG3(regs);

    if(!size) {
        return -EINVAL;
    }

    process_t* proc = scheduler::get_current_process();
    bool fixed = flags & MAP_FIXED;
    bool anon = flags & MAP_ANONYMOUS;

    uint64_t unknown_flags = flags & ~static_cast<uint64_t>(MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED);
    if(unknown_flags || !anon) {
        log::warning("sys_mmap: Unsupported mmap flags 0x%llx", flags);
        return -EINVAL;
    }

    mm::mapped_region* region = proc->address_space->allocate_anonymous_vmo(size, hint, fixed);
    if(!region || !region->base()) {
        log::error("sys_mmap: Failed to map region (hint 0x%llx)", hint);
        return -1;
    }

    *address = region->base();
    return 0;
}

long sys_seek(register_context* regs) {
    process_t* proc = scheduler::get_current_process();
    fs::fs_fd_t* handle = proc->get_file_desc(SC_ARG0(regs));
    if(!handle) {
        log::warning("sys_seek: Invalid file descriptor: %d", SC_ARG0(regs));
        return -EBADF;
    }

    switch(SC_ARG2(regs)) {
        case SEEK_SET: {
            if(SC_ARG1(regs) > handle->node->size) {
                return -EINVAL;
            }

            return (handle->pos = SC_ARG1(regs));
        }
        case SEEK_CUR: {
            if(SC_ARG1(regs) + handle->pos > handle->node->size) {
                return -EINVAL;
            }
            
            return (handle->pos += SC_ARG1(regs));
        }
        case SEEK_END: {
            if(handle->node->size - SC_ARG1(regs) < 0) {
                return -EINVAL;
            }

            return (handle->pos = handle->node->size - SC_ARG1(regs));
        }
        default:
            log::warning("sys_seek: invalid seek mode: %d, mode: %d", SC_ARG0(regs), SC_ARG2(regs));
            return -EINVAL;
    }
}

long sys_close(register_context* regs) {
    process_t* proc = scheduler::get_current_process();
    int err = proc->destroy_file_desc(SC_ARG0(regs));
    if(err) {
        return -EBADF;
    }

    return 0;
}

long sys_set_fsbase(register_context* regs) {
    uint32_t low = SC_ARG0(regs) & 0xFFFFFFFF;
    uint32_t high = SC_ARG0(regs) >> 32;
    asm volatile("wrmsr" :: "a"(low), "d"(high), "c"(IA32_FS_BASE));
    get_cpu_local()->current_thread->fs_base = SC_ARG0(regs);
    return 0;
}

long sys_map_fb(register_context* regs) {
    video_mode_t vid_mode = video::get_video_mode();
    process_t* proc = scheduler::get_current_process();

    mm::mapped_region* region = proc->address_space->map_vmo(video::get_framebuffer_vmo(), 0, false);
    if(!region || !region->base()) {
        return -1;
    }

    uintptr_t virt = region->base();
    fb_info_t* fb_info = (fb_info_t *)SC_ARG1(regs);
    fb_info->width = vid_mode.width;
    fb_info->height = vid_mode.height;
    fb_info->bpp = vid_mode.bpp;
    fb_info->pitch = vid_mode.pitch;

    *((uintptr_t *)SC_ARG0(regs)) = virt;

    log::info("Framebuffer mapped at 0x%llx", virt);

    return 0;
}

long sys_grant_pty(register_context* regs) {
    if(!SC_ARG0(regs)) {
        return 1;
    }

    pty::pty* p = pty::grant_pty();
    process_t* proc = scheduler::get_current_process();
    proc->replace_file_desc(0, p->open_worker());
    proc->replace_file_desc(1, p->open_worker());
    proc->replace_file_desc(2, p->open_worker());

    *((int *)SC_ARG0(regs)) = proc->allocate_file_desc(p->open_controller());
    return 0;
}

syscall_t syscalls[NUM_SYSCALLS] = {
    sys_log,
    sys_open,
    sys_read,
    sys_write,
    sys_seek,
    sys_close,
    sys_exit,
    sys_mmap,
    sys_set_fsbase,
    sys_map_fb,
    sys_grant_pty
};

extern "C" void syscall_handler(register_context* regs) {
    if(__builtin_expect(regs->rax >= NUM_SYSCALLS, 0)) {
        regs->rax = -ENOSYS;
        return;
    }

    asm("sti");
    threading::thread* thread = get_cpu_local()->current_thread;
    if(__builtin_expect(thread->state == thread_state::zombie, 0)) {
        while(true) {
            asm("hlt");
        }
    }

    #ifdef KERNEL_DEBUG
    if(debug_level_syscalls >= debug::LEVEL_NORMAL) {
        thread->last_syscall = *regs;
    }
    #endif

    if(__builtin_expect(acquire_test_lock(&thread->lock), 0)) {
        while(true) {
            asm("hlt");
        }
    }

    regs->rax = syscalls[regs->rax](regs);
    release_lock(&thread->lock);
}