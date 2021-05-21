#include <device.h>
#include <kstring.h>
#include <debug.h>
#include <logging.h>
#include <kmath.h>

namespace devices {
    partition_device::partition_device(uint64_t start_lba, uint64_t end_lba, disk_device* device, guid_t type)
        :devices::device(device_type::storage)
        ,_start_lba(start_lba)
        ,_end_lba(end_lba)
        ,_parent(device)
        ,_partition_type(type)
    {
        flags = fs::FS_NODE_BLKDEVICE;

        _name = (char *)malloc(18);
        size_t parent_name_len = strnlen(_parent->_instance_name, 18);
        strncpy(_name, _parent->_instance_name, 18);
        strncat(_name, "p", 18);
        itoa(_parent->_next_partition_num++, _name + parent_name_len + 1, 10);
        _instance_name = _name;
    }

    partition_device::~partition_device() {
        if(_name) {
            free(_name);
        }
    }

    uint8_t* partition_device::read_bytes(uint64_t lba, uint32_t count) {
        uint32_t block_count = kstd::intervals_needed(count, _parent->block_size());
        uint32_t full_byte_size = _parent->block_size() * block_count;
        uint8_t* buffer = (uint8_t *)malloc(full_byte_size);
        if(int e = read_block(lba, block_count, buffer) < 0) {
            log::error("[partition] Couldn't read bytes: %d", e);
            free(buffer);
            return nullptr;
        }

        memset(buffer + count, 0, full_byte_size - count);
        return buffer;
    }

    int partition_device::read_block(uint64_t lba, uint32_t count, void* buffer) {
        if(lba * _parent->_block_size + count > (_end_lba - _start_lba) * _parent->_block_size) {
            log::debug(debug_level_partitions, debug::LEVEL_NORMAL, 
                "[partition_device]: read_block: LBA %d out of parition range!", lba + count / _parent->_block_size);
            return 2;
        }

        return _parent->read_disk_block(lba + _start_lba, count, buffer);
    }

    int partition_device::write_block(uint64_t lba, uint32_t count, void* buffer) {
        if(lba * _parent->_block_size + count > (_end_lba - _start_lba) * _parent->_block_size) {
            log::debug(debug_level_partitions, debug::LEVEL_NORMAL, 
                "[partition_device]: read_block: LBA %d out of parition range!", lba + count / _parent->_block_size);
            return 2;
        }

        return _parent->write_disk_block(lba + _start_lba, count, buffer);
    }
}
