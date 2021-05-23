#pragma once

#include <stdint.h>
#include <fs/fs_volume.h>
#include <fs/fs_node.h>
#include <device.h>
#include <lru_cache.hpp>
#include <frg/std_compat.hpp>

typedef struct {
    uint16_t i_mode;
    uint16_t i_uid;
    uint32_t i_size;
    uint32_t i_atime;
    uint32_t i_ctime;
    uint32_t i_mtime;
    uint32_t i_dtime;
    uint16_t i_gid;
    uint16_t i_links_count;
    uint32_t i_blocks;
    uint32_t i_flags;
    uint32_t i_osd1;
    uint32_t i_block[15];
    uint32_t i_generation;
    uint32_t i_file_acl;
    uint32_t i_dir_acl;
    uint32_t i_faddr;
    uint8_t i_osd2[12];
} ext2_inode_t;

typedef struct {
    /*
    32bit value indicating the total number of inodes, both used and free, in the file system. 
    This value must be lower or equal to (s_inodes_per_group * number of block groups). 
    It must be equal to the sum of the inodes defined in each block group. 
    */
    uint32_t inode_count;

    /*
    32bit value indicating the total number of blocks in the system including all used, free and reserved. 
    This value must be lower or equal to (s_blocks_per_group * number of block groups). 
    It can be lower than the previous calculation if the last block group has a smaller number of
    blocks than s_blocks_per_group due to volume size. It must be equal to the sum of the blocks 
    defined in each block group. 
    */
    uint32_t block_count;

    /*
    32bit value indicating the total number of blocks reserved for the usage of the super user.
    This is most useful if for some reason a user, maliciously or not, fill the file system to capacity; 
    the super user will have this specified amount of free blocks at his disposal so he can edit 
    and save configuration files. 
    */
    uint32_t r_block_count;

    /*
    32bit value indicating the total number of free blocks, including the number of reserved blocks (see r_block_count). 
    This is a sum of all free blocks of all the block groups. 
    */
    uint32_t free_block_count;

    /*
    32bit value indicating the total number of free inodes. This is a sum of all free inodes of all the block groups. 
    */
    uint32_t free_inode_count;

    /*
     32bit value identifying the first data block, in other word the id of the block containing the superblock structure.
     Note that this value is always 0 for file systems with a block size larger than 1KB, and always 1 for file systems 
     with a block size of 1KB. The superblock is always starting at the 1024th byte of the disk, which normally happens 
     to be the first byte of the 3rd sector. 
    */
    uint32_t first_data_block;

    /*
     The block size is computed using this 32bit value as the number of bits to shift left the value 1024. 
     This value may only be non-negative.
     
     block size = 1024 << s_log_block_size;
     Common block sizes include 1KiB, 2KiB, 4KiB and 8Kib.
     Must be >= sector size
    */
    uint32_t log_block_size;

    /*
    The fragment size is computed using this 32bit value as the number of bits to shift left the value 1024. 
    Note that a negative value would shift the bit right rather than left.
    if( positive )
    fragment size = 1024 << s_log_frag_size;
    else
    framgent size = 1024 >> -s_log_frag_size;
    */
    int32_t log_frag_size;
    
    /*
    32bit value indicating the total number of blocks per group. This value in combination with first_data_block can 
    be used to determine the block groups boundaries. Due to volume size boundaries, the last block group might have a s
    maller number of blocks than what is specified in this field
    */
    uint32_t blocks_per_group;

    /*
    32bit value indicating the total number of fragments per group. 
    It is also used to determine the size of the block bitmap of each block group. 
    */
    uint32_t frags_per_group;

    /*
    32bit value indicating the total number of inodes per group. This is also used to determine the size of the
    inode bitmap of each block group. Note that you cannot have more than (block size in bytes * 8) inodes per 
    group as the inode bitmap must fit within a single block. This value must be a perfect multiple of the number 
    of inodes that can fit in a block ((1024<<s_log_block_size)/s_inode_size). 
    */
    uint32_t inodes_per_group;

    /*
    Unix time, as defined by POSIX, of the last time the file system was mounted. 
    */
    uint32_t mtime;

    /*
    Unix time, as defined by POSIX, of the last write access to the file system. 
    */
    uint32_t wtime;

    /*
    16bit value indicating how many time the file system was mounted since the last time it was fully verified. 
    */
    uint16_t mnt_count;

    /*
    16bit value indicating the maximum number of times that the file system may be mounted before a full check is performed. 
    */
    uint16_t max_mnt_count;

    /*
    16bit value identifying the file system as Ext2. The value is currently fixed to EXT2_SUPER_MAGIC of value 0xEF53. 
    */
    uint16_t magic;

    /*
    16bit value indicating the file system state. When the file system is mounted, this state is set to EXT2_ERROR_FS. 
    After the file system was cleanly unmounted, this value is set to EXT2_VALID_FS.
    When mounting the file system, if a valid of EXT2_ERROR_FS is encountered it means the file system was not cleanly 
    unmounted and most likely contain errors that will need to be fixed. Typically under Linux this means running fsck. 
    */
    uint16_t state;

    /*
    16bit value indicating what the file system driver should do when an error is detected. 
    */
    uint16_t errors;

    /*
    16bit value identifying the minor revision level within its revision level. 
    */
    uint16_t minor_rev_level;

    /*
    Unix time, as defined by POSIX, of the last file system check. 
    */
    uint32_t last_check;

    /*
    Maximum Unix time interval, as defined by POSIX, allowed between file system checks. 
    */
    uint32_t check_interval;

    /*
    32bit identifier of the os that created the file system
    */
    uint32_t creator_os;

    /*
    32bit revision level value. 
    */
    uint32_t rev_level;

    /*
    16bit value used as the default user id for reserved blocks. 
    */
    uint16_t def_resuid;

    /*
    16bit value used as the default group id for reserved blocks. 
    */
    uint16_t def_resgid;
} ext2_super_block_t;

