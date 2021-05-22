#pragma once

#include <device.h>
#include <storage/ahci.h>
#include <lock.h>

namespace ahci {
    class ahci_port : public devices::disk_device {
    public:
        ahci_port(int num, ahci_hba_port_t* port, ahci_hba_mem_t* mem, uint8_t slot_count);
        virtual ~ahci_port();

        int read_disk_block(uint64_t lba, uint32_t count, void* buffer) override;
        int write_disk_block(uint64_t lba, uint32_t count, void* buffer) override;

    private:
        achi_hba_cmd_header_t* prepare_cmd_header(int slot, uint64_t sector_count);
        int acquire_buffer();
        void release_buffer(int);

        ahci_hba_port_t* _registers;
        achi_hba_cmd_header_t* _command_base;
        hba_received_fis_t* _fis;
        uint8_t _slot_count;

        lock_t _buf_locks[8];
        page_entry _buffers[8];
        kstd::semaphore _buffer_semaphore {8};
        kstd::semaphore _port_lock {1};
    };
}