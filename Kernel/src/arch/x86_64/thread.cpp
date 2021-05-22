#include <thread.h>
#include <kassert.h>
#include <timer_event.h>
#include <scheduler.h>
#include <lock.h>

namespace threading {
    void thread::sleep(long us) {
        assert(check_interrupts());

        blocked_timeout = false;
        timer::timer_callback_t timer_cb = [](void *t) {
            thread* thr = (thread *)t;
            thr->blocked_timeout = true;
            thr->unblock();
        };

        timer::timer_event* ev = new timer::timer_event(us, timer_cb, this);
        asm("cli");
        state = thread_state::blocked;
        if(!blocked_timeout) {
            asm("sti");
            scheduler::yield();
        } else {
            state = thread_state::running;
            asm("sti");
        }
    }

    void thread::unblock() {
        time_slice = time_slice_default;
        if(state != thread_state::zombie) {
            state = thread_state::running;
        }
    }

    bool thread::block(thread_blocker* nblocker) {
        assert(check_interrupts());
        {
            auto lock = nblocker->get_lock();
            asm("cli");
            nblocker->set_thread(this);
            if(!nblocker->should_block()) {
                asm("sti");
                return true;
            }

            blocker = nblocker;
        }

        state = thread_state::blocked;
        asm("sti");
        scheduler::yield();
        blocker = nullptr;
        return !nblocker->was_interrupted();
    }

    bool thread::block(thread_blocker* nblocker, long& timeout) {
        assert(check_interrupts());

        timer::timer_callback_t timer_cb = [](void* t) {
            thread* thr = (thread *)t;
            thr->blocked_timeout = true;
            thr->unblock();
        };

        auto lock = nblocker->get_lock();
        if(!nblocker->should_block()) {
            return true;
        }

        blocked_timeout = false;
        blocker = nblocker;
        blocker->set_thread(this);
        timer::timer_event evt(timeout, timer_cb, this);
        asm("cli");
        lock = nullptr;
        state = thread_state::blocked;

        if(!blocked_timeout) {
            asm("sti");
            scheduler::yield();
        } else {
            // timed out before we even got here
            state = thread_state::running;
            asm("sti");
            blocker->interrupt();
        }

        blocker = nullptr;
        return !blocked_timeout && !nblocker->was_interrupted();
    }

    kstd::ref_counted<kstd::lock> thread_blocker::get_lock() {
        return kstd::ref_counted<kstd::lock>(new kstd::lock(_lock));
    }

    void thread_blocker::interrupt() {
        _interrupted = true;
        _should_block = false;
        kstd::lock l(_lock);
        if(_thread) {
            _thread->unblock();
        }
    }

    void thread_blocker::unblock() {
        _should_block = false;
        _removed = true;
        kstd::lock l(_lock);
        if(_thread) {
            _thread->unblock();
        }
    }
}