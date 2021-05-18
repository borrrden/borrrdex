#pragma once

#include <lock.h>
#include <fs/filesystem.h>
#include <fs/fs_node.h>
#include <kassert.h>
#include <klist.hpp>

namespace fs {
    class fs_watcher : public kstd::semaphore {
    public:
        fs_watcher()
            :kstd::semaphore(0)
        {

        }

        ~fs_watcher() {
            for(auto& fd : _watching) {
                fd->node->unwatch(*this);
                fd->node->close();
            }
        }

        inline void watch_node(fs::fs_node* node, int events) {
            fs::fs_fd_t* desc = node->open(0);
            assert(desc);

            desc->node->watch(*this, events);
            _watching.add(desc);
        }
    private:
        list<fs::fs_fd_t* > _watching;
    };
}