#pragma once

#include <stdint.h>
#include <fs/filesystem.h>
#include <fs/fs_node.h>
#include <klist.hpp>
#include <abi-bits/pid_t.h>
#include <timer.h>
#include <paging.h>
#include <thread.h>
#include <lock.hpp>
#include <mm/address_space.h>
#include <cpu.h>
#include <fs/filesystem.h>

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
    uint64_t active_ticks {0};

    ~proc() {
        for(auto fd : _file_descriptors) {
            fd->node->close();
            delete fd;
        }

        _file_descriptors.clear();
    }

    ALWAYS_INLINE int allocate_file_desc(fs::fs_fd_t* fd) {
        kstd::lock l(_file_desc_lock);
        unsigned i = 0;
        for(; i < _file_descriptors.size(); i++) {
            if(!_file_descriptors[i] && i > 2) {
                _file_descriptors.set(fd, i);
                return i;
            }
        }

        _file_descriptors.add(fd);
        return i;
    }

    ALWAYS_INLINE int replace_file_desc(int fd, fs::fs_fd_t* ptr) {
        kstd::lock l(_file_desc_lock);
        if(fd > _file_descriptors.size()) {
            return 1;
        }

        if(fd == _file_descriptors.size()) {
            _file_descriptors.add(ptr);
            return 0;
        }

        fs::fs_fd_t* f = _file_descriptors[fd];
        if(f) {
            fs::close(f);
            delete f;
        }

        _file_descriptors.set(ptr, fd);
        return 0;
    }

    ALWAYS_INLINE int destroy_file_desc(int fd) {
        kstd::lock l(_file_desc_lock);
        if(fd > _file_descriptors.size()) {
            return 1;
        }

        if(fs::fs_fd_t* f = _file_descriptors[fd]) {
            fs::close(f);
            delete f;
            _file_descriptors.set(nullptr, fd);
            return 0;
        }

        return 1;
    }

    ALWAYS_INLINE fs::fs_fd_t* get_file_desc(int fd) {
        kstd::lock l(_file_desc_lock);
        if(fd < _file_descriptors.size()) {
            return _file_descriptors[fd];
        }

        return nullptr;
    }

    ALWAYS_INLINE void destroy_all_files() {
        kstd::lock l(_file_desc_lock);
        for(auto fd : _file_descriptors) {
            if(fd && fd->node) {
                fd->node->close();
                fd->node = nullptr;
            }

            delete fd;
        }

        _file_descriptors.clear();
    }

    ALWAYS_INLINE unsigned file_desc_count() const { return _file_descriptors.size(); }

    ALWAYS_INLINE page_map_t* get_page_map() { return address_space->get_page_map(); }
private:
    list<fs::fs_fd_t *> _file_descriptors;
    lock_t _file_desc_lock {0};
} process_t;

namespace scheduler {
    void initialize();
    void tick(register_context* regs);

    void start_process(process_t* proc);
    void end_process(process_t* proc);

    void yield();

    void gc();

    process_t* create_elf_process(void* elf, int argc = 0, char** argv = nullptr, 
        int envc = 0, char** envp = nullptr, const char* exec_path = nullptr);

    inline static process_t* get_current_process() {
        cpu* c = get_cpu_local();
        process_t* ret = nullptr;
        if(c->current_thread) {
            ret = c->current_thread->parent;
        }

        return ret;
    }

    inline static threading::thread* get_current_thread() {
        cpu* c = get_cpu_local();
        return c->current_thread;
    }
}