#include "pci.h"
#include "PciDeviceType.h"
#include "../graphics/BasicRenderer.h"
#include "../cstr.h"

#include <cstddef>

void pci_check_bus(void *, uint8_t);

inline bool pci_device_exists(PCI_HEADER* h) {
    return h->device_id != 0xffff;
}

DeviceTypeEntry* pci_device_table_lookup(DeviceTypeEntry* table, size_t count, uint8_t id) {
    for(size_t i = 0; i < count; i++) {
        DeviceTypeEntry entry = table[i];
        if(entry.id == id) {
            return &table[i];
        }
    }

    return NULL;
}

bool pci_get_type(PCI_HEADER* h, const char** cls, const char ** subclass, const char** progif) {
    *cls = NULL;
    *subclass = NULL;
    *progif = NULL;

    DeviceTypeEntry* d = pci_device_table_lookup(Root, kNumClasses, h->class_code);
    if(!d) {
        return false;
    }

    *cls = d->title;
    if(!d->categories) {
        return true;
    }

    d = pci_device_table_lookup(d->categories, d->category_count, h->subclass);
    if(!d) {
        if(h->subclass == 0x80) {
            *subclass = "Other";
            return true;
        }

        return false;
    }
    
    *subclass = d->title;
    if(!d->categories) {
        return true;
    }

    d = pci_device_table_lookup(d->categories, d->category_count, h->prog_interface);
    if(!d) {
        return false;
    }

    *progif = d->title;
    return true;
}

void pci_print_function(uint8_t* base, uint8_t* function, uint8_t functionNum) {
    PCI_HEADER* h = (PCI_HEADER *)function;
    GlobalRenderer->Print("                Function ");
    GlobalRenderer->Print(to_string((uint64_t)functionNum));
    GlobalRenderer->Print(" -> Vendor ID: ");
    GlobalRenderer->Print(to_hstring(h->vendor_id));
    GlobalRenderer->Print(" / Device ID: ");
    GlobalRenderer->Print(to_hstring(h->device_id));
    GlobalRenderer->Print(" / Status: ");
    GlobalRenderer->Print(to_string((uint64_t)h->status));
    GlobalRenderer->Print(" / Type: \\\\");
    const char* c, *s, *p;
    if(pci_get_type(h, &c, &s, &p)) {
        GlobalRenderer->Print(c);
        if(s) {
            GlobalRenderer->Print("\\");
            GlobalRenderer->Print(s);
        }

        if(p) {
            GlobalRenderer->Print("\\");
            GlobalRenderer->Print(p);
        }
    } else {
        GlobalRenderer->Print("[TYPE CORRUPT!]");
    }
    
    if(h->class_code == 0x06 && h->subclass == 0x04) {
        pci_check_bus(base, ((PCI_TO_PCI_BRIDGE*)h)->secondary_bus);
    } else {
        pci_print_all_bar(h);
    }
}

void pci_print_device(uint8_t* base, uint8_t* device, uint8_t deviceNum) {
    PCI_HEADER* h = (PCI_HEADER *)device;
    if(!pci_device_exists(h)) {
        return;
    }

    GlobalRenderer->Print("              Device ");
    GlobalRenderer->Print(to_string((uint64_t)deviceNum));
    GlobalRenderer->Next();
    pci_print_function(base, device, 0);
    GlobalRenderer->Next();
    if(h->header_type & 0x80) {
        device += 0x1000;
        for(uint8_t i = 1; i < 8; i++, device += 0x1000) {
            h = (PCI_HEADER *)device;
            if(pci_device_exists(h)) {
                pci_print_function(base, device, i);
                GlobalRenderer->Next();
            }
        }
    }
}

void pci_print_bus(void* base_address, uint8_t num) {
    uint8_t* base = (uint8_t *)base_address;
    uint8_t* bus = base + (num * 0x100000);
    uint8_t* device = bus;

    GlobalRenderer->Print("            Bus ");
    GlobalRenderer->Print(to_string((uint64_t)num));
    GlobalRenderer->Next();
    for(uint8_t i = 0; i < 32; i++, device += 0x8000) {
        pci_print_device(base, device, i);
    }
}

PCI_HEADER* pci_get_device(void* cfgArea, uint8_t bus, uint8_t device, uint8_t function) {
    uint8_t* p = (uint8_t *)cfgArea;
    p += bus * 0x100000 + device * 0x8000 + function * 0x1000;
    PCI_HEADER* pci_header = (PCI_HEADER *)p;
    if(pci_header->device_id == INVALID_DEVICE) {
        return NULL;
    }

    return pci_header;
}

void pci_print_bar_at(PCI_HEADER* h, uint32_t* addresses, size_t count) {
    for(size_t i = 0; i < count; i++) {
        uint64_t r1 = addresses[i];
        if(r1 == 0) {
            continue;
        }

        uint64_t width = 0;
        uint32_t realAddr = addresses[i];
        GlobalRenderer->Next();
        if(r1 & 1) {
            bool ioActive = h->command & 0x02;
            if(ioActive) {
                h->command &= ~0x02;
            }

            GlobalRenderer->Print("                  I/O BAR: 0x");
            GlobalRenderer->Print(to_hstring((uint64_t)&addresses[i]));

            addresses[i] = 0xffffffff;
            uint32_t nextValue = addresses[i];
            addresses[i] = (uint32_t)r1;
            width = ~(nextValue & 0xFFFFFFFC) + 1;

            if(ioActive) {
                h->command |= 0x02;
            }
        } else {
            bool memActive = h->command & 0x01;
            if(memActive) {
                h->command &= ~0x01;
            }

            GlobalRenderer->Print("                  RAM BAR: 0x");
            GlobalRenderer->Print(to_hstring((uint64_t)&addresses[i]));
            if(r1 & 0x4) {
                i++;
                uint64_t r2 = addresses[i];
                addresses[i-1] = addresses[i] = 0xffffffff;
                uint64_t nextValue = addresses[i-1] + (addresses[i] << 32);
                addresses[i-1] = r1;
                addresses[i] = r2;
                width = ~(nextValue & 0xFFFFFFFFFFFFFFF0) + 1;
            } else {
                addresses[i] = 0xffffffff;
                uint32_t nextValue = addresses[i];
                addresses[i] = (uint32_t)r1;
                width = ~(nextValue & 0xFFFFFFF0) + 1;
            }

            if(memActive) {
                h->command |= 0x01;
            }
        }

        GlobalRenderer->Print(" (");
        const char* suffix;
        GlobalRenderer->Print(to_string(Smallest(width, &suffix)));
        GlobalRenderer->Print(suffix);
        GlobalRenderer->Print(" used by device)");
    }
}

void pci_print_all_bar(PCI_HEADER* h)
{
    uint8_t type = h->header_type & 0x7f;
    if(type == 0) {
        PCI_DEVICE* d = (PCI_DEVICE *)h;
        pci_print_bar_at(h, d->bar, 6);
    } else if(type == 1) {
        PCI_TO_PCI_BRIDGE* b = (PCI_TO_PCI_BRIDGE *)h;
        pci_print_bar_at(h, b->bar, 2);
    }
}