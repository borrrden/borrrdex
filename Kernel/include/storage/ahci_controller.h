#pragma once

#include <storage/ahci.h>
#include <stddef.h>
#include <kpci.h>

typedef struct {
    uint8_t slot_count;
    uint32_t block_size;
    uint64_t block_count;
} ahci_device_info_t;

namespace ahci {
    class ahci_controller final {
    public:
        static int detected_controller_count();
        static ahci_controller* get_detected_controller(size_t index);

        ahci_controller(pci_device_t* pci_location);
        ~ahci_controller();
    };
}