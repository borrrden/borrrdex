#include <fs/filesystem.h>
#include <fs/fs_node.h>
#include <fs/fs_volume.h>

#include <logging.h>
#include <kerrno.h>
#include <kstring.h>
#include <kassert.h>

namespace fs {
    class root_node : public fs_node {
    public:
        root_node() {
            flags = FS_NODE_DIRECTORY;
        }

        int read_dir(directory_entry*, uint32_t) override;
        fs_node* find_dir(const char* name) override;

        int create(directory_entry* ent, uint32_t mode) override {
            log::warning("[RootFS] Attempted to create a file!");
            return -EROFS;
        }

        int create_dir(directory_entry* ent, uint32_t mode) override {
            log::warning("[RootFS] Attempted to create a directory!");
            return -EROFS;
        }

        bool can_write() const override { return false; }
    };

    list<fs_volume *>* volumes;
    volume_id_t next_vid = 1;
    root_node root;



    void initialize() {
        volumes = new list<fs_volume *>();
    }

    const list<fs_volume *>* get_volumes() {
        return volumes;
    }

    fs_node* get_root() {
        return &root;
    }

    void register_volume(fs_volume* vol) {
        vol->mount_point()->parent = &root;
        vol->set_volume_id(next_vid++);
        volumes->add(vol);
    }

    fs_node* find_dir(fs_node* parent, const char* name) {
        assert(parent);
        return parent->find_dir(name);
    }

    ssize_t read(fs_node* node, size_t off, size_t size, void* buf) {
        assert(node);
        return node->read(off, size, reinterpret_cast<uint8_t *>(buf)); 
    }

    int root_node::read_dir(directory_entry* ent, uint32_t index) {
        if(index < fs::volumes->size()) {
            *ent = volumes->get(index)->mount_point_entry();
            return 1;
        }

        return 0;
    }

    fs_node* root_node::find_dir(const char* name) {
        if(strcmp(".", name) == 0 || strcmp("..", name) == 0) {
            return this;
        }

        for(unsigned i = 0; i < volumes->size(); i++) {
            directory_entry ent = volumes->get(i)->mount_point_entry();
            if(strcmp(name, ent.name()) == 0) {
                return ent.node;
            }
        }

        return nullptr;
    }
}