typedef struct {
    uint32_t bg_block_bitmap;
    uint32_t bg_inode_bitmap;
    uint32_t bg_inode_table;
    uint16_t bg_free_blocks_count;
    uint16_t bg_free_inodes_count;
    uint16_t bg_used_dirs_count;
    uint16_t bg_pad;
    uint8_t bg_reserved[12];
} ext2_block_group_desc_t;

namespace fs {
    namespace ext2 {
        constexpr uint16_t SUPERBLOCK_LOCATION  = 1024; // bytes
        constexpr uint16_t SUPERBLOCK_SIZE      = 1024; // bytes, independent of block size
        constexpr uint16_t SUPER_MAGIC          = 0xEF53;
        constexpr uint8_t  MAX_NAME_LEN         = 255;
        constexpr uint8_t  ROOT_DIR_INODE       = 2;

        const uint16_t S_IFMT   = 0xF000;

        constexpr uint8_t  INODE_IND_BLOCK      = 12;
        constexpr uint8_t  INODE_DIND_BLOCK     = INODE_IND_BLOCK + 1;
        constexpr uint8_t  INODE_TIND_BLOCK     = INODE_DIND_BLOCK + 1;

        constexpr uint8_t FT_UNKNOWN    = 0;
        constexpr uint8_t FT_REG_FILE   = 1;
        constexpr uint8_t FT_DIR        = 2;
        constexpr uint8_t FT_CHRDEV     = 3;
        constexpr uint8_t FT_BLKDEV     = 4;
        constexpr uint8_t FT_FIFO       = 5;
        constexpr uint8_t FT_SOCK       = 6;
        constexpr uint8_t FT_SYMLINK    = 7;
    }

    class ext2_volume;

    class ext2_node : public fs::fs_node {
    public:
        ext2_node(ext2_volume* volume, ext2_inode_t& ino, ino_t inode);

        ssize_t read(size_t, size_t, uint8_t*) override;
        ssize_t write(size_t, size_t, uint8_t*) override;
        int read_dir(directory_entry*, uint32_t) override;
        fs_node* find_dir(const char*) override;

        int create(directory_entry*, uint32_t) override;
        int create_dir(directory_entry*, uint32_t) override;

        ssize_t read_link(char*, size_t) override;
        int link(fs_node*, directory_entry*) override;
        int unlink(directory_entry*, bool = false) override;
        int truncate(off_t) override;

        void close() override;
        void sync() override;

        inline const ext2_inode_t& ext2_inode() const { return _ext2_inode; }
    private:
        ext2_volume* _vol;
        size_t _link_count;
        ext2_inode_t _ext2_inode;
        fs_lock _flock;
    };

    class ext2_volume : public fs::fs_volume {
    public:
        ext2_volume(devices::partition_device* partition, ext2_super_block_t* sb, const char* name);

        ssize_t read(ext2_node*, size_t, size_t, uint8_t*);
        ssize_t write(ext2_node*, size_t, size_t, uint8_t*);
        int read_dir(ext2_node*, directory_entry*, uint32_t);
        fs_node* find_dir(ext2_node*, const char*);

        int create(ext2_node*, directory_entry*, uint32_t);
        int create_dir(ext2_node*, directory_entry*, uint32_t);

        ssize_t read_link(ext2_node*, char*, size_t);
        int link(ext2_node*, fs_node*, directory_entry*);
        int unlink(ext2_node*, directory_entry*, bool = false);
        int truncate(ext2_node*, off_t);

        void sync_node(ext2_node*);
        void clean_node(ext2_node*);

        int error() const override { return _error; }
    private:
        using block_cache_t = kstd::lru_cache<ino_t, uint8_t*, frg::hash<ino_t>, frg::stl_allocator>;
        using inode_cache_t = kstd::lru_cache<ino_t, ext2_node*, frg::hash<ino_t>, frg::stl_allocator>;

        int read_inode(ino_t num, ext2_inode_t& inode);
        int read_block(uint32_t block, void* buffer, bool cache = false);
        
        int64_t find_data_blocks(const ext2_inode_t& inode, uint32_t start, uint32_t end, uint32_t* block_list);
        int64_t find_data_block(const ext2_inode_t& inode, uint32_t index);
        void sync_inode(const ext2_inode_t& ext2_inode, uint32_t inode);
        uint64_t inode_lba(uint32_t inode);

        devices::partition_device* _partition;
        bool _file_type {false};
        bool _sparse {false};
        bool _large_files {false};
        ext2_super_block_t* _sb;
        uint32_t _block_group_count;
        ext2_block_group_desc_t* _block_groups;
        uint32_t _block_size;
        uint32_t _inode_size;
        block_cache_t _block_cache {50, LRU_CACHE_C_FREE};
        inode_cache_t _inode_cache {1600, LRU_CACHE_CPP_DELETE};
        int _error {0};
    };

    
}

fs::fs_volume* ext2_init(devices::partition_device* partition);