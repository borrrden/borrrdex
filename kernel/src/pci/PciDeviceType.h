#pragma once

#include <stdint.h>
#include <stddef.h>

typedef struct DeviceTypeEntry DeviceTypeEntry;

struct DeviceTypeEntry {
    const char* title;
    uint8_t id;
    DeviceTypeEntry* categories;
    size_t category_count;
};

static DeviceTypeEntry Unclassified[] = {
    {
        "Non-VGA-Compatible Device",
        0x0,
        NULL,
        0
    },
    {
        "VGA-Compatible Device",
        0x1,
        NULL,
        0
    }
};

static DeviceTypeEntry IDEController[] = {
    {
        "ISA Compatibility mode-only controller",
        0x00,
        NULL,
        0
    },
    {
        "PCI native mode-only controller",
        0x05,
        NULL,
        0
    },
    {
        "ISA Compatibility mode controller w/ both channels PCI",
        0x0A,
        NULL,
        0
    },
    {
        "PCI native mode controller w/ both channels ISA",
        0x0F,
        NULL,
        0
    },
    {
        "ISA Compatibility mode-only controller w/ mastering",
        0x80,
        NULL,
        0
    },
    {
        "PCI native mode-only controller w/ mastering",
        0x85,
        NULL,
        0
    },
    {
        "ISA Compatibility mode controller w/ both channels PCI & mastering",
        0x8A,
        NULL,
        0
    },
    {
        "PCI native mode controller w/ both channels ISA & mastering",
        0x8F,
        NULL,
        0
    }
};

static DeviceTypeEntry ATAController[] = {
    {
        "Single DMA",
        0x20,
        NULL,
        0
    },
    {
        "Chained DMA",
        0x30,
        NULL,
        0
    }
};

static DeviceTypeEntry SerialATA[] = {
    {
        "Vendor Specific Interface",
        0x00,
        NULL,
        0
    },
    {
        "AHCI 1.0",
        0x01,
        NULL,
        0
    },
    {
        "Serial Storage Bus",
        0x02,
        NULL,
        0
    }
};

static DeviceTypeEntry SerialAttachedSCSI[] = {
    {
        "SAS",
        0x00,
        NULL,
        0
    },
    {
        "Serial Storage Bus",
        0x01,
        NULL,
        0
    }
};

static DeviceTypeEntry NonVolatileMemoryController[] = {
    {
        "NVMHCI",
        0x01,
        NULL,
        0
    },
    {
        "NVM Express",
        0x02,
        NULL,
        0
    }
};

static DeviceTypeEntry MassStorageController[] = {
    {
        "SCSI Bus Controller",
        0x00,
        NULL,
        0
    },
    {
        "IDE Controller",
        0x01,
        IDEController,
        8
    },
    {
        "Floppy Disk Controller",
        0x02,
        NULL,
        0
    },
    {
        "IPI Bus Controller",
        0x03,
        NULL,
        0
    },
    {
        "RAID Controller",
        0x04,
        NULL,
        0
    },
    {
        "ATA Controller",
        0x05,
        ATAController,
        2
    },
    {
        "Serial ATA",
        0x06,
        SerialATA,
        3
    },
    {
        "Serial Attached SCSI",
        0x07,
        SerialAttachedSCSI,
        2
    },
    {
        "Non-Volatile Memory Controller",
        0x08,
        NonVolatileMemoryController,
        2
    }
};

static DeviceTypeEntry NetworkController[] = {
    {
        "Ethernet Controller",
        0x00,
        NULL,
        0
    },
    {
        "Token Ring Controller",
        0x01,
        NULL,
        0
    },
    {
        "FDDI Controller",
        0x02,
        NULL,
        0
    },
    {
        "ATM Controller",
        0x03,
        NULL,
        0
    },
    {
        "ISDN Controller",
        0x04,
        NULL,
        0
    },
    {
        "WorldFip Controller",
        0x05,
        NULL,
        0
    },
    {
        "PICMG 2.14 Multi Computing",
        0x06,
        NULL,
        0
    },
    {
        "Infiniband Controller",
        0x07,
        NULL,
        0
    },
    {
        "Fabric Controller",
        0x08,
        NULL,
        0
    }
};

static DeviceTypeEntry VGACompatibleController[] = {
    {
        "VGA Controller",
        0x00,
        NULL,
        0
    },
    {
        "8514-Compatible Controller",
        0x01,
        NULL,
        0
    }
};

static DeviceTypeEntry DisplayController[] = {
    {
        "VGA Compatible Controller",
        0x00,
        VGACompatibleController,
        2
    },
    {
        "XGA Controller",
        0x01,
        NULL,
        0
    },
    {
        "3D Controller (Not VGA-Compatible)",
        0x02,
        NULL,
        0
    }
};

