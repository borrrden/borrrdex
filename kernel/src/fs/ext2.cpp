#include "ext2.h"
#include "arch/x86_64/io/serial.h"
#include "paging/PageFrameAllocator.h"
#include "libk/string.h"
#include "memory/heap.h"
#include "KernelUtil.h"

#include <stdint.h>
#include <vector>

constexpr uint16_t EXT2_VALID_FS = 1;
constexpr uint16_t EXT2_ERROR_FS = 2;

constexpr uint16_t EXT2_ERRORS_CONTINUE = 1;
constexpr uint16_t EXT2_ERRORS_RO = 2;
constexpr uint16_t EXT2_ERRORS_PANIC = 3;

constexpr uint32_t EXT2_OS_LINUX = 0;
constexpr uint32_t EXT2_OS_HURD = 1;
constexpr uint32_t EXT2_OS_MASIX = 2;
constexpr uint32_t EXT2_OS_FREEBSD = 3;
constexpr uint32_t EXT2_OS_LITES = 4;
constexpr uint32_t EXT2_OS_BORRRDEX = 5;

constexpr uint32_t EXT2_GOOD_OLD_REV = 0;
constexpr uint32_t EXT2_DYNAMIC_REV = 1;

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
    /*
    32bit value used as index to the first inode useable for standard files. 
    In revision 0, the first non-reserved inode is fixed to 11 (EXT2_GOOD_OLD_FIRST_INO). 
    In revision 1 and later this value may be set to any value. 
    */
    uint32_t first_ino;

    /*
    16bit value indicating the size of the inode structure. 
    In revision 0, this value is always 128 (EXT2_GOOD_OLD_INODE_SIZE). 
    In revision 1 and later, this value must be a perfect power of 2 and 
    must be smaller or equal to the block size (1<<s_log_block_size). 
    */
    uint16_t inode_size;

    /*
    16bit value used to indicate the block group number hosting this superblock structure. 
    This can be used to rebuild the file system from any superblock backup. 
    */
    uint16_t block_group_nr;

    /*
    32bit bitmask of compatible features. The file system implementation is free to support 
    them or not without risk of damaging the meta-data. 
    */
    uint32_t feature_compat;

    /*
    32bit bitmask of incompatible features. The file system implementation should refuse to 
    mount the file system if any of the indicated feature is unsupported. 
    */
    uint32_t feature_incompat;

    /*
    32bit bitmask of “read-only” features. The file system implementation should mount as 
    read-only if any of the indicated feature is unsupported. 
    */
    uint32_t feature_ro_compat;

    /*
    128bit value used as the volume id. This should, as much as possible, be unique for each file system formatted. 
    */
    guid_t uuid;

    /*
    16 bytes volume name, mostly unusued. A valid volume name would consist of only ISO-Latin-1 characters and be 0 terminated. 
    */
    char volume_name[16];
} ext2_super_block_ext_t;

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
    uint32_t inode;
    uint16_t rec_len;
    uint8_t name_len;
    uint8_t file_type;
    char name[ext2::MAX_NAME_LEN];
} ext2_directory_entry_t;

typedef struct {
    // Where on the disk does the filesystem start
    uint64_t startsector;

    // The number of blocks [sectors] the filesystem occupies
    uint64_t totalsectors;

    // A handle to the abstracted disk for reads, writes, etc
    gbd_t* disk;

    ext2_super_block_t superblock;
    
    uint32_t block_size;

    uint32_t inode_size;

    uint32_t sectors_per_block;

    uint32_t bg_size;

    ext2_block_group_desc_t* group_desc;

    uint32_t group_desc_count;

    void* block_buffer;
} ext2_t;

typedef struct {
    uint32_t inode;
    ext2_inode_t inode_data;
} ext2_open_file_t;

inline bool S_ISDIR(ext2_inode_t* inode) { return (inode->i_mode & ext2::S_IFDIR) != 0; }
inline bool S_ISREG(ext2_inode_t* inode) { return (inode->i_mode & ext2::S_IFREG) != 0; }

