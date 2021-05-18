#pragma once

#include <fs/fs_node.h>
#include <kassert.h>

namespace devices {
    enum device_type {
        unknown,
        devfs,
        unix_pseudo,
        unix_pseudo_terminal,
        kernel_log
    };

    class device : public fs::fs_node {
    public:
        device(device_type type, const char* name = nullptr, device* parent = nullptr);
        ~device();

        inline void set_device_id(int64_t id) {
            assert(_device_id == 0 && id > 0);
            _device_id = id;
        }

        inline bool is_root_device() const { return _is_root; }
        inline const char* instance_name() const { return _instance_name; }
        inline const char* device_name() const { return _device_name; }
        inline device_type type() const { return _type; }
        inline int64_t id() const { return _device_id; }
    protected:
        const char* _instance_name;
        const char* _device_name { "Unknown Device" };
        device_type _type { device_type::unknown };
        int64_t _device_id {0};
        bool _is_root;
    };
}

namespace device_manager {
    void initialize();
    void register_device(devices::device* d);
    void unregister_device(devices::device* d);

    fs::fs_node* get_devfs();
}