#include <device.h>
#include <kstring.h>
#include <fs/fs_volume.h>
#include <fs/filesystem.h>
#include <abi-bits/errno.h>
#include <fs/ext2.h>
#include <frg/hash_map.hpp>
#include <frg/std_compat.hpp>
#include <storage/gpt.h>

template<>
class frg::hash<guid_t> {
public:
    unsigned int operator() (const guid_t &guid) const {
        unsigned int hash = guid.d1;
        hash += 31 * hash + guid.d2;
        hash += 31 * hash + guid.d3;
        for(size_t i = 0; i < 8; i++) {
            hash += 31 * hash + guid.d4[i];
        }

        return hash;
    }
};

namespace devices {
    using filesystem_init_t = fs::fs_volume*(*)(partition_device*);

    struct filesystem {
        const char* name;
        filesystem_init_t init;
    };

    using guid_filesystem_map_t = frg::hash_map<guid_t, filesystem, frg::hash<guid_t>, frg::stl_allocator>;

    static guid_filesystem_map_t mapped_filesystems(frg::hash<guid_t>(), {
        frg::make_tuple(gpt::GUID_LINUX_FS_DATA, filesystem{"ext2", ext2_init})
    });

    unsigned disk_device::next_device_num = 0;

    disk_device::disk_device()
        :device(device_type::storage)
    {
        flags = fs::FS_NODE_BLKDEVICE;

        _name = (char *)malloc(16); 
        strncpy(_name, "hd", 2);
        itoa(next_device_num++, _name + 2, 10);
        _instance_name = _name;
    }

    disk_device::~disk_device() {
        free(_name);
    }

    int disk_device::initialize_partitions() {
        static char letter = 'a';
        for(unsigned i = 0; i < _partitions.size(); i++) {
            auto* part = _partitions[i];
            auto found = mapped_filesystems.find(part->type());
            if(found != mapped_filesystems.end()) {
                fs::fs_volume* v = found->get<1>().init(_partitions[i]);
                if(v) {
                    if(v->error()) {
                        delete v;
                    } else {
                        fs::register_volume(v, false);
                    }
                }
            }
        }

        return 0;
    }

    ssize_t disk_device::read(size_t offset, size_t size, uint8_t* buf) {
        if(offset & _block_size) {
            return -EINVAL;
        }

        return read_disk_block(offset / _block_size, size, buf) != 0 ? -EIO : size;
    }

    ssize_t disk_device::write(size_t offset, size_t size, uint8_t* buf) {
        return -ENOSYS;
    }
}