static DeviceTypeEntry MultimediaController[] = {
    {
        "Multimedia Video Controller",
        0x00,
        NULL,
        0
    },
    {
        "Multimedia Audio Controller",
        0x01,
        NULL,
        0
    },
    {
        "Computer Telephony Device",
        0x02,
        NULL,
        0
    },
    {
        "Audio Device",
        0x03,
        NULL,
        0
    }
};

static DeviceTypeEntry MemoryController[] = {
    {
        "RAM Controller",
        0x00,
        NULL,
        0
    },
    {
        "Flash Controller",
        0x01,
        NULL,
        0
    }
};

static DeviceTypeEntry PciToPciBridge[] = {
    {
        "Normal Decode",
        0x00,
        NULL,
        0
    },
    {
        "Subtractive Decode",
        0x01,
        NULL,
        0
    }
};

static DeviceTypeEntry RACEWayBridge[] = {
    {
        "Transparent Mode",
        0x00,
        NULL,
        0
    },
    {
        "Endpoint Mode",
        0x01,
        NULL,
        0
    }
};

static DeviceTypeEntry PciToPciBridge2[] = {
    {
        "Semi-Transparent (primary bus toward CPU)",
        0x40,
        NULL,
        0
    },
    {
        "Semi-Transparent (secondary bus toward CPU)",
        0x80,
        NULL,
        0
    }
};

static DeviceTypeEntry BridgeDevice[] = {
    {
        "Host Bridge",
        0x00,
        NULL,
        0
    },
    {
        "ISA Bridge",
        0x01,
        NULL,
        0
    },
    {
        "EISA Bridge",
        0x02,
        NULL,
        0
    },
    {
        "MCA Bridge",
        0x03,
        NULL,
        0
    },
    {
        "PCI-to-PCI Bridge",
        0x04,
        PciToPciBridge,
        2
    },
    {
        "PCMCIA Bridge",
        0x05,
        NULL,
        0
    },
    {
        "NuBus Bridge",
        0x06,
        NULL,
        0
    },
    {
        "CardBus Bridge",
        0x07,
        NULL,
        0
    },
    {
        "RACEway Bridge",
        0x08,
        RACEWayBridge,
        2
    },
    {
        "PCI-to-PCI Bridge",
        0x09,
        PciToPciBridge2,
        2
    },
    {
        "InfiniBand-to-PCI Host Bridge",
        0x0A,
        NULL,
        0
    }
};

static DeviceTypeEntry SerialController[] = {
    {
        "8250-Compatible (Generic XT)",
        0x00,
        NULL,
        0
    },
    {
        "16450-Compatible",
        0x01,
        NULL,
        0
    },
    {
        "16550-Compatible",
        0x02,
        NULL,
        0
    },
    {
        "16650-Compatible",
        0x03,
        NULL,
        0
    },
    {
        "16750-Compatible",
        0x04,
        NULL,
        0
    },
    {
        "16850-Compatible",
        0x05,
        NULL,
        0
    },
    {
        "16950-Compatible",
        0x06,
        NULL,
        0
    },
};

static DeviceTypeEntry ParallelController[] = {
    {
        "Standard Parallel Port",
        0x00,
        NULL,
        0
    },
    {
        "Bi-Directional Parallel Port ",
        0x01,
        NULL,
        0
    },
    {
        "ECP 1.X Compliant Parallel Port",
        0x02,
        NULL,
        0
    },
    {
        "IEEE 1284 Controller",
        0x03,
        NULL,
        0
    },
    {
        "IEEE 1284 Target Device",
        0xFE,
        NULL,
        0
    }
};

static DeviceTypeEntry Modem[] = {
    {
        "Generic Modem",
        0x00,
        NULL,
        0
    },
    {
        "Hayes 16450-Compatible Interface",
        0x01,
        NULL,
        0
    },
    {
        "Hayes 16550-Compatible Interface",
        0x02,
        NULL,
        0
    },
    {
        "Hayes 16650-Compatible Interface",
        0x03,
        NULL,
        0
    },
    {
        "Hayes 16750-Compatible Interface",
        0x04,
        NULL,
        0
    },
};

static DeviceTypeEntry SimpleCommunicationController[] = {
    {
        "Serial Controller",
        0x00,
        SerialController,
        7
    },
    {
        "Parallel Controller",
        0x01,
        ParallelController,
        5
    },
    {
        "Multiport Serial Controller",
        0x02,
        NULL,
        0
    },
    {
        "Modem",
        0x03,
        Modem,
        5
    },
    {
        "IEEE 488.1/2 (GPIB) Controller",
        0x04,
        NULL,
        0
    },
    {
        "Smart Card",
        0x05,
        NULL,
        0
    }
};

