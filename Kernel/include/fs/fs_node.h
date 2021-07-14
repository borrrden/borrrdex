#pragma once

#include <spinlock.h>
#include <stddef.h>
#include <fs/filesystem.h>

#include <abi-bits/abi.h>
#include <abi-bits/fcntl.h>
#include <types.h>

#include <type_traits>

namespace fs {
    class directory_entry;
    class fs_watcher;
    class fs_blocker;

    class fs_node {
    public:
        ino_t inode {0};
        uid_t uid {0};
        volume_id_t volume_id {0};
        size_t size {0};
        int nlink {0};
        uint32_t flags {0};
        fs_node* parent {nullptr};

        virtual ~fs_node() {}

        virtual ssize_t read(size_t off, size_t size, uint8_t* buf);
        virtual ssize_t write(size_t off, size_t size, uint8_t* buf);
        virtual fs_fd_t* open(size_t flags);
        virtual void close();

        virtual int read_dir(directory_entry*, uint32_t);
        virtual fs_node* find_dir(const char* name);

        virtual int create(directory_entry* ent, uint32_t mode);
        virtual int create_dir(directory_entry* ent, uint32_t mode);

        virtual ssize_t read_link(char* path_buffer, size_t buf_size);
        virtual int link(fs_node*, directory_entry*);
        virtual int unlink(directory_entry*, bool unlink_directories = false);

        virtual int truncate(off_t length);

        virtual int ioctl(uint64_t cmd, uint64_t arg);
        virtual void sync();

        virtual bool can_read() const { return true; }
        virtual bool can_write() const { return true; }

        virtual void watch(fs_watcher& watcher, int events);
        virtual void unwatch(fs_watcher& watcher);

        void add_handle() { _handle_count++; }
        void remove_handle() { _handle_count--; }

        inline void lock() { acquire_lock(&_blocked_lock); }
        inline bool try_lock() { return acquire_test_lock(&_blocked_lock); }
        inline void unlock() { release_lock(&_blocked_lock); }
        list<fs::fs_blocker *>* blocked() { return &_blocked; }

        virtual inline bool is_file() const { return (flags & FS_NODE_TYPE) == FS_NODE_FILE; }
        virtual inline bool is_dir() const { return (flags & FS_NODE_TYPE) == FS_NODE_DIRECTORY; }
        virtual inline bool is_block_dev() const { return (flags & FS_NODE_TYPE) == FS_NODE_BLKDEVICE; }
        virtual inline bool is_symlink() const { return (flags & FS_NODE_TYPE) == FS_NODE_SYMLINK; }
        virtual inline bool is_char_dev() const { return (flags & FS_NODE_TYPE) == FS_NODE_CHARDEVICE; }
        virtual inline bool is_socket() const { return (flags & FS_NODE_TYPE) == FS_NODE_SOCKET; }
    protected:
        fs_node* _link {nullptr};
        unsigned _handle_count {0};
        lock_t _blocked_lock {0};
        list<fs::fs_blocker *> _blocked;
    };
}
