#pragma once

#include <stdint.h>
#include <system.h>
#include <spinlock.h>
#include <ref_counted.hpp>
#include <abi-bits/pid_t.h>

#include <frg/list.hpp>

typedef struct proc process_t;

namespace kstd {
    class lock;
}

namespace threading {
    constexpr uint8_t DEFAULT_TIMESLICE = 10;
    struct thread;

    class thread_blocker {
        friend struct thread;
    public:
        virtual ~thread_blocker() = default;

        virtual void interrupt();
        virtual void unblock();

        inline bool should_block() { return _should_block; }
        inline bool was_interrupted() { return _interrupted; }

        inline void set_thread(thread* t) { _thread = t; }

        kstd::ref_counted<kstd::lock> get_lock();
    protected:
        lock_t _lock {0};
        thread* _thread {nullptr};
        bool _should_block {true};  // If unblock() is called before the thread is blocked or the lock is acquired then tell the thread not to block
        bool _interrupted {false};  // Returned by Block so the thread knows it has been interrupted
        bool _removed {false};      // Has the blocker been removed from queue(s)?
    };

    class generic_thread_blocker : public thread_blocker {
    public:
        void interrupt() override {}
    };

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
        uint32_t time_slice {0};
        uint32_t time_slice_default {0};
        register_context registers;
        register_context last_syscall;
        void* fx_state;

        frg::default_list_hook<thread> hook;
        
        uint8_t priority {0};
        thread_state state;

        uint64_t fs_base {0};

        pid_t tid {0};

        bool blocked_timeout {false};
        thread_blocker* blocker {nullptr};

        void sleep(long us);

        [[nodiscard]] bool block(thread_blocker*);
        [[nodiscard]] bool block(thread_blocker*, long& timeout_us);
        void unblock();
    };
}