static DeviceTypeEntry PIC[] = {
    {
        "Generic 8259-Compatible",
        0x00,
        NULL,
        0
    },
    {
        "ISA-Compatible",
        0x01,
        NULL,
        0
    },
    {
        "EISA-Compatible",
        0x02,
        NULL,
        0
    },
    {
        "I/O APIC Interrupt Controller",
        0x10,
        NULL,
        0
    },
    {
        "I/O(x) APIC Interrupt Controller",
        0x20,
        NULL,
        0
    }
};

static DeviceTypeEntry DMAController[] = {
    {
        "Generic 8237-Compatible",
        0x00,
        NULL,
        0
    },
    {
        "ISA-Compatible",
        0x01,
        NULL,
        0
    },
    {
        "EISA-Compatible",
        0x02,
        NULL,
        0
    },
};

static DeviceTypeEntry Timer[] = {
    {
        "Generic 8254-Compatible",
        0x00,
        NULL,
        0
    },
    {
        "ISA-Compatible",
        0x01,
        NULL,
        0
    },
    {
        "EISA-Compatible",
        0x02,
        NULL,
        0
    },
    {
        "HPET",
        0x03,
        NULL,
        0
    }
};

static DeviceTypeEntry RTCController[] = {
    {
        "Generic RTC",
        0x00,
        NULL,
        0
    },
    {
        "ISA-Compatible",
        0x01,
        NULL,
        0
    }
};

static DeviceTypeEntry BaseSystemPeripheral[] = {
    {
        "PIC",
        0x00,
        PIC,
        5
    },
    {
        "DMA Controller",
        0x01,
        DMAController,
        3
    },
    {
        "Timer",
        0x02,
        Timer,
        4
    },
    {
        "RTC Controller",
        0x03,
        RTCController,
        2
    },
    {
        "PCI Hot-Plug Controller",
        0x04,
        NULL,
        0
    },
    {
        "SD Host controller",
        0x05,
        NULL,
        0
    },
    {
        "IOMMU",
        0x06,
        NULL,
        0
    }
};

static DeviceTypeEntry GameportController[] = {
    {
        "Generic",
        0x00,
        NULL,
        0
    },
    {
        "Extended",
        0x10,
        NULL,
        0
    }
};

static DeviceTypeEntry InputDeviceController[] = {
    {
        "Keyboard Controller",
        0x00,
        NULL,
        0
    },
    {
        "Digitizer Pen",
        0x01,
        NULL,
        0
    },
    {
        "Mouse Controller",
        0x02,
        NULL,
        0
    },
    {
        "Scanner Controller",
        0x03,
        NULL,
        0
    },
    {
        "Gameport Controller",
        0x04,
        GameportController,
        2
    }
};

static DeviceTypeEntry DockingStation[] = {
    {
        "Generic",
        0x00,
        NULL,
        0
    }
};

static DeviceTypeEntry Processor[] = {
    {
        "386",
        0x00,
        NULL,
        0
    },
    {
        "486",
        0x01,
        NULL,
        0
    },
    {
        "Pentium",
        0x02,
        NULL,
        0
    },
    {
        "Pentium Pro ",
        0x02,
        NULL,
        0
    },
    {
        "Alpha",
        0x10,
        NULL,
        0
    },
    {
        "PowerPC",
        0x20,
        NULL,
        0
    },
    {
        "MIPS",
        0x30,
        NULL,
        0
    },
    {
        "Co-Processor",
        0x40,
        NULL,
        0
    }
};

static DeviceTypeEntry FirewareController[] = {
    {
        "Generic",
        0x00,
        NULL,
        0
    },
    {
        "OHCI",
        0x10,
        NULL,
        0
    }
};

static DeviceTypeEntry USBController[] = {
    {
        "UHCI Controller",
        0x00,
        NULL,
        0
    },
    {
        "OHCI Controller",
        0x10,
        NULL,
        0
    },
    {
        "EHCI (USB2) Controller",
        0x20,
        NULL,
        0
    },
    {
        "XHCI (USB3) Controller",
        0x30,
        NULL,
        0
    },
    {
        "Unspecified",
        0x80,
        NULL,
        0
    },
    {
        "USB Device",
        0xFE,
        NULL,
        0
    }
};

static DeviceTypeEntry IPMIInterface[] = {
    {
        "SMIC",
        0x00,
        NULL,
        0
    },
    {
        "Keyboard Controller Style",
        0x01,
        NULL,
        0
    },
    {
        "Block Transfer",
        0x02,
        NULL,
        0
    }
};

