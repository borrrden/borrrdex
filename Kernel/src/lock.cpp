#include <lock.h>
#include <kassert.h>
#include <scheduler.h>

namespace kstd {

    
    bool semaphore::wait(long& timeout) {
        lock l(_lock);
        assert(check_interrupts());
        auto prev = __sync_fetch_and_sub(&_val, 1);
        if(prev <= 0) {
            auto* blocker = new semaphore_blocker(this);
            _blocked.push_back(blocker);
            return scheduler::get_current_thread()->block(blocker, timeout);
        }

        return true;
    }

    void semaphore::signal() {
        lock l(_lock);
        __sync_fetch_and_add(&_val, 1);
        if(!_blocked.empty()) {
            _blocked.front()->unblock();
        }
    }
}