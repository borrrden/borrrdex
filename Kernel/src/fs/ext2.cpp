#include <fs/ext2.h>
#include <liballoc/liballoc.h>
#include <logging.h>
#include <kmath.h>
#include <abi-bits/errno.h>
#include <ref_counted.hpp>
#include <debug.h>
#include <lru_cache.hpp>

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

constexpr uint32_t EXT2_INCOMPAT_COMPRESSION    = 0x1;
constexpr uint32_t EXT2_INCOMPAT_FILETYPE       = 0x2;
constexpr uint32_t EXT2_INCOMPAT_RECOVER        = 0x4;
constexpr uint32_t EXT2_INCOMPAT_JOURNAL_DEV    = 0x8;
constexpr uint32_t EXT2_INCOMPAT_META_BG        = 0x10;

constexpr uint32_t EXT2_COMPAT_DIR_PREALLOC     = 0x1;
constexpr uint32_t EXT2_COMPAT_IMAGIC_INODES    = 0x2;
constexpr uint32_t EXT2_COMPAT_JOURNAL          = 0x4;
constexpr uint32_t EXT2_COMPAT_XATTRS           = 0x8;
constexpr uint32_t EXT2_COMPAT_RESIZE_INODE     = 0x10;
constexpr uint32_t EXT2_COMPAT_DIR_INDEXING     = 0x20;

constexpr uint32_t EXT2_RO_SPARSE               = 0x1;
constexpr uint32_t EXT2_RO_LARGE_FILES          = 0x2;
constexpr uint32_t EXT2_RO_BINARY_TREE          = 0x4;

constexpr uint32_t EXT2_INCOMPAT_FEATURE_SUPPORT    = EXT2_INCOMPAT_FILETYPE;
constexpr uint32_t EXT2_RO_FEATURE_SUPPORT          = EXT2_RO_SPARSE | EXT2_RO_LARGE_FILES;

constexpr uint32_t DISK_READ_ERROR = 1;
constexpr uint32_t INCOMPATIBLE_ERROR = 5;

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
    uint32_t inode;
    uint16_t rec_len;
    uint8_t name_len;
    uint8_t file_type;
    char name[fs::ext2::MAX_NAME_LEN];
} ext2_directory_entry_t;

namespace fs {
    inline static uint32_t location_to_block(uint64_t l, uint32_t log_block_size) {
        return (1 >> log_block_size) >> 10;
    }

    inline static uint64_t block_to_lba(uint64_t block, uint32_t block_size, uint32_t sector_size) {
        return block * (block_size / sector_size);
    }

    uint64_t ext2_volume::inode_lba(uint32_t inode) {
        uint32_t block = _block_groups[(inode - 1) / _sb->inodes_per_group].bg_inode_table;
        return (block * _block_size + ((inode - 1) % _sb->inodes_per_group) * _inode_size) / _partition->parent()->block_size();
    }

