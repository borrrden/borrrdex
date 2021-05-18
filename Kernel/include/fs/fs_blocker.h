#pragma once

#include <thread.h>
#include <lock.h>
#include <fs/fs_node.h>

namespace fs {
    class fs_blocker : public threading::thread_blocker {
    public:
        fs_blocker(fs_node* node, size_t len = 1)
            :_node(node)
            ,_requested_len(len)
        {
            _lock = 1;
            node->lock();
            node->blocked()->add(this);
            node->unlock();
            release_lock(&_lock);
        }

        ~fs_blocker();

        void interrupt() override;

        inline void unblock() override {
            _should_block = false;
            kstd::lock l(_lock);
            if(_node && !_removed) {
                // Caller should lock the node first
                _node->blocked()->remove(this);
                _removed = true;
            }

            if(_thread) {
                _thread->unblock();
            }
        }
    private:
        fs_node* _node;
        size_t _requested_len;
    };
}