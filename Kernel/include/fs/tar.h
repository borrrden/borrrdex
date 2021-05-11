#pragma once

#include <stdint.h>
#include <fs/filesystem.h>
#include <fs/fs_volume.h>

// TAR v1.34
// https://www.gnu.org/software/tar/manual/html_node/Standard.html

typedef struct {
    char name[100];
    char mode[8];
    char uid[8];
    char gid[8];
    char size[12];
    char mod_time[12];
    char checksum[8];
    char type;
    char link_name[100];
    char magic[6]; 
    char version[2];
    char uname[32];
    char gname[32];
    char devmajor[8];
    char devminor[8];
    char prefix[155]; 
} __attribute__((packed)) ustar_header_t;

static_assert(sizeof(ustar_header_t) == 500, "Incorrect ustar header size");

typedef struct tar_header tar_header_t;

namespace fs::tar {
    constexpr uint16_t BYTES_PER_BLOCK = 512;

    class tar_volume;

    class tar_node : public fs_node {
    public:
        char name[256];
        tar_header_t* header;
        ino_t parent;
        int entry_count;
        ino_t* children;
        tar_volume* vol;

        ssize_t read(size_t off, size_t size, uint8_t* buf) override;
        ssize_t write(size_t off, size_t size, uint8_t* buf) override;
        void close() override;

        int read_dir(directory_entry* dirent, uint32_t index) override;
        fs_node* find_dir(const char* name) override;
    };

    class tar_volume : public fs_volume {
    public:
        tar_volume(uintptr_t base, size_t size, const char* name);

        ssize_t read(tar_node* node, size_t offset, size_t size, uint8_t* buffer);
        ssize_t write(tar_node* node, size_t offset, size_t size, uint8_t* buffer);
        void open(tar_node* node, uint32_t flags);
        void close(tar_node* node);
        int read_dir(tar_node* node, directory_entry*, uint32_t);
        fs_node* find_dir(tar_node* node, const char* name);

    private:
        int read_directory(int index, ino_t parent);
        void make_node(tar_header_t* header, tar_node* n, ino_t inode, ino_t parent, tar_header_t* dir_header = nullptr);

        tar_header_t* _blocks {nullptr};
        uint64_t _block_count {0};
        uint64_t _node_count {1};
        ino_t _next_node {1};
        tar_node* _nodes {nullptr};
    };
}

struct __attribute__ ((packed)) tar_header {
    union {
        ustar_header_t ustar;
        uint8_t block[fs::tar::BYTES_PER_BLOCK];
    };
};