    ext2_volume::ext2_volume(devices::partition_device* partition, ext2_super_block_t* sb, const char* name)
        :_partition(partition)
        ,_sb(sb)
    {
        ext2_super_block_ext_t* sbe = (ext2_super_block_ext_t *)(sb + 1);
        if(sb->rev_level != 0) {
            if(sbe->feature_incompat & (~EXT2_INCOMPAT_FILETYPE) || sbe->feature_ro_compat & (~EXT2_RO_FEATURE_SUPPORT)) {
                log::error("[ext2] Incompatible ext2 features present (Incompt: %x, Read-only Compt: %x)", 
                    sbe->feature_incompat, sbe->feature_ro_compat);
                _error = INCOMPATIBLE_ERROR;
                return;
            }

            _file_type = (sbe->feature_incompat & EXT2_INCOMPAT_FILETYPE) != 0;
            _large_files = (sbe->feature_ro_compat & EXT2_RO_LARGE_FILES) != 0;
            _sparse = (sbe->feature_ro_compat & EXT2_RO_SPARSE) != 0;
        } else {
            memset(sbe, 0, sizeof(ext2_super_block_ext_t));
        }

        _block_group_count = (sb->block_count + sb->blocks_per_group - 1) / sb->blocks_per_group;
        _block_size = 1024 << sb->log_block_size;
        _inode_size = sb->rev_level > 0 ? sbe->inode_size : 128;

        // Block groups start at the next block after the superblock
        uint64_t block_group_lba = block_to_lba(location_to_block(fs::ext2::SUPERBLOCK_LOCATION, sb->log_block_size) + 1, 
            _block_size, _partition->parent()->block_size());
        
        _block_groups = (ext2_block_group_desc_t *)_partition->read_bytes(block_group_lba, _block_group_count * sizeof(ext2_block_group_desc_t));
        if(!_block_groups) {
            log::error("[ext2] Disk error initializing volume");
            _error = DISK_READ_ERROR;
            return;
        }

        ext2_inode_t root;
        if(read_inode(fs::ext2::ROOT_DIR_INODE, root) < 0) {
            log::error("[ext2] Disk error initializing volume");
            _error = DISK_READ_ERROR;
            return;
        }

        ext2_node* ext2_mount_point = new ext2_node(this, root, fs::ext2::ROOT_DIR_INODE);
        _mount_point = ext2_mount_point;

        _mount_point_entry.node = _mount_point;
        _mount_point_entry.flags = DT_DIR;
        _mount_point_entry.set_name(name);
    }

    int ext2_volume::read_block(uint32_t block_num, void* buffer, bool cache) {
        if(block_num > _sb->block_count) {
            return -1;
        }

        auto do_disk_read = [&](void* b) {
            uint32_t sectors_per_block = _block_size / _partition->parent()->block_size();
            return _partition->read_block(block_num * sectors_per_block, sectors_per_block, b);
        };

        
        if(cache) {
            uint8_t* cached_block;
            if(!_block_cache.get(block_num, cached_block)) {
                cached_block = (uint8_t *)malloc(_block_size);
                if(int e = do_disk_read(cached_block) < 0) {
                    free(cached_block);
                    return e;
                }

                _block_cache.set(block_num, cached_block);
            }

            memcpy(buffer, cached_block, _block_size);
            return 1;
        } 
        
        return do_disk_read(buffer);
    }

    int ext2_volume::read_inode(ino_t num, ext2_inode_t& inode) {
        num--;
        uint32_t bg_number = num / _sb->inodes_per_group;
        uint32_t bg_offset = num % _sb->inodes_per_group;
        ext2_block_group_desc_t* gd = _block_groups + bg_number;
        uint32_t inode_table_block = gd->bg_inode_table;
        uint32_t inode_block_num = bg_offset * _inode_size / _block_size;
        uint32_t inode_block_offset = bg_offset % (_block_size / _inode_size);
        uint32_t block_number = inode_table_block + inode_block_num;

        void* val = malloc(_block_size);
        if(int e = read_block(block_number, val) < 0) {
            free(val);
            log::error("[ext2] Disk error (%d) reading inode %d", e, num);
            return e;
        }

        ext2_inode_t* indexed = (ext2_inode_t *)kstd::offset_by(val, _inode_size * inode_block_offset);
        inode = *indexed;
        free(val);
        return 0;
    }

    int64_t ext2_volume::find_data_blocks(const ext2_inode_t& inode, uint32_t start, uint32_t end, uint32_t* block_list) {
        while(start < end) {
            auto found_block = find_data_block(inode, start++);
            if(found_block < 0) {
                return found_block;
            }

            *block_list++ = (uint32_t)found_block;
        }

        return 0;
    }