static DeviceTypeEntry SerialBusController[] = {
    {
        "FireWire (IEEE 1394) Controller",
        0x00,
        FirewareController,
        2
    },
    {
        "ACCESS Bus",
        0x01,
        NULL,
        0
    },
    {
        "SSA",
        0x02,
        NULL,
        0
    },
    {
        "USB Controller",
        0x03,
        USBController,
        6
    },
    {
        "Fibre Channel",
        0x04,
        NULL,
        0
    },
    {
        "SMBus",
        0x05,
        NULL,
        0
    },
    {
        "InfiniBand",
        0x06,
        NULL,
        0
    },
    {
        "IPMI Interface",
        0x07,
        IPMIInterface,
        3
    },
    {
        "SERCOS Interface (IEC 61491)",
        0x08,
        NULL,
        0
    },
    {
        "CANbus",
        0x09,
        NULL,
        0
    }
};

static DeviceTypeEntry WirelessController[] = {
    {
        "iRDA Compatible Controller",
        0x00,
        NULL,
        0
    },
    {
        "Consumer IR Controller",
        0x01,
        NULL,
        0
    },
    {
        "RF Controller",
        0x10,
        NULL,
        0
    },
    {
        "Bluetooth Controller",
        0x11,
        NULL,
        0
    },
    {
        "Broadband Controller",
        0x12,
        NULL,
        0
    },
    {
        "Ethernet Controller (802.1a)",
        0x20,
        NULL,
        0
    },
    {
        "Ethernet Controller (802.1b)",
        0x21,
        NULL,
        0
    }
};

static DeviceTypeEntry IntelligentController[] = {
    {
        "I20",
        0x00,
        NULL,
        0
    }
};

static DeviceTypeEntry SatelliteCommunicationController[] = {
    {
        "Satellite TV Controller",
        0x01,
        NULL,
        0
    },
    {
        "Satellite Audio Controller",
        0x02,
        NULL,
        0
    },
    {
        "Satellite Voice Controller",
        0x03,
        NULL,
        0
    },
    {
        "Satellite Data Controller",
        0x04,
        NULL,
        0
    }
};

static DeviceTypeEntry EncryptionController[] = {
    {
        "Network and Computing Encrpytion/Decryption",
        0x00,
        NULL,
        0
    },
    {
        "Entertainment Encryption/Decryption",
        0x10,
        NULL,
        0
    }
};

static DeviceTypeEntry SignalProcessingController[] = {
    {
        "DPIO Modules",
        0x00,
        NULL,
        0
    },
    {
        "Performance Counters",
        0x01,
        NULL,
        0
    },
    {
        "Communication Synchronizer",
        0x10,
        NULL,
        0
    },
    {
        "Signal Processing Management",
        0x20,
        NULL,
        0
    }
};

static constexpr int kNumClasses = 22;
static DeviceTypeEntry Root[kNumClasses] = {
    {
        "Unclassified",
        0x00,
        Unclassified,
        2
    },
    {
        "Mass Storage Controller",
        0x01,
        MassStorageController,
        8
    },
    {
        "Network Controller",
        0x02,
        NetworkController,
        9
    },
    {
        "Display Controller",
        0x03,
        DisplayController,
        3
    },
    {
        "Multimedia Controller",
        0x04,
        MultimediaController,
        4
    },
    {
        "Memory Controller",
        0x05,
        MemoryController,
        2
    },
    {
        "Bridge Device",
        0x06,
        BridgeDevice,
        11
    },
    {
        "Simple Communication Controller",
        0x07,
        SimpleCommunicationController,
        6
    },
    {
        "Base System Peripheral",
        0x08,
        BaseSystemPeripheral,
        7
    },
    {
        "Input Device Controller",
        0x09,
        InputDeviceController,
        5
    },
    {
        "Docking Station",
        0x0A,
        DockingStation,
        1
    },
    {
        "Processor",
        0x0B,
        Processor,
        8
    },
    {
        "Serial Bus Controller",
        0x0C,
        SerialBusController,
        10
    },
    {
        "Wireless Controller",
        0x0D,
        WirelessController,
        7
    },
    {
        "Intelligent Controller",
        0x0E,
        IntelligentController,
        1
    },
    {
        "Satellite Communication Controller",
        0x0F,
        SatelliteCommunicationController,
        5
    },
    {
        "Encryption Controller",
        0x10,
        EncryptionController,
        2
    },
    {
        "Signal Processing Controller",
        0x11,
        SignalProcessingController,
        0x21
    },
    {
        "Processing Accelerator",
        0x12,
        NULL,
        0
    },
    {
        "Non-Essential Instrumentation",
        0x13,
        NULL,
        0
    },
    {
        "Co-Processor",
        0x40,
        NULL,
        0
    },
    {
        "Unassigned",
        0xFF,
        NULL,
        0
    }
};