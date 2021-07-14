#include <abi.h>
#include <idt.h>
#include <stddef.h>
#include <abi-bits/errno.h>
#include <scheduler.h>
#include <abi-bits/vm-flags.h>
#include <fcntl.h>
#include <logging.h>
#include <pty.h>
#include <debug.h>
#include <kstring.h>
#include <fs/fs_node.h>
#include <fs/filesystem.h>
#include <fs/pipe.h>
#include <video/video.h>
#include <borrrdex/core/framebuffer.h>
#include <frg/random.hpp>
#include <sys/ioctl.h>

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
            if(SC_ARG1(regs) > 0 || SC_ARG1(regs) + handle->pos < 0) {
                return -EINVAL;
            }

            return (handle->pos = handle->node->size + SC_ARG1(regs));
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

long sys_uptime(register_context* regs) {
    uint64_t* seconds = (uint64_t*)SC_ARG0(regs);
	uint64_t* milliseconds = (uint64_t*)SC_ARG1(regs);
    if(seconds) {
        *seconds = timer::get_system_uptime();
    }

    if(milliseconds) {
        *milliseconds = timer::get_ticks() * 1000 / timer::get_frequency();
    }

    return 0;
}

long sys_ioctl(register_context* regs) {
    uint64_t request = SC_ARG1(regs);
    uint64_t arg = SC_ARG2(regs);
    int* result = (int *)SC_ARG3(regs);

    process_t* proc = scheduler::get_current_process();

    if(result && !memory::check_usermode_pointer((uintptr_t)result, sizeof(int), proc->address_space)) {
        log::warning("(%s): sys_ioctl: invalid result buffer: 0x%llx", proc->name, SC_ARG2(regs));
        return -EFAULT;
    }

    fs::fs_fd_t* handle = proc->get_file_desc(SC_ARG0(regs));
    if(!handle) {
        log::warning("sys_ioctl: Invalid file descriptor: %d", SC_ARG0(regs));
        return -EBADF;
    }

    if(request == FIOCLEX) {
        handle->mode |= O_CLOEXEC;
        return 0;
    }

    int ret = fs::ioctl(handle, request, arg);
    if(result && ret > 0) {
        *result = ret;
    }

    return ret;
}

long sys_getuid(register_context* regs) {
    return scheduler::get_current_process()->uid;
}

long sys_setuid(register_context* regs) {
    auto proc = scheduler::get_current_process();
    uid_t requested_id = SC_ARG0(regs);
    if(proc->uid == requested_id) {
        return 0;
    }

    if(proc->euid != 0) {
        return -EPERM;
    }

    proc->uid = requested_id;
    proc->euid = requested_id;
    return 0;
}

long sys_getgid(register_context* regs) {
    return scheduler::get_current_process()->gid;
}

long sys_setgid(register_context* regs) {
    auto proc = scheduler::get_current_process();
    uid_t requested_id = SC_ARG0(regs);
    if(proc->gid == requested_id) {
        return 0;
    }

    if(proc->euid != 0) {
        return -EPERM;
    }

    proc->gid = requested_id;
    proc->egid = requested_id;
    return 0;
}

long sys_geteuid(register_context* regs) {
    return scheduler::get_current_process()->euid;
}

long sys_seteuid(register_context* regs) {
    auto proc = scheduler::get_current_process();
    uid_t requested_id = SC_ARG0(regs);
    if(proc->euid == requested_id) {
        return 0;
    }

    if(proc->uid != 0 && proc->uid != requested_id) {
        return -EPERM;
    }

    proc->euid = requested_id;
    return 0;
}

long sys_getegid(register_context* regs) {
    return scheduler::get_current_process()->egid;
}

long sys_setegid(register_context* regs) {
    auto proc = scheduler::get_current_process();
    uid_t requested_id = SC_ARG0(regs);
    if(proc->egid == requested_id) {
        return 0;
    }

    if(proc->uid != 0 && proc->gid != requested_id) {
        return -EPERM;
    }

    proc->egid = requested_id;
    return 0;
}

long sys_getpid(register_context* regs) {
    return scheduler::get_current_process()->pid;
}

long sys_getppid(register_context* regs) {
    auto* proc = scheduler::get_current_process();
    return proc->parent ? proc->parent->pid : -1;
}

long sys_dup(register_context* regs) {
    int fd = SC_ARG0(regs);
    int requested_fd = SC_ARG2(regs);
    if(fd == requested_fd) {
        return -EINVAL;
    }

    auto current_proc = scheduler::get_current_process();
    long flags = SC_ARG1(regs);
    auto* handle = current_proc->get_file_desc(fd);
    if(!handle) {
        return -EBADF;
    }

    auto* new_handle = new fs::fs_fd_t();
    *new_handle = *handle;
    new_handle->node->add_handle();
    if(flags & O_CLOEXEC) {
        new_handle->mode |= O_CLOEXEC;
    }

    if(requested_fd >= 0) {
        if((unsigned)requested_fd >= current_proc->file_desc_count()) {
            log::error("sys_dup: Unallocated fd request not supported");
            return -ENOSYS;
        }

        current_proc->destroy_file_desc(requested_fd);
        if(current_proc->replace_file_desc(requested_fd, new_handle)) {
            return -EBADF;
        }

        return requested_fd;
    }

    return current_proc->allocate_file_desc(new_handle);
}

long sys_get_fstat_flags(register_context* regs) {
    auto* cur_process = scheduler::get_current_process();
    fs::fs_fd_t* handle = cur_process->get_file_desc(SC_ARG0(regs));
    return handle ? handle->mode : -EBADF;
}