    int64_t ext2_volume::find_data_block(const ext2_inode_t& inode, uint32_t index) {
        uint32_t max = ext2::INODE_IND_BLOCK;
        if(index < max) {
            return inode.i_block[index];
        }

        kstd::auto_free<uint32_t> block_buffer(_block_size / sizeof(uint32_t));
        uint32_t entries_per_block = _block_size / sizeof(uint32_t);
        max += entries_per_block;
        if(index < max) {
            // Single indirect lookup
            index -= ext2::INODE_IND_BLOCK;
            if(int e = read_block(inode.i_block[ext2::INODE_IND_BLOCK], block_buffer, true) < 0) {
                return e;
            }

            return (int64_t)*(block_buffer + index);
        }

        max = ext2::INODE_IND_BLOCK + entries_per_block * entries_per_block;
        if(index < max) {
            // Double indirect lookup
            index -= ext2::INODE_IND_BLOCK;
            if(int e = read_block(inode.i_block[ext2::INODE_DIND_BLOCK], block_buffer, true) < 0) {
                return e;
            }

            uint32_t subindex = index / entries_per_block;
            uint32_t suboffset = index % entries_per_block;
            uint32_t* indirect = block_buffer + subindex;

            if(int e = read_block(*indirect, block_buffer) < 0) {
                return e;
            }

            return (int64_t)*(block_buffer + suboffset);
        }

        max += (_block_size / sizeof(uint32_t)) * _block_size * _block_size;
        if(index > max) {
            return -EINVAL;
        }

        // Triple indirect lookup
        index -= ext2::INODE_IND_BLOCK;
        if(int e = read_block(inode.i_block[ext2::INODE_TIND_BLOCK], block_buffer, true) < 0) {
            return e;
        }

        uint32_t subindex = index / (entries_per_block * entries_per_block);
        uint32_t suboffset = index % (entries_per_block * entries_per_block);
        uint32_t* indirect = block_buffer + subindex;
        if(int e = read_block(*indirect, block_buffer, true) < 0) {
            return e;
        }

        subindex = index / entries_per_block;
        suboffset = index % entries_per_block;
        indirect = block_buffer + subindex;
        if(int e = read_block(*indirect, block_buffer, true) < 0) {
            return e;
        }

        return (int64_t)*(block_buffer + suboffset);
    }

    ssize_t ext2_volume::read(ext2_node* node, size_t offset, size_t size, uint8_t* buffer) {
        if(offset >= node->size) {
            return 0;
        }

        if(offset + size > node->size) {
            size = node->size - offset;
        }

        uint32_t start_block_num = offset / _block_size;
        uint32_t start_block_offset = offset % _block_size;
        uint32_t end_block_num = kstd::intervals_needed(offset + size, (uint64_t)_block_size);
        uint32_t end_block_size = (offset + size) % _block_size;
        uint32_t block_count = end_block_num - start_block_num;
        uint32_t block_list[block_count];
        auto success = find_data_blocks(node->ext2_inode(), start_block_num, end_block_num, block_list);
        if(success != 0) {
            return success;
        }

        uint8_t* b = buffer;
        kstd::auto_free<uint8_t> data(_block_size);
        for(uint32_t i = 0; i < block_count; i++) {
            if(int e = read_block(block_list[i], data, true) < 0) {
                return e;
            }

            if(i == 0) {
                uint32_t first_size = kstd::min(_block_size - start_block_offset, (unsigned)size);
                memcpy(b, data + start_block_offset, first_size);
                b += first_size;
            } else if(i == block_count - 1) {
                memcpy(b, data, end_block_size);
            } else {
                memcpy(b, data, _block_size);
                b += _block_size;
            }
        }

        return size;
    }

    ssize_t ext2_volume::write(ext2_node* node, size_t offset, size_t size, uint8_t* buffer) {
        return -ENOSYS;
    }

