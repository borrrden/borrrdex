#include <fs/tar.h>
#include <kstring.h>
#include <kerrno.h>

namespace fs::tar {
    constexpr char TAR_TYPE_FILE                    = '0';
    constexpr char TAR_TYPE_LINK_HARD               = '1';
    constexpr char TAR_TYPE_LINK_SYMBOLIC           = '2';
    constexpr char TAR_TYPE_CHARACTER_SPECIAL       = '3';
    constexpr char TAR_TYPE_BLOCK_SPECIAL           = '4';
    constexpr char TAR_TYPE_DIRECTORY               = '5';
    constexpr char TAR_TYPE_FIFO                    = '6';
    constexpr char TAR_TYPE_FILE_CONTIGUOUS         = '7';
    constexpr char TAR_TYPE_GLOBAL_EXTENDED_HEADER  = 'g';
    constexpr char TAR_TYPE_EXTENDED_HEADER         = 'g';
}

inline static long oct_to_dec(const char* str, int size) {
    long n = 0;
    while(size-- && *str) {
        n <<= 3;
        n |= (*str - '0') & 0x7;
        str++;
    }

    return n;
}

inline static long get_block_count(const char* size) {
    long sz = oct_to_dec(size, 12);
    return (sz + fs::tar::BYTES_PER_BLOCK - 1) / fs::tar::BYTES_PER_BLOCK;
}

inline static uint32_t tar_flags_to_fs(char type) {
    switch(type) {
        case fs::tar::TAR_TYPE_DIRECTORY:
            return fs::FS_NODE_DIRECTORY;
        case fs::tar::TAR_TYPE_LINK_SYMBOLIC:
            return fs::FS_NODE_SYMLINK;
        default:
            return fs::FS_NODE_FILE;
    }
}

namespace fs::tar {
    ssize_t tar_node::read(size_t off, size_t size, uint8_t* buf) {
        if(!vol) return -1;
        return vol->read(this, off, size, buf);
    }

    ssize_t tar_node::write(size_t off, size_t size, uint8_t* buf) {
        if(!vol) return -1;
        return vol->write(this, off, size, buf);
    }

    void tar_node::close() {
        if(!vol) return;
        vol->close(this);
    }

    int tar_node::read_dir(directory_entry* dirent, uint32_t index) {
        if(!vol) return -1;
        return vol->read_dir(this, dirent, index);
    }

    fs_node* tar_node::find_dir(const char* name) {
        if(!vol) return nullptr;
        return vol->find_dir(this, name);
    }

    void tar_volume::make_node(tar_header_t* header, tar_node* n, ino_t inode, ino_t parent, tar_header_t* dir_header) {
        n->parent = parent;
        n->header = header;
        n->inode = inode;
        n->uid = oct_to_dec(header->ustar.uid, 8);
        n->flags = tar_flags_to_fs(header->ustar.type);
        n->vol = this;
        n->volume_id = _volume_id;

        const char* name = header->ustar.name;
        char* saved;
        char* name_part = strtok_r(header->ustar.name, "/", &saved);
        while(name_part) {
            name = name_part;
            name_part = strtok_r(nullptr, "/", &saved);
        }

        strncpy(n->name, name, 256);
        n->size = oct_to_dec(header->ustar.size, 12);
    }

    int tar_volume::read_directory(int block_index, ino_t parent) {
        ino_t dir_inode = _next_node++;
        tar_header_t* dir_header = &_blocks[block_index];
        tar_node* dir_node = &_nodes[dir_inode];
        make_node(dir_header, dir_node, dir_inode, parent);

        dir_node->entry_count = 0;
        unsigned i = block_index + get_block_count(dir_header->ustar.size);
        while(i < _block_count) {
            if(strcmp(_blocks[i].ustar.name, dir_header->ustar.name) || !strnlen(_blocks[i].ustar.name, 1)) {
                break; // End of directory
            }

            if(_blocks[i].ustar.name[strnlen(dir_header->ustar.name, 100)] != '/') {
                break; // Found the leaf
            }

            dir_node->entry_count++;
            i += get_block_count(_blocks[i].ustar.size) + 1;
        }

        dir_node->children = (ino_t *)malloc(sizeof(ino_t) * dir_node->entry_count);
        i = block_index + get_block_count(dir_header->ustar.size) + 1;
        for(int e = 0; i < _block_count && e < dir_node->entry_count; e++) {
            ino_t inode = _next_node++;
            tar_node* n = &_nodes[inode];
            make_node(&_blocks[i], n, inode, dir_inode, dir_header);
            dir_node->children[e] = inode;
            i += get_block_count(_blocks[i].ustar.size) + 1;
        }

        return i;
    }

