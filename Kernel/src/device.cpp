#include <device.h>
#include <fs/fs_volume.h>

#include <frg/list.hpp>
#include <frg/std_compat.hpp>

namespace devices {
    device::device(device_type type, const char* name, device* parent)
        :_type(type)
    {
        _is_root = !parent;
        this->parent = parent;
        if(type == device_type::devfs) {
            return;
        }

        _instance_name = name ?: "unknown";
        device_manager::register_device(this);
    }

    device::~device() {
        device_manager::unregister_device(this);
    }

    class null : public device {
    public:
        null(const char* name)
            :device(device_type::unix_pseudo, name)
        {
            flags = fs::FS_NODE_CHARDEVICE;
            _device_name = "UNIX null device";
        }

        ssize_t read(size_t offset, size_t size, uint8_t* buffer) override {
            memset(buffer, -1, size);

            return size;
        }

        ssize_t write(size_t offset, size_t size, uint8_t* buffer) override {
            return size;
        }
    };
}

namespace device_manager {
    struct device_node {
        
        devices::device* entry;
        frg::default_list_hook<device_node> hook;
    };

    using device_list_t = frg::intrusive_list<device_node, frg::locate_member<device_node, frg::default_list_hook<device_node>, &device_node::hook>>;

    int64_t next_devid = 1;
    device_list_t root_devices;
    size_t root_device_count = 0;
    device_list_t all_devices;
    size_t device_count = 0;

    class dev_fs : public devices::device {
    public:
        dev_fs(const char* name)
            : devices::device(devices::device_type::devfs)
        {
            flags = fs::FS_NODE_DIRECTORY;
            _vol.set_mount_point(this);
            _vol.set_mount_point_entry(fs::directory_entry(this, name));

            fs::register_volume(&_vol);
        }

        int read_dir(fs::directory_entry* dir_ent, uint32_t index) override final {
            if(index >= root_device_count + 2) {
                return 0;
            }

            if(index == 0) {
                dir_ent->set_name(".");
                dir_ent->flags = fs::FS_NODE_DIRECTORY;
                return 1;
            }

            if(index == 1) {
                dir_ent->set_name("..");
                dir_ent->flags = fs::FS_NODE_DIRECTORY;
                return 1;
            }

            auto item = root_devices.front();
            int i = 2;
            while(i < index) {
                item = item->hook.next;
            }

            dir_ent->set_name(item->entry->instance_name());
            return 1;
        }

        fs::fs_node* find_dir(const char* name) override final {
            if(strncmp(name, ".", 1) == 0) {
                return this;
            }

            if(strncmp(name, "..", 1) == 0) {
                return fs::get_root();
            }

            for(auto dev : all_devices) {
                if(strncmp(dev->entry->instance_name(), name, fs::NAME_MAX) == 0) {
                    return dev->entry;
                }
            }

            return nullptr;
        }
    private:
        fs::fs_volume _vol;
    };

    dev_fs* dfs;
    devices::null null_device("null");

    void initialize() {
        dfs = new dev_fs("dev");
    }

    void register_device(devices::device* d) {
        d->set_device_id(next_devid++);
        all_devices.push_back(new device_node{d});
        device_count++;
        if(d->is_root_device()) {
            root_devices.push_back(new device_node{d});
            root_device_count++;
        }
    }

    void unregister_device(devices::device* d) {
        device_node tmp{d};
        if(d->is_root_device()) {
            auto i = root_devices.iterator_to(&tmp);
            root_devices.erase(i);
            delete *i;
            root_device_count--;
        }

        auto i = all_devices.iterator_to(&tmp);
        all_devices.erase(i);
        delete *i;
    }

    fs::fs_node* get_devfs() {
        return dfs;
    }
}