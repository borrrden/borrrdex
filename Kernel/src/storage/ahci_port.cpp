#include <storage/ahci_port.h>
#include <storage/gpt.h>
#include <stddef.h>
#include <abi-bits/errno.h>
#include <kstring.h>
#include <timer.h>
#include <logging.h>
#include <paging.h>
#include <physical_allocator.h>
#include <liballoc/liballoc.h>

namespace ahci {
    extern int find_cmdslot(ahci_hba_port_t* port, int slot_count);

    achi_hba_cmd_header_t* ahci_port::prepare_cmd_header(int slot, uint64_t sector_count) {
        achi_hba_cmd_header_t* cmd_header = _command_base + slot;
        
        memset(cmd_header, 0, sizeof(achi_hba_cmd_header_t));
        cmd_header->command_fis_length = sizeof(fis_reg_h2d_t) / sizeof(uint32_t);
        cmd_header->prdt_entries = (uint16_t)((sector_count-1)>>4) + 1;
        cmd_header->command_table_base = _registers->cmd_list_base + sizeof(achi_hba_cmd_header_t) * 32;

        return cmd_header;
    }

    static void prepare_cmd_fis(hba_cmd_tbl_t* cmd_tbl, uint8_t command, uint64_t start, uint64_t count) {
        fis_reg_h2d_t* cmd_fis = (fis_reg_h2d_t *)(cmd_tbl->command_fis);
        cmd_fis->fis_type = reg_host_to_device;
        cmd_fis->cmd_register = command;
        cmd_fis->is_command = 1;
        cmd_fis->device = 0x40;
        cmd_fis->count = (uint16_t)(count & 0xffff);
        cmd_fis->lba0 = (uint8_t)start;
        cmd_fis->lba1 = (uint8_t)(start >> 8);
        cmd_fis->lba2 = (uint8_t)(start >> 16);
        cmd_fis->lba3 = (uint8_t)(start >> 24);
        cmd_fis->lba4 = (uint8_t)(start >> 32);
        cmd_fis->lba5 = (uint8_t)(start >> 40);
    }

    static void prepare_prdt_entries(hba_cmd_tbl_t* cmd_tbl, uint64_t buf, uint16_t entry_count, uint64_t sector_count) {
        int last_entry = entry_count - 1;
        for(int i = 0; i < last_entry; i++) {
            cmd_tbl->prdt_entries[i].db_addr = buf;
            cmd_tbl->prdt_entries[i].byte_count = 0x1fff;
            buf += 0x2000;
            sector_count -= 16;
        }

        cmd_tbl->prdt_entries[last_entry].db_addr = buf;
        cmd_tbl->prdt_entries[last_entry].byte_count = (sector_count<<9)-1;
    }

    static int issue_ahci_command(ahci_hba_port_t* port, uint8_t slot) {
        int spin = 0;
        while((port->task_file_data & (ahci::PXTFD_STS_BUSY_FLAG | ahci::PXTFD_STS_DRQ_FLAG)) && spin < 1000000) {
            spin++;
        }
        
        if(spin == 1000000) {
            return -1;
        }

        port->command_issue = (1 << slot);
        while(port->command_issue & (1 << slot)) {
            timer::wait(1);
        }

        if(spin == 1000000 || port->interrupt_status & ahci::PXIS_TFES_FLAG) {
            hba_received_fis_t* received = (hba_received_fis_t*)port->fis_base;
            return -1;
        }

        return 0;
    }

    ahci_port::ahci_port(int num, ahci_hba_port_t* port, ahci_hba_mem_t* mem, uint8_t slot_count)
        :_registers(port)
        ,_slot_count(slot_count)
        ,_command_base((achi_hba_cmd_header_t *)memory::get_io_mapping(port->cmd_list_base))
        ,_fis((hba_received_fis_t *)memory::get_io_mapping(port->fis_base))
    {
        _device_name = "SATA Hard Disk";

        for(int i = 0; i < 8; i++) {
            _buffers[i] = {
                .phys = memory::allocate_physical_block(),
                .virt = memory::kernel_allocate_4k_pages(1)
            };
            
            memory::kernel_map_virtual_memory_4k(_buffers[i].phys, (uint64_t)_buffers[i].virt, 1);
        }

        switch(gpt::parse(this)) {
            case 0:
                log::error("[ahci] disk has corrupted or non-existent GPT.  MBR disks are not supported.");
                break;
            case -1:
                log::error("[ahci] Error while parsing GPT");
                break;
            default:
                break;
        }

        log::info("[ahci] Found %d partitions!", _partitions.size());
        initialize_partitions();
    }