    tar_volume::tar_volume(uintptr_t base, size_t size, const char* name) {
        _blocks = (tar_header_t *)base;
        _block_count = size / BYTES_PER_BLOCK;

        int entry_count = 0;
        for(uint64_t i = 0; i < _block_count; i++, _node_count++) {
            tar_header_t header = _blocks[i];
            char* slash_pos = strchr(header.ustar.name, '/');
            if(!slash_pos || (header.ustar.type == TAR_TYPE_DIRECTORY && slash_pos == header.ustar.name + strnlen(header.ustar.name, 100) - 1)) {
                entry_count++;
            }

            i += get_block_count(header.ustar.size);
        }

        _nodes = new tar_node[_node_count];

        tar_node* volume_node = &_nodes[0];
        volume_node->header = nullptr;
        volume_node->flags = FS_NODE_DIRECTORY | FS_NODE_MOUNTPOINT;
        volume_node->inode = 0;
        volume_node->size = size;
        volume_node->vol = this;
        volume_node->parent = 0;

        _mount_point = volume_node;
        _mount_point_entry.set_name(name);
        _mount_point_entry.flags = DT_DIR;
        _mount_point_entry.node = volume_node;

        volume_node->children = (ino_t *)malloc(sizeof(ino_t) * entry_count);
        volume_node->entry_count = entry_count;
        int e = 0;
        for(unsigned i = 0; i < _block_count; i++, e++) {
            tar_header_t header = _blocks[i];
            if(!strnlen(header.ustar.name, 100)) {
                break;
            }

            if(header.ustar.type == TAR_TYPE_DIRECTORY) {
                volume_node->children[e] = _next_node;
                i = read_directory(i, 0);
            } else if(header.ustar.type == TAR_TYPE_FILE) {
                ino_t inode = _next_node++;
                tar_node* node = &_nodes[inode];
                make_node(&_blocks[i], node, inode, 0);
                volume_node->children[e] = inode;
            }

            i += get_block_count(header.ustar.size);
        }

        volume_node->entry_count = e;
    }

    ssize_t tar_volume::read(tar_node* node, size_t off, size_t size, uint8_t* buf) {
        tar_node* tar_node = &_nodes[node->inode];
        if((node->is_dir())) {
            return -EISDIR;
        }

        if(off > node->size || size == 0) {
            return 0;
        }

        memcpy(buf, (void *)(((uintptr_t)tar_node->header) + BYTES_PER_BLOCK + off), size);
        return size;
    }

    ssize_t tar_volume::write(tar_node* node, size_t off, size_t size, uint8_t* buf) {
        if((node->flags & FS_NODE_TYPE) == FS_NODE_DIRECTORY){
            return -EISDIR;
        }

        return -EROFS;
    }

    void tar_volume::open(__attribute__((unused)) tar_node* node, __attribute__((unused)) uint32_t flags) {

    }

    void tar_volume::close(__attribute__((unused)) tar_node* node) {

    }

    int tar_volume::read_dir(tar_node* node, directory_entry* dirent, uint32_t index) {
        tar_node* tnode = &_nodes[node->inode];
        if(!node->is_dir()) return -ENOTDIR;

        if(index >= (unsigned)(tnode->entry_count + 2)) return 0;

        if(index == 0) {
            dirent->set_name(".");
            dirent->flags = DT_DIR;
            return 1;
        } else if(index == 1) {
            dirent->set_name("..");
            dirent->flags = DT_DIR;
            return 1;
        }

        tar_node* dir = &_nodes[tnode->children[index - 2]];
        dirent->set_name(dir->name);
        dirent->flags = dir->flags;
        dirent->node = dir;

        switch(dir->flags & FS_NODE_TYPE) {
            case FS_NODE_FILE:
                dirent->flags = DT_REG;
                break;
            case FS_NODE_DIRECTORY:
                dirent->flags = DT_DIR;
                break;
            case FS_NODE_CHARDEVICE:
                dirent->flags = DT_CHR;
                break;
            case FS_NODE_BLKDEVICE:
                dirent->flags = DT_BLK;
                break;
            case FS_NODE_SOCKET:
                dirent->flags = DT_SOCK;
                break;
            case FS_NODE_SYMLINK:
                dirent->flags = DT_LNK;
                break;
        }

        return 1;
    }

    fs_node* tar_volume::find_dir(tar_node* node, const char* name) {
        tar_node* tnode = &_nodes[node->inode];
        if(!tnode->is_dir()) return nullptr;

        if(strncmp(".", name, 1) == 0) {
            return node;
        }

        if(strncmp("..", name, 1) == 0) {
            return node->inode == 0 ? get_root() : &_nodes[tnode->parent];
        }

        for(int i = 0; i < tnode->entry_count; i++) {
            if(strncmp(_nodes[tnode->children[i]].name, name, 256) == 0) {
                return &_nodes[tnode->children[i]];
            }
        }

        return nullptr;
    }
}