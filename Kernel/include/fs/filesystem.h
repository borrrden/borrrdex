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
    constexpr uint8_t NAME_MAX = 255;
    constexpr uint16_t PATH_MAX = 4096;

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
    void register_volume(fs_volume* vol);

    fs_node* find_dir(fs_node* parent, const char* name);
    ssize_t read(fs_node* node, size_t off, size_t size, void* buffer);
}