    int ext2_volume::read_dir(ext2_node* node, directory_entry* ent, uint32_t index) {
        if(!node->is_dir()) {
            return -ENOTDIR;
        }

        if(!node->inode) {
            log::warning("[ext2] ext2_volume::read_dir: Invalid inode");
            return -EINVAL;
        }

        uint8_t buffer[_block_size];
        uint32_t current_block = 0;
        uint32_t block_offset = 0;
        uint32_t total_offset = 0;
        ext2_directory_entry_t* ext2_dirent = (ext2_directory_entry_t *)buffer;
        int64_t block_num = find_data_block(node->ext2_inode(), current_block);
        if(block_num < 1) {
            log::warning("[ext2] Failed to read entry %d of inode %d", current_block, node->inode);
            _error = DISK_READ_ERROR;
            return -1;
        }

        if(int e = read_block((uint32_t)block_num, buffer) < 0) {
            log::warning("[ext2] Failed to read block %d", block_num);
            _error = DISK_READ_ERROR;
            return -1;
        }

        for(unsigned i = 0; i < index; i++) {
            if(ext2_dirent->rec_len == 0) {
                return 0;
            }

            block_offset += ext2_dirent->rec_len;
            total_offset += ext2_dirent->rec_len;

            if(ext2_dirent->rec_len < 8) {
                IF_DEBUG(debug_level_ext2 >= debug::LEVEL_NORMAL, {
                    log::warning("[ext2] Error (inode: %d) record length of directory entry has invalid length (%d)!", 
                        node->inode, ext2_dirent->rec_len);
                })

                break;
            }

            if(ext2_dirent->inode == 0) {
                // Ignore this kind of entry...
                index--;
            }

            if(block_offset > _block_size) {
                current_block++;
                if(current_block >= node->ext2_inode().i_blocks / (_block_size / _partition->parent()->block_size())) {
                    // End of directory
                    return 0;
                }

                block_offset = 0;
                block_num = find_data_block(node->ext2_inode(), current_block);
                if(block_num < 1) {
                    log::warning("[ext2] Failed to read entry %d of inode %d", current_block, node->inode);
                    _error = DISK_READ_ERROR;
                    return -1;
                }

                if(int e = read_block((uint32_t)block_num, buffer) < 0) {
                    log::warning("[ext2] Failed to read block %d", block_num);
                    _error = DISK_READ_ERROR;
                    return -1;
                }
            }

            ext2_dirent = (ext2_directory_entry_t *)(buffer + block_offset);
        }

        ent->set_name(ext2_dirent->name, ext2_dirent->name_len);
        switch(ext2_dirent->file_type) {
            case ext2::FT_REG_FILE:
                ent->flags = DT_REG;
                break;
            case ext2::FT_DIR:
                ent->flags = DT_DIR;
                break;
            case ext2::FT_CHRDEV:
                ent->flags = DT_CHR;
                break;
            case ext2::FT_BLKDEV:
                ent->flags = DT_BLK;
                break;
            case ext2::FT_FIFO:
                ent->flags = DT_FIFO;
                break;
            case ext2::FT_SOCK:
                ent->flags = DT_SOCK;
                break;
            case ext2::FT_SYMLINK:
                ent->flags = DT_LNK;
                break;
        }

        return 1;
    }