static std::vector<ext2_open_file_t> s_open_files;

static gbd_request_t create_read_block_req(uint64_t start, uint64_t count, void* buffer) {
    gbd_request_t req;
    req.start = start;
    req.count = count;
    req.operation = GBD_OPERATION_READ;
    req.buf = buffer;

    return req;
}

static int efs_unmount(fs_t* fs) {
    ext2_t* internal = (ext2_t *)fs->internal;
    uint32_t buffer_page_count = (1024 << internal->superblock.log_block_size) / 0x1000;
    if(buffer_page_count == 0) {
        buffer_page_count = 1;
    }

    PageFrameAllocator::SharedAllocator()->FreePages(internal->block_buffer, buffer_page_count);
    kfree(internal->group_desc);
    kfree(fs);

    return VFS_OK;
}

static void* read_block(ext2_t* efs, uint32_t block_number) {
    gbd_request_t req = create_read_block_req(efs->startsector + block_number * efs->sectors_per_block, efs->sectors_per_block, efs->block_buffer);
    int r = efs->disk->read_block(efs->disk, &req);
    if(r == 0) {
        uart_printf("!! efs_open: read error at block 0x%x (sector 0x%x)\r\n", block_number, block_number * efs->sectors_per_block);
        return nullptr;
    }

    return efs->block_buffer;
}

static int64_t find_inode_in_dirlist(ext2_t* efs, uint32_t data_block, uint32_t size, const char* entry, uint8_t entry_len) {
    ext2_directory_entry_t* dirlist = (ext2_directory_entry_t *)read_block(efs, data_block);
    uint32_t pos = 0;
    while(pos < size) {
        if(dirlist->name_len == entry_len && memcmp(dirlist->name, entry, entry_len) == 0) {
            return dirlist->inode;
        }

        pos += dirlist->rec_len;
        dirlist = (ext2_directory_entry_t *)((uint64_t)dirlist + dirlist->rec_len);
    }

    return VFS_NOT_FOUND;
}

static ext2_inode_t* read_inode(ext2_t* efs, uint32_t inode) {
    inode--;
    uint32_t bg_number = inode / efs->superblock.inodes_per_group;
    uint32_t bg_offset = inode % efs->superblock.inodes_per_group;
    ext2_block_group_desc_t* gd = efs->group_desc + bg_number;
    uint32_t inode_table_block = gd->bg_inode_table;
    uint32_t inode_block_num = bg_offset * sizeof(ext2_inode_t) / efs->block_size;
    uint32_t inode_block_offset = bg_offset % (efs->block_size / sizeof(ext2_inode_t));
    uint32_t block_number = inode_table_block + inode_block_num;

    return (ext2_inode_t *)read_block(efs, block_number) + inode_block_offset;
}

static int64_t find_inode_in_directory(ext2_t* efs, uint32_t start_inode, const char* entry, uint8_t entry_len) {
    ext2_inode_t* inode = read_inode(efs, start_inode);
    if(!S_ISDIR(inode)) {
        return VFS_NOT_FOUND;
    }

    uint32_t* data_block = inode->i_block;
    while(*data_block != 0) {
        int64_t next_inode = find_inode_in_dirlist(efs, *data_block, inode->i_size, entry, entry_len);
        if(next_inode > 10) {
            return next_inode;
        }

        if(next_inode < 0 && next_inode != VFS_NOT_FOUND) {
            return next_inode;
        }

        data_block++;
    }

    return VFS_NOT_FOUND;
}

static int64_t find_inode(ext2_t* efs, const char* path) {
    const char* start, *current;
    start = current = path;
    uint32_t inode = ext2::ROOT_DIR_INODE;
    while(*current != 0) {
        current++, start++;
        while(*current != '/') {
            if(!*current) break;
            current++;
        }

        int64_t next = find_inode_in_directory(efs, inode, start, current - start);
        if(next < 0) {
            return next;
        }

        inode = (uint32_t)next;
        start = current;
    }

    return inode;
}

