#include "pci/pci.h"
#include "uhci.h"
#include "ohci.h"
#include "memory/heap.h"

constexpr uint8_t INTERFACE_UHCI = 0x0;
constexpr uint8_t INTERFACE_OHCI = 0x10;
constexpr uint8_t INTERFACE_EHCI = 0x20;
constexpr uint8_t INTERFACE_XHCI = 0x30;

static int usb_init(pci_header_t* mem) {
    pci_device_t* dev = (pci_device_t *)mem;
    switch(dev->common.prog_interface) {
        case INTERFACE_XHCI:
        case INTERFACE_EHCI:
            return -1;
        case INTERFACE_OHCI:
            return ohci_init(dev);
        case INTERFACE_UHCI:
            return uhci_init(dev);
        default:
            return -1;
    }
}

void usb_buffer_free(usb_buffer_t* buf) {
    kfree(buf->buf);
    buf->buf = nullptr;
}

usb_descriptor_base_t* usb_find_descriptor_type(usb_config_desc_t* config, uint8_t type) {
    int remaining = config->total_length - config->desc_length;
    usb_descriptor_base_t* current = (usb_descriptor_base_t *)((uint8_t *)config + config->desc_length);
    while(remaining) {
        if(current->type == type) {
            return current;
        }

        remaining -= current->length;
        current = (usb_descriptor_base_t *)((uint8_t *)current + current->length);
    }

    return nullptr;
}

PCI_MODULE_INIT(USB_PCI_MODULE, usb_init, 0xC, 0x3);