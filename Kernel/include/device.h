#pragma once

#include <fs/fs_node.h>
#include <kassert.h>
#include <klist.hpp>
#include <kguid.h>

namespace devices {
    enum device_type {
        unknown,
        devfs,
        unix_pseudo,
        unix_pseudo_terminal,
        kernel_log,
        storage
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

    class partition_device;

    class disk_device : public device {
        friend class partition_device;
    public:
        disk_device();
        virtual ~disk_device();

        int initialize_partitions();
        inline void add_partition(partition_device* partition) { _partitions.add(partition); }

        virtual int read_disk_block(uint64_t lba, uint32_t count, void* buffer) = 0;
        virtual int write_disk_block(uint64_t lba, uint32_t count, void* buffer) = 0;

        ssize_t read(size_t, size_t, uint8_t*) override;
        ssize_t write(size_t, size_t, uint8_t*) override;

        inline unsigned block_size() const { return _block_size; }
    protected:
        static unsigned next_device_num;

        unsigned _next_partition_num {0};
        list<partition_device *> _partitions;
        unsigned _block_size {512};
        char* _name;
    };

    class partition_device : public device {
    public:
        partition_device(uint64_t start_lba, uint64_t end_lba, disk_device* device, guid_t type = nullguid);
        virtual ~partition_device();

        uint8_t* read_bytes(uint64_t lba, uint32_t count);

        virtual int read_block(uint64_t lba, uint32_t count, void* buffer);
        virtual int write_block(uint64_t lba, uint32_t count, void* buffer);

        const disk_device* parent() const { return _parent; }
        inline const guid_t& type() const { return _partition_type; }
    private:
        uint64_t _start_lba;
        uint64_t _end_lba;
        guid_t _partition_type {nullguid};
        disk_device* _parent;
        char* _name;
    };
}

namespace device_manager {
    void initialize();
    void register_device(devices::device* d);
    void unregister_device(devices::device* d);

    fs::fs_node* get_devfs();
}