static int efs_open(fs_t* fs, const char* filename) {
    if(filename[0] != '/') {
        // Malformed
        return VFS_INVALID_PARAMS;
    }

    ext2_t* efs = (ext2_t *)fs->internal;
    uint32_t inode = find_inode(efs, filename);
    ext2_inode_t* inode_obj = read_inode(efs, inode);
    if(!S_ISREG(inode_obj)) {
        return VFS_NOT_FOUND;
    }

    for(const auto& entry : s_open_files) {
        if(entry.inode == inode) {
            return inode;
        }
    }

    s_open_files.push_back({inode, *inode_obj});
    return inode;
}

static int efs_close(fs_t* fs, int fileid) {
    // No-op for TFS
    fs = fs;
    
    for(auto entry = s_open_files.begin(); entry != s_open_files.end(); entry++) {
        if(entry->inode == fileid) {
            s_open_files.erase(entry);
            break;
        }
    }

    return VFS_OK;
}

static int efs_create(fs_t* fs, const char* filename, int size, int mask) {
    return VFS_NOT_SUPPORTED;
}

static int efs_remove(fs_t* fs, const char* filename) {
    return VFS_NOT_SUPPORTED;
}

static uint32_t find_data_block(ext2_t* efs, const ext2_inode_t* inode, uint32_t index) {
    uint32_t max = ext2::INODE_IND_BLOCK;
    if(index < max) {
        return inode->i_block[index];
    }

    uint32_t entries_per_block = efs->block_size / sizeof(uint32_t);
    max += entries_per_block;
    if(index < max) {
        // Single indirect lookup
        index -= ext2::INODE_IND_BLOCK;
        uint32_t* block = (uint32_t *)read_block(efs, inode->i_block[ext2::INODE_IND_BLOCK]);
        return *(block + index);
    }

    max = ext2::INODE_IND_BLOCK + entries_per_block * entries_per_block;
    if(index < max) {
        index -= ext2::INODE_IND_BLOCK;
        // Double indirect lookup
        uint32_t* indirect = (uint32_t *)read_block(efs, inode->i_block[ext2::INODE_DIND_BLOCK]);
        
        uint32_t subindex = index / entries_per_block;
        uint32_t suboffset = index % entries_per_block;
        indirect += subindex;

        uint32_t* block = (uint32_t *)read_block(efs, *indirect);
        return *(block + suboffset);
    }

    max += (efs->block_size / sizeof(uint32_t)) * efs->block_size * efs->block_size;
    if(index > max) {
        return VFS_ERROR;
    }

    // Triple indirect lookup
    index -= ext2::INODE_IND_BLOCK;
    uint32_t* indirect = (uint32_t *)read_block(efs, inode->i_block[ext2::INODE_TIND_BLOCK]);
    uint32_t subindex = index / (entries_per_block * entries_per_block);
    uint32_t suboffset = index % (entries_per_block * entries_per_block);
    indirect += subindex;

    indirect = (uint32_t *)read_block(efs, *indirect);
    subindex = index / entries_per_block;
    suboffset = index % entries_per_block;
    indirect += subindex;
    uint32_t* block = (uint32_t *)read_block(efs, *indirect);
    return *(block + suboffset);
}

static int find_data_blocks(ext2_t* efs, const ext2_inode_t* inode, uint32_t start, uint32_t end, uint32_t* block_list) {
    while(start < end) {
        *block_list = find_data_block(efs, inode, start++);
        if(*block_list < 0) {
            return *block_list;
        }

        block_list++;
    }

    return 0;
}