    fs_node* ext2_volume::find_dir(ext2_node* node, const char* name) {
        if(!node->is_dir()) {
            return nullptr;
        }

        if(!node->inode) {
            log::warning("[ext2] ext2_volume::find_dir: Invalid inode");
            return nullptr;
        }

        uint8_t buffer[_block_size];
        uint32_t current_block = 0;
        uint32_t block_offset = 0;
        uint32_t total_offset = 0;
        ext2_directory_entry_t* ext2_dirent = (ext2_directory_entry_t *)buffer;
        int64_t block_num = find_data_block(node->ext2_inode(), current_block);
        if(block_num < 1) {
            log::warning("[ext2] Failed to read entry %d of inode %d", current_block, node->inode);
            _error = DISK_READ_ERROR;
            return nullptr;
        }

        if(int e = read_block((uint32_t)block_num, buffer, true) < 0) {
            log::warning("[ext2] Failed to read block %d", block_num);
            _error = DISK_READ_ERROR;
            return nullptr;
        }

        while(current_block < node->ext2_inode().i_blocks / (_block_size / _partition->parent()->block_size())) {
            if(ext2_dirent->rec_len < 8) {
                IF_DEBUG(debug_level_ext2 >= debug::LEVEL_NORMAL, {
                    log::warning("[ext2] Error (inode: %d) record length of directory entry has invalid length (%d)!", 
                        node->inode, ext2_dirent->rec_len);
                })

                break;
            }

            if(ext2_dirent->inode > 0) {
                IF_DEBUG(debug_level_ext2 >= debug::LEVEL_VERBOSE, {
                    char buf[ext2_dirent->name_len + 1];
                    strncpy(buf, ext2_dirent->name, ext2_dirent->name_len);
                    buf[ext2_dirent->name_len] = 0;
                    log::info("Checking name '%s' (name len %d), inode %d, len %d (parent inode: %d)", 
                        buf, ext2_dirent->name_len, ext2_dirent->inode, ext2_dirent->rec_len, node->inode);
                })

                if(strnlen(name, fs::NAME_MAX) == ext2_dirent->name_len && strncmp(name, ext2_dirent->name, ext2_dirent->name_len) == 0) {
                    break;
                }
            }

            block_offset += ext2_dirent->rec_len;
            total_offset += ext2_dirent->rec_len;

            if(total_offset > node->ext2_inode().i_size) {
                return nullptr;
            }

            if(block_offset >= _block_size) {
                current_block++;
                if(current_block >= node->ext2_inode().i_blocks / (_block_size / _partition->parent()->block_size())) {
                    // End of directory
                    return nullptr;
                }

                block_offset = 0;
                block_num = find_data_block(node->ext2_inode(), current_block);
                if(block_num < 1) {
                    log::warning("[ext2] Failed to read entry %d of inode %d", current_block, node->inode);
                    _error = DISK_READ_ERROR;
                    return nullptr;
                }

                if(int e = read_block((uint32_t)block_num, buffer, true) < 0) {
                    log::warning("[ext2] Failed to read block %d", block_num);
                    _error = DISK_READ_ERROR;
                    return nullptr;
                }
            }

            ext2_dirent = (ext2_directory_entry_t *)(buffer + block_offset);
        }

        if(strnlen(name, fs::NAME_MAX) != ext2_dirent->name_len || strncmp(name, ext2_dirent->name, ext2_dirent->name_len) != 0) {
            // Not found
            return nullptr;
        }

        if(!ext2_dirent->inode || ext2_dirent->inode > _sb->inode_count) {
            log::error("[ext2] Directory entry %s contains invalid inode %d", name, ext2_dirent->inode);
            return nullptr;
        }

        ext2_node* ret_node;
        if(!_inode_cache.get(ext2_dirent->inode, ret_node)) {
            ext2_inode_t dirent_inode;
            if(read_inode(ext2_dirent->inode, dirent_inode) < 0) {
                log::error("[ext2] Failed to read inode of directory (inode %d) entry %s", ext2_dirent->inode, name);
                return nullptr;
            }

            IF_DEBUG(debug_level_ext2 >= debug::LEVEL_VERBOSE, {
                log::info("[ext2] Opening inode %d, size: %d", ext2_dirent->inode, dirent_inode.i_size);
            })

            ret_node = new ext2_node(this, dirent_inode, ext2_dirent->inode);
            _inode_cache.set(ext2_dirent->inode, ret_node);
        }
        
        return ret_node;
    }

    void ext2_volume::sync_inode(const ext2_inode_t& ext2_inode, uint32_t inode) {
        
    }

    void ext2_volume::sync_node(ext2_node* node) {
        sync_inode(node->ext2_inode(), (uint32_t)node->inode);
    }

    int ext2_volume::create(ext2_node* node, directory_entry* ent, uint32_t mode) {
        return -ENOSYS;
    }

    int ext2_volume::create_dir(ext2_node* node, directory_entry* ent, uint32_t mode) {
        return -ENOSYS;
    }

    ssize_t ext2_volume::read_link(ext2_node* node, char* path_buffer, size_t size) {
        return -ENOSYS;
    }

    int ext2_volume::link(ext2_node* node, fs_node* file, directory_entry *ent) {
        return -ENOSYS;
    }

    int ext2_volume::unlink(ext2_node* node, directory_entry* ent, bool unlink_dirs) {
        return -ENOSYS;
    }

    int ext2_volume::truncate(ext2_node* node, off_t length) {
        return -ENOSYS;
    }

    void ext2_volume::clean_node(ext2_node* node) {

    }

