#pragma once

#include <storage/ahci.h>
#include <stddef.h>
#include <kpci.h>
#include <klist.hpp>
#include <storage/ahci_port.h>

namespace ahci {
    class ahci_controller final {
        friend class ahci_port;
    public:
        ahci_controller(const pci_device_t* pci_location);
        ~ahci_controller();

        inline uint8_t command_slot_count() const { return (uint8_t)((_abar->capabilities >> ahci::CAP_NCS_OFFSET) & ahci::CAP_NCS_MASK) + 1; }

    private:
        bool register_achi_disk(ahci_hba_port_t* port, uint32_t index, uint8_t max_command_slots);
        bool identify_device(ahci_hba_port_t* port, uint32_t index, int slot_count);

        struct page_entry {
            uintptr_t phys;
            void* virt;
        };

        ahci_hba_mem_t* _abar;
        list<ahci_port *> _ports;
        page_entry _command_pages[32];
        char _version[6]; 
    };
}