static int efs_read(fs_t* fs, int fileid, void* buffer, int bufsize, int offset) {
    ext2_t *efs = (ext2_t *)fs->internal;
    if(fileid < 10 || fileid > (int)efs->superblock.inode_count) {
        return VFS_ERROR;
    }

    const ext2_inode_t* inode = nullptr;
    for(const auto& entry : s_open_files) {
        if(entry.inode == fileid) {
            inode = &(entry.inode_data);
            break;
        }
    }

    if(!inode) {
        return VFS_NOT_OPEN;
    }

    if(offset + bufsize > inode->i_size) {
        return VFS_INVALID_PARAMS;
    }

    uint32_t start_block_num = offset / efs->block_size;
    uint32_t start_block_offset = offset % efs->block_size;
    uint32_t end_block_num = (offset + bufsize) / efs->block_size;
    uint32_t end_block_size = (offset + bufsize) % efs->block_size;
    uint32_t block_count = end_block_num - start_block_num;
    uint32_t block_list[block_count];
    int success = find_data_blocks(efs, inode, start_block_num, end_block_num, block_list);
    if(success != 0) {
        return success;
    }

    uint8_t* b = (uint8_t *)buffer;
    for(uint32_t i = 0; i < block_count; i++) {
        void* data = read_block(efs, block_list[i]);
        if(i == 0) {
            uint32_t size = efs->block_size - start_block_offset;
            memcpy(b, (uint8_t *)data + start_block_offset, size);
            b += size;
        } else if(i == block_count - 1) {
            memcpy(b, data, end_block_size);
        } else {
            memcpy(b, data, efs->block_size);
        }
    }

    return VFS_OK;
}

static int efs_write(fs_t* fs, int fileid, void* buffer, int datasize, int offset) {
    return VFS_NOT_SUPPORTED;
}

int efs_getfree(fs_t* fs) {
    ext2_t *efs = (ext2_t *)fs->internal;
    return efs->superblock.free_block_count * efs->block_size;
}

int efs_filecount(fs_t* fs, const char* dirname) {
    ext2_t *efs = (ext2_t *)fs->internal;
    return efs->superblock.inode_count - efs->superblock.free_inode_count;
}

int efs_file(fs_t* fs, const char* dirname, int idx, char* buffer) {
    if(dirname[0] != '/') {
        // Malformed
        return VFS_INVALID_PARAMS;
    }

    ext2_t *efs = (ext2_t *)fs->internal;
    uint32_t inode = find_inode(efs, dirname);
    ext2_inode_t* inode_obj = read_inode(efs, inode);
    if(!S_ISDIR(inode_obj)) {
        return VFS_INVALID_PARAMS;
    }

    uint32_t* data_block = inode_obj->i_block;
    int current = 0;
    uint32_t size = inode_obj->i_size;
    while(*data_block != 0) {
        ext2_directory_entry_t* dirlist = (ext2_directory_entry_t *)read_block(efs, *data_block);
        uint32_t pos = 0;
        while(pos < size) {
            if(current == idx) {
                strncpy(buffer, dirlist->name, dirlist->name_len);
                return VFS_OK;
            }

            pos += dirlist->rec_len;
            dirlist = (ext2_directory_entry_t *)((uint64_t)dirlist + dirlist->rec_len);
            current++;
        }

        data_block++;
    }

    return VFS_ERROR;
}

int efs_filesize(fs_t* fs, int fileid) {
    for(const auto& entry : s_open_files) {
        if(entry.inode == fileid) {
            return entry.inode_data.i_size;
        }
    }

    return VFS_NOT_OPEN;
}

