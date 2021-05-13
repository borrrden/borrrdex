#pragma once

#include <stdint.h>
#include <fs/filesystem.h>
#include <klist.hpp>
#include <abi-bits/pid_t.h>
#include <timer.h>
#include <paging.h>
#include <thread.h>
#include <spinlock.h>
#include <mm/address_space.h>

typedef struct proc {
    pid_t pid {-1};
    mm::address_space* address_space;
    threading::thread::thread_state state = threading::thread::thread_state::running;
    list<threading::thread *> threads;
    uid_t uid {-1};
    uid_t gid {-1};

    proc* parent {nullptr};
    list<proc *> children;

    char working_dir[fs::PATH_MAX];
    char name[fs::NAME_MAX];

    timeval creation_time;
    
    lock_t file_descriptors_lock;
    list<fs::fs_fd_t *> file_descriptors;

    ALWAYS_INLINE page_map_t* get_page_map() { return address_space->get_page_map(); }
} process_t;

namespace scheduler {
    void initialize();
    void tick(register_context* regs);
}