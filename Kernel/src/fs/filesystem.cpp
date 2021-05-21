#include <fs/filesystem.h>
#include <fs/fs_node.h>
#include <fs/fs_volume.h>

#include <logging.h>
#include <kerrno.h>
#include <kstring.h>
#include <kassert.h>
#include <debug.h>

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

    bool has_system_volume() {
        for(int i = 0; i < volumes->size(); i++) {
            auto* vol = volumes->get(i);
            if(strncmp("system", vol->mount_point_entry().name(), 7) == 0) {
                return true;
            }
        }

        return false;
    }

    fs_node* get_root() {
        return &root;
    }

    void register_volume(fs_volume* vol, bool add_root) {
        if(add_root) {
            vol->mount_point()->parent = &root;
            vol->set_volume_id(next_vid++);
        }
        
        volumes->add(vol);
    }

    fs_node* follow_link(fs_node* link, fs_node* working_dir) {
        assert(link);

        char buffer[PATH_MAX + 1];
        auto bytes_read = link->read_link(buffer, PATH_MAX);
        if(bytes_read < 0) {
            log::warning("follow_link: read_link error %d", bytes_read);
            return nullptr;
        }

        buffer[bytes_read] = 0;
        fs_node* node = resolve_path(buffer, working_dir, false);
        if(!node) {
            log::warning("follow_link: failed to resolve symlink %s", buffer);
        }

        return node;
    }

    fs_node* resolve_path(const char* path, const char* working_dir, bool follow_symlinks) {
        char* temp_path = nullptr;
        if(working_dir && path[0] != '/') {
            // Relative directory
            size_t path_len = strnlen(path, PATH_MAX);
            size_t wd_len = strnlen(working_dir, PATH_MAX);
            temp_path = (char *)malloc(path_len + wd_len + 2);
            strncpy(temp_path, working_dir, wd_len);
            strncpy(temp_path + wd_len, "/", 1);
            strncpy(temp_path + wd_len + 1, path, path_len);
        }

        fs_node* ret_val = resolve_path(temp_path ?: path, fs::get_root(), follow_symlinks);
        if(temp_path) {
            free(temp_path);
        }

        return ret_val;
    }

    fs_node* resolve_path(const char* path, fs_node* working_dir, bool follow_symlinks) {
        assert(path);

        fs_node* root = fs::get_root();
        fs_node* current_node = working_dir;

        char* saved;
        char* file = strtok_r((char *)path, "/", &saved);
        while(file != nullptr) {
            fs_node* node = find_dir(current_node, file);
            if(!node) {
                IF_DEBUG(debug_level_filesystem >= debug::LEVEL_NORMAL, {
                    log::warning("%s not found", file);
                })
                
                return nullptr;
            }

            size_t symlink_count = 0;
            while(node->is_symlink()) {
                if(symlink_count++ > SYMLINK_MAX) {
                    log::warning("resolve_path: too many symlinks");
                    return nullptr;
                }

                node = follow_link(node, current_node);
                if(!node) {
                    log::warning("resolve_path: unresolved symlink");
                    return nullptr;
                }
            }

            if(node->is_dir()) {
                current_node = node;
                file = strtok_r(nullptr, "/", &saved);
                continue;
            }

            if((file = strtok_r(nullptr, "/", &saved))) {
                log::warning("%s is not a directory!", file);
                return nullptr;
            }

            while(follow_symlinks && node->is_symlink()) {
                if(symlink_count++ > SYMLINK_MAX) {
                    log::warning("resolve_path: too many symlinks");
                    return nullptr;
                }

                node = follow_link(node, current_node);
                if(!node) {
                    log::warning("resolve_path: unresolved symlink");
                    return nullptr;
                }
            }

            current_node = node;
            break;
        }

        return current_node;
    }

    fs_node* find_dir(fs_node* parent, const char* name) {
        assert(parent);
        return parent->find_dir(name);
    }

    ssize_t read(fs_node* node, size_t off, size_t size, void* buf) {
        assert(node);
        return node->read(off, size, reinterpret_cast<uint8_t *>(buf)); 
    }

    ssize_t read(fs_fd_t* handle, size_t size, uint8_t* buf) {
        assert(handle);
        ssize_t ret = read(handle->node, handle->pos, size, buf);
        if(ret > 0) {
            handle->pos += ret;
        }

        return ret;
    }

    ssize_t write(fs_node* node, size_t off, size_t size, void* buf) {
        assert(node);
        return node->write(off, size, reinterpret_cast<uint8_t *>(buf)); 
    }

    ssize_t write(fs_fd_t* handle, size_t size, uint8_t* buf) {
        assert(handle);
        ssize_t ret = write(handle->node, handle->pos, size, buf);
        if(ret > 0) {
            handle->pos += ret;
        }

        return ret;
    }

    fs_fd_t* open(fs_node* node, uint32_t flags) {
        return node->open(flags);
    }

    void close(fs_node* node) {
        node->close();
    }

    void close(fs_fd_t* fd) {
        if(!fd) {
            return;
        }

        assert(fd->node);
        fd->node->close();
        fd->node = nullptr;
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