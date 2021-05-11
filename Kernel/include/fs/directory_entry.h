#pragma once

#include <stdint.h>
#include <fs/filesystem.h>
#include <kstring.h>

namespace fs {
    class directory_entry {
    public:
        mode_t flags {0};
        fs_node* node {nullptr};

        directory_entry() {}
        directory_entry(fs_node* node, const char* name);

        const char* name() const { return _name; }
        void set_name(const char* name) { strncpy(_name, name, NAME_MAX); }

        static mode_t file_to_dirent_flags(mode_t flags);

    private:
        char _name[NAME_MAX];
        uint32_t _inode {UINT32_MAX};
        directory_entry* _parent {nullptr};
    };
}