fs_t* ext2_init(gbd_t* disk, uint32_t sector) {
    void* addr = PageFrameAllocator::SharedAllocator()->RequestPage();
    if(!addr) {
        uart_print("!! ext2_init: Could not allocate memory\r\n");
        return nullptr;
    }

    uint32_t sectorSize = disk->block_size(disk);
    int sector_no = ext2::SUPERBLOCK_LOCATION / sectorSize;
    int sector_count = (ext2::SUPERBLOCK_SIZE + sectorSize - 1) / sectorSize;
    gbd_request_t req = create_read_block_req(sector + sector_no, sector_count, addr);
    int r = disk->read_block(disk, &req);
    if(r == 0) {
        PageFrameAllocator::SharedAllocator()->FreePage(addr);
        uart_print("!! ext2_init: Error during disk read.  Intialization Failed\r\n");
        return nullptr;
    }

    ext2_super_block_t* superblock = sector_no == 0 
        ? (ext2_super_block_t *)((uint8_t *)addr + ext2::SUPERBLOCK_SIZE) 
        : (ext2_super_block_t *)addr;

    if(superblock->magic != ext2::SUPER_MAGIC || superblock->rev_level != 1) {
        PageFrameAllocator::SharedAllocator()->FreePage(addr);
        return nullptr;
    }

    uint32_t block_size = 1024 << superblock->log_block_size;
    uint32_t sectors_per_block = block_size / sectorSize;
    uint32_t inodes_per_block = block_size / sizeof(ext2_inode_t);
    uint32_t itable_blocks = superblock->inodes_per_group / inodes_per_block;

    ext2_super_block_ext_t* superblock_ext = (ext2_super_block_ext_t *)(superblock + 1);
    fs_t* fs = (fs_t *)kmalloc(sizeof(fs_t) + sizeof(ext2_t));
    ext2_t* efs = (ext2_t *)((uint8_t *)fs + sizeof(fs_t));
    efs->startsector = sector;
    efs->totalsectors = disk->total_blocks(disk);
    efs->disk = disk;
    efs->block_size = 1024 << superblock->log_block_size;
    efs->sectors_per_block = sectors_per_block;
    efs->superblock = *superblock;
    efs->inode_size = superblock_ext->inode_size;

    uint32_t bg_size = 3; // Superblock, inode bitmap, data bitmap;
    uint32_t gd_count = (superblock->block_count + superblock->blocks_per_group - 1) / superblock->blocks_per_group;
    uint32_t gd_block_count = (gd_count * sizeof(ext2_block_group_desc_t) + block_size - 1) / block_size;
    bg_size += gd_block_count; // Group desc
    bg_size += superblock->inodes_per_group * sizeof(ext2_inode_t) / block_size;
    bg_size += superblock->blocks_per_group;
    efs->bg_size = bg_size;

    // TMP: Tool for generating ext2 doesn't shadow copy group desc, so save them
    sector_no += sectors_per_block;
    sector_count = gd_block_count * sectors_per_block;
    req = create_read_block_req(sector + sector_no, sector_count, addr);
    r = disk->read_block(disk, &req);
    if(r == 0) {
        PageFrameAllocator::SharedAllocator()->FreePage(addr);
        uart_print("!! ext2_init: Error during disk read.  Intialization Failed\r\n");
        return nullptr;
    }

    efs->group_desc = (ext2_block_group_desc_t *)kmalloc(gd_count * sizeof(ext2_block_group_desc_t));
    efs->group_desc_count = gd_count;
    memcpy(efs->group_desc, addr, gd_count * sizeof(ext2_block_group_desc_t));

    if(block_size >= 4096) {
        PageFrameAllocator::SharedAllocator()->FreePage(addr);
        uint32_t pages_needed = block_size / 0x1000;
        addr = PageFrameAllocator::SharedAllocator()->RequestPages(pages_needed);
    }

    memset(addr, 0, block_size);
    efs->block_buffer = addr;

    fs->internal = efs;
    strncpy(fs->volume_name, superblock_ext->volume_name, 16);

    fs->unmount = efs_unmount;
    fs->open = efs_open;
    fs->close = efs_close;
    fs->create  = efs_create;
    fs->remove  = efs_remove;
    fs->read    = efs_read;
    fs->write   = efs_write;
    fs->getfree  = efs_getfree;
    fs->filecount = efs_filecount;
    fs->file      = efs_file;
    fs->filesize = efs_filesize;

    return fs; 
}