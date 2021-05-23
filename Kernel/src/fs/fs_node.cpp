#include <fs/fs_node.h>
#include <logging.h>
#include <abi-bits/errno.h>
#include <fs/fs_watcher.h>

namespace fs {
    ssize_t fs_node::read(size_t off, size_t size, uint8_t* buf) {
        log::warning("fs_node::read called");
        return -ENOSYS;
    }

    ssize_t fs_node::write(size_t off, size_t size, uint8_t* buf) {
        log::warning("fs_node::write called");
        return -ENOSYS;
    }

    fs_fd_t* fs_node::open(size_t flags) {
        fs_fd_t* file_desc = new fs_fd_t();

        file_desc->pos = 0;
        file_desc->mode = flags;
        file_desc->node = this;

        _handle_count++;
        return file_desc;
    }

    void fs_node::close() {
        _handle_count--;
    }

    int fs_node::read_dir(directory_entry*, uint32_t) {
        if(!is_dir()) {
            return -ENOTDIR;
        }

        log::warning("fs_node::read_dir called");
        return -ENOSYS;
    }

    fs_node* fs_node::find_dir(const char* name) {
        log::warning("fs_node::find_dir called");
        return nullptr;
    }

    int fs_node::create(directory_entry*, uint32_t) {
        if(!is_dir()) {
            return -ENOTDIR;
        }

        log::warning("fs_node::create called");
        return -ENOSYS;
    }

    int fs_node::create_dir(directory_entry*, uint32_t) {
        if(!is_dir()) {
            return -ENOTDIR;
        }

        log::warning("fs_node::create_dir called");
        return -ENOSYS;
    }

    ssize_t fs_node::read_link(char* path_buffer, size_t buf_size) {
        if(!is_symlink()) {
            return -EINVAL;
        }

        log::warning("fs_node::read_link called");
        return -ENOSYS;
    }

    int fs_node::link(fs_node*, directory_entry*) {
        log::warning("fs_node::link called");
        return -ENOSYS;
    }

    int fs_node::unlink(directory_entry*, bool) {
        log::warning("fs_node::unlink called");
        return -ENOSYS;
    }

    int fs_node::truncate(off_t) {
        log::warning("fs_node::truncate called");
        return -ENOSYS;
    }

    int fs_node::ioctl(uint64_t cmd, uint64_t) {
        log::warning("fs_node::ioctl called (cmd: %d)", cmd);
        return -ENOSYS;
    }

    void fs_node::sync() {

    }

    void fs_node::watch(fs_watcher& watcher, int events) {
        log::warning("fs_node::watch called");
        watcher.signal();
    }

    void fs_node::unwatch(fs_watcher& watcher) {
        log::warning("fs_node::unwatch called");
    }
}