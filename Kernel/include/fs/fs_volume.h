#pragma once

#include <fs/filesystem.h>
#include <fs/fs_node.h>
#include <fs/directory_entry.h>

#include <kstring.h>

namespace fs {
    class fs_volume {
    public:
        virtual ~fs_volume() = default;

        virtual void set_volume_id(volume_id_t id);

        fs_node* mount_point() const { return _mount_point; }
        void set_mount_point(fs_node* mp) { _mount_point = mp; }

        directory_entry mount_point_entry() { return _mount_point_entry; }
        void set_mount_point_entry(const directory_entry &de) { _mount_point_entry = de; }
        void set_mount_point_entry(directory_entry&& de) { _mount_point_entry = std::move(de); }
    protected:
        volume_id_t _volume_id;
        fs_node* _mount_point;
        directory_entry _mount_point_entry;
    };

    class link_volume : public fs_volume {
    public:
        inline link_volume(fs_volume* link, const char* name) {
            _mount_point_entry.set_name(name);
            _mount_point_entry.node = link->mount_point();
            _mount_point_entry.flags = link->mount_point_entry().flags;
            _mount_point_entry.node->nlink++;
        }

        inline link_volume(fs_node* link, const char* name) {
            _mount_point_entry.set_name(name);
            _mount_point_entry.node = link;
            _mount_point_entry.flags = DT_DIR;
            _mount_point_entry.node->nlink++;
        }
    };
}