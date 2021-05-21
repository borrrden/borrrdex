#include <storage/ahci.h>
#include <kpci.h>
#include <logging.h>
#include <klist.hpp>
#include <storage/ahci_controller.h>
#include <paging.h>
#include <timer.h>

namespace ahci {
    constexpr uint16_t PCI_CLASS_STORAGE = 0x01;
    constexpr uint16_t PCI_SUBCLASS_SATA = 0x06;

    constexpr uint32_t SATA_SIG_ATA     = 0x00000101;
    constexpr uint32_t SATA_SIG_ATAPI   = 0xEB140101;
    constexpr uint32_t SATA_SIG_SEMB    = 0xC33C0101;
    constexpr uint32_t SATA_SIG_PM      = 0x96690101;

    list<ahci_controller> controllers;

    int initialize() {
        int i = 0;
        const pci_device_t* device_info = (pci_device_t *)pci::get_generic_device(PCI_CLASS_STORAGE, PCI_SUBCLASS_SATA, i);
        while(device_info) {
            controllers.add(ahci_controller(device_info));
            device_info = device_info = (pci_device_t *)pci::get_generic_device(PCI_CLASS_STORAGE, PCI_SUBCLASS_SATA, ++i);
        }

        if(controllers.size() == 0) {
            log::warning("[ahci] No controller found.");
            return 1;
        }

        return 0;
    }
}