#pragma once

#include "fs/filesystems.h"
#include "drivers/gbd.h"

namespace ext2 {
    constexpr uint16_t SUPERBLOCK_LOCATION  = 1024; // bytes
    constexpr uint16_t SUPERBLOCK_SIZE      = 1024; // bytes, independent of block size
    constexpr uint16_t SUPER_MAGIC          = 0xEF53;
    constexpr uint8_t  MAX_NAME_LEN         = 255;
    constexpr uint8_t  ROOT_DIR_INODE       = 2;

    constexpr uint8_t  INODE_IND_BLOCK      = 12;
    constexpr uint8_t  INODE_DIND_BLOCK     = INODE_IND_BLOCK + 1;
    constexpr uint8_t  INODE_TIND_BLOCK     = INODE_DIND_BLOCK + 1;

    constexpr uint16_t S_IFSOCK             = 0xC000;
    constexpr uint16_t S_IFLNK              = 0xA000;
    constexpr uint16_t S_IFREG              = 0x8000;
    constexpr uint16_t S_IFBLK              = 0x6000;
    constexpr uint16_t S_IFDIR              = 0x4000;
    constexpr uint16_t S_IFCHR              = 0x2000;
    constexpr uint16_t S_IFIFO              = 0x1000;
}

fs_t* ext2_init(gbd_t* disk, uint32_t sector);