    ext2_node::ext2_node(ext2_volume* vol, ext2_inode_t& ino, ino_t inode) 
        :_vol(vol)
        ,_link_count(ino.i_links_count)
        ,_ext2_inode(ino)
    {
        this->inode = inode;
        volume_id = vol->get_volume_id();
        uid = ino.i_uid;
        size = ino.i_size;

        switch(ino.i_mode & ext2::S_IFMT) {
            case S_IFBLK:
                flags = FS_NODE_BLKDEVICE;
                break;
            case S_IFCHR:
                flags = FS_NODE_CHARDEVICE;
                break;
            case S_IFDIR:
                flags = FS_NODE_DIRECTORY;
                break;
            case S_IFLNK:
                flags = FS_NODE_SYMLINK;
                break;
            case S_IFSOCK:
                flags = FS_NODE_SOCKET;
                break;
            default:
                flags = FS_NODE_FILE;
                break;
        }
    }

    int ext2_node::read_dir(directory_entry* ent, uint32_t index) {
        _flock.acquire_read();
        auto ret = _vol->read_dir(this, ent, index);
        _flock.release_read();
        return ret;
    }

    fs_node* ext2_node::find_dir(const char* name) {
        _flock.acquire_read();
        auto ret = _vol->find_dir(this, name);
        _flock.release_read();
        return ret;
    }

    ssize_t ext2_node::read(size_t offset, size_t size, uint8_t* buffer) {
        _flock.acquire_read();
        auto ret = _vol->read(this, offset, size, buffer);
        _flock.release_read();
        return ret;
    }

    ssize_t ext2_node::write(size_t offset, size_t size, uint8_t* buffer) {
        _flock.acquire_read();
        auto ret = _vol->write(this, offset, size, buffer);
        _flock.release_read();
        return ret;
    }

    int ext2_node::create(directory_entry* ent, uint32_t mode) {
        _flock.acquire_write();
        auto ret = _vol->create(this, ent, mode);
        _flock.acquire_write();
        return ret;
    }

    int ext2_node::create_dir(directory_entry* ent, uint32_t mode) {
        _flock.acquire_write();
        auto ret = _vol->create_dir(this, ent, mode);
        _flock.acquire_write();
        return ret;
    }

    ssize_t ext2_node::read_link(char* path_buffer, size_t size) {
        _flock.acquire_read();
        auto ret = _vol->read_link(this, path_buffer, size);
        _flock.release_read();
        return ret;
    }

    int ext2_node::link(fs_node* node, directory_entry* entry) {
        if(!node->inode) {
            log::warning("[ext2] ext2_node::link Invalid inode");
            return -EINVAL;
        }

        _flock.acquire_write();
        auto ret = _vol->link(this, (ext2_node *)node, entry);
        _flock.acquire_write();
        return ret;
    }

    int ext2_node::unlink(directory_entry* ent, bool unlink_directories) {
        _flock.acquire_write();
        auto ret = _vol->unlink(this, ent, unlink_directories);
        _flock.acquire_write();
        return ret;
    }

    int ext2_node::truncate(off_t length) {
        _flock.acquire_write();
        auto ret = _vol->truncate(this, length);
        _flock.acquire_write();
        return ret;
    }

    void ext2_node::sync() {
        _vol->sync_node(this);
    }

    void ext2_node::close() {
        fs_node::close();

        if(_handle_count == 0) {
            _vol->clean_node(this);
        }
    }
}

fs::fs_volume* ext2_init(devices::partition_device* partition) {
    size_t size_to_read = fs::ext2::SUPERBLOCK_SIZE + sizeof(ext2_super_block_ext_t);
    int sector_no = fs::ext2::SUPERBLOCK_LOCATION / partition->parent()->block_size();
    uint8_t* buffer = partition->read_bytes(sector_no, size_to_read);
    if(!buffer) {
        log::warning("[ext2] Failed to read superblock");
        return nullptr;
    }

    ext2_super_block_t* superblock = sector_no == 0 
        ? (ext2_super_block_t *)(buffer + fs::ext2::SUPERBLOCK_SIZE) 
        : (ext2_super_block_t *)buffer;

    if(superblock->magic != fs::ext2::SUPER_MAGIC || superblock->rev_level > 1) {
        return nullptr;
    }

    const char* vol_name = fs::has_system_volume() ? partition->instance_name() : "system";
    return new fs::ext2_volume(partition, superblock, vol_name);
}