#pragma once

#include <spinlock.h>
#include <move.h>

namespace kstd {
    class lock final {
    public:
        ALWAYS_INLINE lock(lock_t& lock)
            :_lock(lock)
        {
            acquire_lock(&lock);
        }

        ALWAYS_INLINE ~lock() {
            release_lock(&_lock);
        }
    private:
        lock_t &_lock;
    };

    class read_write_lock final {
    public:
        inline void acquire_read() {
            // Enter the read lock
            acquire_lock(&_read_lock);

            // Even though this looks silly, release read doesn't use read_lock
            // so do this atomically 
            if(__atomic_add_fetch(&_active_readers, 1, __ATOMIC_ACQUIRE) == 1) {
                // The first reader acquires the write lock.  This does two things
                // 1. Blocks inside the read lock, queueing up further readers
                // 2. Waits for any pending writers to finish
                acquire_lock(&_write_lock);
            }

            // Exit the read lock, this allows any pending readers to start
            release_lock(&_read_lock);
        }

        inline void acquire_write() {
            // If readers are currently active, this will block until they are finished.
            // It also blocks if any other writer is active
            acquire_lock(&_write_lock);
        }

        inline bool try_acquire_write() {
            return _has_writer_lock || (_has_writer_lock = acquire_test_lock(&_write_lock));
        }

        inline void release_read() {
            if(__atomic_sub_fetch(&_active_readers, 1, __ATOMIC_RELEASE) == 0) {
                release_lock(&_write_lock);
            }
        }
    
        inline void release_write() {
            release_lock(&_write_lock);
        }
    private:
        lock_t _read_lock {0};
        lock_t _write_lock {0};
        unsigned _active_readers {0};

        bool _has_writer_lock {false};
    };
}