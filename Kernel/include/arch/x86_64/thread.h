#pragma once

#include <stdint.h>
#include <system.h>
#include <spinlock.h>
#include <abi-bits/pid_t.h>

#include <frg/list.hpp>

typedef struct proc process_t;

namespace threading {
    constexpr uint8_t DEFAULT_TIMESLICE = 10;

    struct thread {
        enum class thread_state : uint8_t {
            running,
            blocked,
            zombie,
            dying
        };

        lock_t lock;
        lock_t state_lock;

        process_t* parent;
        void* stack;
        void* stack_limit;
        void* kernel_stack;
        uint32_t time_slice {0xFFFFFFFF};
        uint32_t time_slice_default {0xFFFFFFFF};
        register_context registers;
        register_context last_syscall;
        void* fx_state;

        frg::default_list_hook<thread> hook;
        
        uint8_t priority {0};
        thread_state state;

        uint64_t fs_base {0};

        pid_t tid {0};
    };
}