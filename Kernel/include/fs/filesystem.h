#pragma once

#include <types.h>
#include <klist.hpp>

typedef int64_t ino_t;
typedef uint64_t dev_t;
typedef int32_t uid_t;
typedef int64_t off_t;
typedef int32_t mode_t;
typedef int32_t nlink_t;
typedef int64_t volume_id_t;

namespace fs {
    constexpr uint8_t  NAME_MAX     = 255;
    constexpr uint16_t PATH_MAX     = 4096;
    constexpr uint8_t  SYMLINK_MAX  = 10;

    // Reused from ELF spec
    constexpr uint16_t S_IFMT   = 0xF000;
    constexpr uint16_t S_IFBLK  = 0x6000;
    constexpr uint16_t S_IFCHR  = 0x2000;
    constexpr uint16_t S_IFIFO  = 0x1000;
    constexpr uint16_t S_IFREG  = 0x8000;
    constexpr uint16_t S_IFDIR  = 0x4000;
    constexpr uint16_t S_IFLNK  = 0xA000;
    constexpr uint16_t S_IFSOCK = 0xC000;

    // More understandable names
    constexpr uint16_t FS_NODE_TYPE         = S_IFMT;
    constexpr uint16_t FS_NODE_FILE         = S_IFREG;
    constexpr uint16_t FS_NODE_DIRECTORY    = S_IFDIR;
    constexpr uint16_t FS_NODE_MOUNTPOINT   = S_IFDIR;
    constexpr uint16_t FS_NODE_BLKDEVICE    = S_IFBLK;
    constexpr uint16_t FS_NODE_SYMLINK      = S_IFLNK;
    constexpr uint16_t FS_NODE_CHARDEVICE   = S_IFCHR;
    constexpr uint16_t FS_NODE_SOCKET       = S_IFSOCK;

    // Flags for entries in a directory
    constexpr uint8_t DT_UNKNOWN    = 0;
    constexpr uint8_t DT_FIFO       = 1;
    constexpr uint8_t DT_CHR        = 2;
    constexpr uint8_t DT_DIR        = 4;
    constexpr uint8_t DT_BLK        = 6;
    constexpr uint8_t DT_REG        = 8;
    constexpr uint8_t DT_LNK        = 10;
    constexpr uint8_t DT_SOCK       = 12;
    constexpr uint8_t DT_WHT        = 14;

    // Filesystem events
    constexpr uint8_t POLLIN        = 0x01;
    constexpr uint8_t POLLOUT       = 0x02;
    constexpr uint8_t POLLPRI       = 0x04;
    constexpr uint8_t POLLHUP       = 0x08;
    constexpr uint8_t POLLERR       = 0x10;
    constexpr uint8_t POLLRDHUP     = 0x20;
    constexpr uint8_t POLLNVAL      = 0x40;
    constexpr uint8_t POLLWRNORM    = 0x80;

    class fs_node;
    class fs_volume;

    typedef struct {
        fs_node* node;
        off_t pos;
        mode_t mode;
    } fs_fd_t;

    void initialize();
    const list<fs_volume *>* get_volumes();
    fs_node* get_root();
    void register_volume(fs_volume* vol, bool add_root = true);

    fs_node* resolve_path(const char* path, const char* working_dir = nullptr, bool follow_symlink = true);
    fs_node* resolve_path(const char* path, fs_node* working_dir, bool follow_symlink = true);

    fs_node* find_dir(fs_node* parent, const char* name);
    ssize_t read(fs_node* node, size_t off, size_t size, void* buffer);
    ssize_t read(fs_fd_t* handle, size_t size, uint8_t* buffer);
    ssize_t write(fs_node* node, size_t off, size_t size, void* buffer);
    ssize_t write(fs_fd_t* handle, size_t size, uint8_t* buffer);
    fs_fd_t* open(fs_node* node, uint32_t flags);
    void close(fs_node* node);
    void close(fs_fd_t* fd);
}