long sys_set_fstat_flags(register_context* regs) {
    auto* cur_process = scheduler::get_current_process();
    fs::fs_fd_t* handle = cur_process->get_file_desc(SC_ARG0(regs));
    if(!handle) {
        return -EBADF;
    }

    int mask = (O_APPEND | O_NONBLOCK); // only these are supported
    int flags = SC_ARG1(regs);
    handle->mode = (handle->mode & ~mask) | (flags & mask);
    return 0;
}

long sys_pipe(register_context* regs) {
    int* fds = (int *)SC_ARG0(regs);
    int flags = SC_ARG1(regs);
    if(flags & ~(O_CLOEXEC | O_NONBLOCK)) {
        log::debug(debug_level_syscalls, debug::LEVEL_NORMAL, "sys_pipe: Invalid flags %d", flags);
        return -EINVAL;
    }

    auto* proc = scheduler::get_current_process();

    fs::unix_pipe* read, *write;
    fs::unix_pipe::create_pipe(read, write);

    fs::fs_fd_t* read_handle = fs::open(read);
    fs::fs_fd_t* write_handle = fs::open(write);
    read_handle->mode = flags;
    write_handle->mode = flags;

    fds[0] = proc->allocate_file_desc(read_handle);
    fds[1] = proc->allocate_file_desc(write_handle);

    return 0;
}

long sys_fstat(register_context* regs) {
    auto* proc = scheduler::get_current_process();
    stat_t* stat = (stat_t *)SC_ARG0(regs);
    fs::fs_fd_t* handle = proc->get_file_desc(SC_ARG1(regs));
    if(!handle) {
        log::warning("sys_fstat: Invalid file descriptor %d", SC_ARG1(regs));
        return -EBADF;
    }

    fs::fs_node* const& node = handle->node;
    stat->st_dev = 0;
    stat->st_ino = node->inode;
    stat->st_mode = 0;

    if(node->is_dir()) stat->st_mode |= fs::S_IFDIR;
    if(node->is_file()) stat->st_mode |= fs::S_IFREG;
    if(node->is_block_dev()) stat->st_mode |= fs::S_IFBLK;
    if(node->is_char_dev()) stat->st_mode |= fs::S_IFCHR;
    if(node->is_symlink()) stat->st_mode |= fs::S_IFLNK;
    if(node->is_socket()) stat->st_mode |= fs::S_IFSOCK;

    stat->st_nlink = 0;
    stat->st_uid = node->uid;
    stat->st_gid = 0;
    stat->st_rdev = 0;
    stat->st_size = node->size;
    stat->st_blksize = 0;
    stat->st_blocks = 0;
    return 0;
}

long sys_stat(register_context* regs) {
    auto* proc = scheduler::get_current_process();
    stat_t* stat = (stat_t *)SC_ARG0(regs);
    const char* filepath = (const char*)SC_ARG1(regs);
    uint64_t flags = SC_ARG2(regs);

    if(!memory::check_usermode_pointer(SC_ARG0(regs), sizeof(stat_t), proc->address_space)) {
        log::error("sys_stat: stat structure points to invalid address 0x%x", SC_ARG0(regs));
        return -EINVAL;
    }

    if(!memory::check_usermode_pointer(SC_ARG1(regs), sizeof(stat_t), proc->address_space)) {
        log::error("sys_stat: filepath points to invalid address %s", filepath);
        return -EINVAL;
    }

    bool follow_symlinks = !(flags & AT_SYMLINK_NOFOLLOW);
    fs::fs_node* node = fs::resolve_path(filepath, proc->working_dir, follow_symlinks);
    if(!node) {
        log::debug(debug_level_syscalls, debug::LEVEL_VERBOSE, "sys_stat: nonexistent filepath %s", filepath);
        return -ENOENT;
    }

    stat->st_dev = 0;
    stat->st_ino = node->inode;
    stat->st_mode = 0;

    if(node->is_dir()) stat->st_mode |= fs::S_IFDIR;
    if(node->is_file()) stat->st_mode |= fs::S_IFREG;
    if(node->is_block_dev()) stat->st_mode |= fs::S_IFBLK;
    if(node->is_char_dev()) stat->st_mode |= fs::S_IFCHR;
    if(node->is_symlink()) stat->st_mode |= fs::S_IFLNK;
    if(node->is_socket()) stat->st_mode |= fs::S_IFSOCK;

    stat->st_nlink = 0;
    stat->st_uid = node->uid;
    stat->st_gid = 0;
    stat->st_rdev = 0;
    stat->st_size = node->size;
    stat->st_blksize = 0;
    stat->st_blocks = 0;
    return 0;
}

syscall_t syscalls[NUM_SYSCALLS] = {
    sys_log,
    sys_open,
    sys_read,
    sys_write,
    sys_seek,
    sys_close,
    sys_ioctl,
    sys_exit,
    sys_mmap,
    sys_set_fsbase,
    sys_map_fb,
    sys_grant_pty,
    sys_uptime,
    sys_getuid,
    sys_setuid,
    sys_getgid,
    sys_setgid,
    sys_geteuid,
    sys_seteuid,
    sys_getegid,
    sys_setegid,
    sys_getpid,
    sys_getppid,
    sys_dup,
    sys_get_fstat_flags,
    sys_set_fstat_flags,
    sys_pipe,
    sys_fstat,
    sys_stat
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

    if(!__builtin_expect(acquire_test_lock(&thread->lock), true)) {
        while(true) {
            asm("hlt");
        }
    }

    regs->rax = syscalls[regs->rax](regs);
    release_lock(&thread->lock);
}