    ahci_port::~ahci_port() {
        for(int i = 0; i < 8; i++) {
            memory::kernel_free_4k_pages(_buffers[i].virt, 1);
            memory::free_physical_block(_buffers[i].phys);
        }
    }

    int ahci_port::acquire_buffer() {
        if(!_buffer_semaphore.wait()) {
            return -EINTR;
        }

        for(int i = 0; i < 8; i++) {
            if(acquire_test_lock(&_buf_locks[i])) {
                return i;
            }
        }

        _buffer_semaphore.signal();
        return -1;
    }

    void ahci_port::release_buffer(int index) {
        assert(index < 8);

        release_lock(&_buf_locks[index]);
        _buffer_semaphore.signal();
    }

    int ahci_port::read_disk_block(uint64_t lba, uint32_t count, void* buffer) {
        _registers->interrupt_status = -1;
        int slot = find_cmdslot(_registers, _slot_count);
        if(slot == -1) {
            return -EBUSY;
        }

        int buffer_idx = acquire_buffer();
        if(buffer_idx == -EINTR) {
            return EINTR;
        }

        if(buffer_idx < 0 || buffer_idx > 7) {
            return 4;
        }

        uintptr_t phys = _buffers[buffer_idx].phys;
        uint32_t max_sectors = memory::PAGE_SIZE_4K / _block_size;

        auto do_read = [&](uint32_t to_read) {
            achi_hba_cmd_header_t* cmd_header = prepare_cmd_header(slot, to_read);
            hba_cmd_tbl_t* cmd_tbl = (hba_cmd_tbl_t *)memory::get_io_mapping(cmd_header->command_table_base);
            memset(cmd_tbl, 0, sizeof(hba_cmd_tbl_t) + sizeof(hba_prdt_entry_t) * cmd_header->prdt_entries);

            prepare_prdt_entries(cmd_tbl, phys, cmd_header->prdt_entries, to_read);

            prepare_cmd_fis(cmd_tbl, ahci::IDE_COMMAND_DMA_READ_EX, lba, to_read);
            return issue_ahci_command(_registers, slot);
        };

        uint32_t remaining = count;
        uint8_t* b = (uint8_t *)buffer;
        while(remaining >= max_sectors) {
            // Don't read more than the buffer can take
            if(int e = do_read(max_sectors) != 0) {
                release_buffer(buffer_idx);
                return e;
            }

            memcpy(b, _buffers[buffer_idx].virt, max_sectors * _block_size);
            b += max_sectors * _block_size;
            lba += max_sectors;
            remaining -= max_sectors;
        }

        if(remaining) {
            if(int e = do_read(remaining) != 0) {
                release_buffer(buffer_idx);
                return e;
            }

            memcpy(b, _buffers[buffer_idx].virt, remaining * _block_size);
        }

        release_buffer(buffer_idx);
        return count;
    }

    int ahci_port::write_disk_block(uint64_t lba, uint32_t count, void* buffer) {
        _registers->interrupt_status = -1;
        int slot = find_cmdslot(_registers, _slot_count);
        if(slot == -1) {
            return -EBUSY;
        }

        achi_hba_cmd_header_t* cmd_header = prepare_cmd_header(slot, count);
        hba_cmd_tbl_t* cmd_tbl = (hba_cmd_tbl_t *)cmd_header->command_table_base;
        memset(cmd_tbl, 0, sizeof(hba_cmd_tbl_t) + sizeof(hba_prdt_entry_t) * cmd_header->prdt_entries);
        
        uint64_t phys = memory::virtual_to_physical_addr((uint64_t)buffer) + sizeof(boundary_tag);
        prepare_prdt_entries(cmd_tbl, phys, cmd_header->prdt_entries, count);

        prepare_cmd_fis(cmd_tbl, ahci::IDE_COMMAND_DMA_WRITE_EX, lba, count);
        if(issue_ahci_command(_registers, slot) == 0) {
            return count;
        }

        return -1;
    }
}