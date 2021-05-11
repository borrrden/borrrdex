#pragma once

#include <fs/filesystem.h>
#include <fs/fs_node.h>
#include <fs/directory_entry.h>

namespace fs {
    class fs_volume {
    public:
        virtual ~fs_volume() = default;

        virtual void set_volume_id(volume_id_t id);
        fs_node* mount_point() const { return _mount_point; }
        directory_entry mount_point_entry() { return _mount_point_entry; }
    
    protected:
        volume_id_t _volume_id;
        fs_node* _mount_point;
        directory_entry _mount_point_entry;
    };
}