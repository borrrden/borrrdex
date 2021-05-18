#include <fs/fs_blocker.h>
#include <scheduler.h>

namespace fs {
    void fs_blocker::interrupt() {
        _should_block = false;
        _interrupted = true;

retry:
        acquire_lock(&_lock);
        if(_node && !_removed) {
            if(!_node->try_lock()) {
                release_lock(&_lock);
                scheduler::yield();
                goto retry;
            }

            _node->blocked()->remove(this);
            _removed = true;

            _node->unlock();
        }

        release_lock(&_lock);
    }

    fs_blocker::~fs_blocker() {
retry:
        acquire_lock(&_lock);
        if(_node && !_removed) {
            if(!_node->try_lock()) {
                release_lock(&_lock);
                goto retry;
            }

            _node->blocked()->remove(this);
            _node->unlock();
        }

        release_lock(&_lock);
    }
}