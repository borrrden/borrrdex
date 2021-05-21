#include <storage/ahci_controller.h>
#include <storage/ahci.h>
#include <paging.h>
#include <timer.h>
#include <kstring.h>
#include <physical_allocator.h>

namespace ahci {
    constexpr uint32_t AHCI_INTERNAL_VER_0_95   = 0x000905;
    constexpr uint32_t AHCI_INTERNAL_VER_1_0    = 0x010000;
    constexpr uint32_t AHCI_INTERNAL_VER_1_1    = 0x010100;
    constexpr uint32_t AHCI_INTERNAL_VER_1_2    = 0x010200;
    constexpr uint32_t AHCI_INTERNAL_VER_1_3    = 0x010300;
    constexpr uint32_t AHCI_INTERNAL_VER_1_3_1  = 0x010301;

    static void take_controller_ownership(ahci_hba_mem_t* controller) {
        if(controller->version < AHCI_INTERNAL_VER_1_2) {
            return; 
        }

        if(!(controller->capabilities2 & ahci::CAP2_BOH_FLAG)) {
            return;
        }

        controller->bios_handoff_ctrl |= ahci::BOHC_OOS_FLAG;
        timer::wait(25);

        if(!(controller->bios_handoff_ctrl & (ahci::BOHC_BB_FLAG | ahci::BOHC_BOS_FLAG))) {
            // BIOS no longer owns the controller, and is not busy
            return;
        }

        //Wait for the BIOS to stop being busy
        timer::wait(2000);

        // If it's not done by now, it is faulty 
        assert(!(controller->bios_handoff_ctrl & (ahci::BOHC_BB_FLAG | ahci::BOHC_BOS_FLAG)));
        assert(controller->bios_handoff_ctrl & ahci::BOHC_OOS_FLAG);

        controller->bios_handoff_ctrl &= ~ahci::BOHC_OOC_FLAG;
    }

    int find_cmdslot(ahci_hba_port_t* port, int slot_count) {
        for(int i = 0; i < slot_count; i++) {
            if((port->command_issue & (1 << i)) == 0) {
                return i;
            }
        }

        return -1;
    }

    bool ahci_controller::identify_device(ahci_hba_port_t* port, uint32_t index, int slot_count) {
        port->interrupt_status = -1;
        int spin = 0;
        int slot = find_cmdslot(port, slot_count);
        if(slot == -1) {
            return false;
        }

        uint8_t sector[512];
        memset(sector, 0, 512);

        achi_hba_cmd_header_t* cmd_header = (achi_hba_cmd_header_t *)memory::get_io_mapping(port->cmd_list_base) + slot;
        
        memset(cmd_header, 0, sizeof(achi_hba_cmd_header_t));
        cmd_header->command_fis_length = sizeof(fis_reg_h2d_t) / sizeof(uint32_t);
        cmd_header->prdt_entries = 1;
        cmd_header->command_table_base = port->cmd_list_base + sizeof(achi_hba_cmd_header_t) * 32;

        hba_cmd_tbl_t* cmd_tbl = (hba_cmd_tbl_t *)memory::get_io_mapping(cmd_header->command_table_base);
        memset(&cmd_tbl->command_fis, 0, 64);
        fis_reg_h2d_t* cmdfis = (fis_reg_h2d_t *)(cmd_tbl->command_fis);
        cmdfis->fis_type = reg_host_to_device;
        cmdfis->cmd_register = IDE_COMMAND_IDENTIFY;
        cmdfis->is_command = 1;

        memset(&cmd_tbl->prdt_entries[0], 0, sizeof(hba_prdt_entry_t));
        cmd_tbl->prdt_entries[0].byte_count = 0x1ff;
        cmd_tbl->prdt_entries[0].db_addr = (uint64_t)&sector[0];

        port->command_issue = (1 << slot);
        while(port->command_issue & (1 << slot)) {
            timer::wait(10);
        }

        if(port->interrupt_status & ahci::PXIS_TFES_FLAG) {
            return false;
        }

        return true;
    }

    ahci_controller::ahci_controller(const pci_device_t* device) {
        _abar = (ahci_hba_mem_t *)memory::get_io_mapping((uintptr_t)device->bar[5]);

        take_controller_ownership(_abar);

        uint32_t pi = _abar->port_implemented;
        int idx = 0;
        for(int i = 0; i < 32; i++, pi >>= 1) {
            if(!(pi & 1)) {
                continue;
            }

            uint8_t slot_count = command_slot_count();
            if(register_achi_disk(&_abar->ports[i], idx, slot_count)) {
                idx++;
                _ports.add(new ahci_port(idx, &_abar->ports[i], _abar, slot_count));
            }
        }

        memset(_version, 0, 6);
        memset(_command_pages, 0, sizeof(page_entry) * 32);
    }
    
    ahci_controller::~ahci_controller() {
        for(int i = 0; i < 32; i++) {
            if(_command_pages[i].phys) {
                memory::free_physical_block((uint64_t)_command_pages[i].phys);
            }
        }
    }

    bool ahci_controller::register_achi_disk(ahci_hba_port_t* port, uint32_t index, uint8_t max_cmd_slots) {
        uint32_t ssts = port->sata_status;
        ipm_status ipm = (ipm_status)((port->sata_status >> ahci::SCR0_IPM_OFFSET) & ahci::SCR0_IPM_MASK);
        device_detection_status det = (device_detection_status)(port->sata_status & ahci::SCR0_DET_MASK);
        if(det != phy_comm || ipm != active) {
            return false;
        }

        _command_pages[index].phys = memory::allocate_physical_block();
        _command_pages[index].virt = memory::kernel_allocate_4k_pages(1);
        memory::kernel_map_virtual_memory_4k(_command_pages[index].phys, (uint64_t)_command_pages->virt, 1);
        memset(_command_pages[index].virt, 0, memory::PAGE_SIZE_4K);

        // Idle the drive before modifying it
        // First, halt the command processing
        port->command &= ~ahci::PXCMD_ST_FLAG;
        while(port->command & ahci::PXCMD_CR_FLAG) {
            timer::wait(10);
        }

        // Then halt the FIS Receiving
        port->command &= ~ahci::PXCMD_FRE_FLAG;
        while(port->command & ahci::PXCMD_FR_FLAG) {
            timer::wait(10);
        }

        // Hooray, we can now write the memory address for the buffer
        // for the drive to use.  Be careful, this must be a *physical*
        // address since the controller bypasses the CPU and does not use
        // paging.
        port->cmd_list_base = (uint64_t)_command_pages[index].phys;

        // Get to work!
        port->command |= (ahci::PXCMD_FRE_FLAG | ahci::PXCMD_ST_FLAG);

        // Now whatever we write to the memory we just created, can be directly
        // processed by the drive via the controller.

        // Not going to use interrupts quite yet, that's for another day...
        port->interrupt_status = 0xffffffff; // Some are read only, controller doesn't seem to mind us trying though
        port->interrupt_enable = 0;

        // Currently hackishly ignoring ATAPI devices because I haven't written support for them yet
        if(!identify_device(port, index, max_cmd_slots)) {
            memory::free_physical_block((uint64_t)_command_pages[index].phys);
            memory::kernel_free_4k_pages(_command_pages[index].virt, 1);
            _command_pages[index] = {0, nullptr};
            return false;
        }

        return true;
    }
}