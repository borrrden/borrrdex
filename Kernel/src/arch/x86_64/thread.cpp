#include <thread.h>
#include <kassert.h>
#include <timer_event.h>
#include <scheduler.h>

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
}