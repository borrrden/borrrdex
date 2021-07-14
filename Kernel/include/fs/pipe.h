#pragma once

#include <fs/fs_node.h>
#include <fs/fs_watcher.h>
#include <ref_counted.hpp>
#include <stream.h>
#include <frg/intrusive.hpp>

namespace fs {
    class unix_pipe final : public fs_node {
    public:
        unix_pipe(int end, kstd::ref_counted<kstd::data_stream> stream);

        ssize_t read(size_t off, size_t size, uint8_t* buffer) override;
        ssize_t write(size_t off, size_t size, uint8_t* buffer) override;

        void watch(fs::fs_watcher& watcher, int events) override;
        void unwatch(fs::fs_watcher& watcher) override;

        void close() override;

        static void create_pipe(unix_pipe*& read, unix_pipe*& write);

    protected: 
        enum {
            InvalidPipe,
            ReadEnd,
            WriteEnd
        } _end = InvalidPipe;

        bool _widowed { false };
        unix_pipe* _other_end { nullptr };
        kstd::ref_counted<kstd::data_stream> _stream;

        struct watch_item {
            fs::fs_watcher& watcher;
            frg::default_list_hook<watch_item> hook;
        };

        frg::intrusive_list<watch_item, frg::locate_member<watch_item, frg::default_list_hook<watch_item>, &watch_item::hook>> _watching;
        lock_t _watching_lock {0};
    };
}