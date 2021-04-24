#include "drivers/device.h"
#include "drivers/disk.h"
#include "_disk.h"
#include "io/io.h"
#include "pci/pci.h"
#include "interrupt/interrupt.h"

extern "C" void ide_irq_handler0();
extern "C" void ide_irq_handler1();

static ide_channel_t s_ide_channels[IDE_CHANNELS_PER_CTRL];
static ide_device_t s_ide_devices[IDE_CHANNELS_PER_CTRL * IDE_DEVICES_PER_CHANNEL];
static device_t s_ide_dev[IDE_CHANNELS_PER_CTRL * IDE_DEVICES_PER_CHANNEL];
static gbd_t s_ide_gbd[IDE_CHANNELS_PER_CTRL * IDE_DEVICES_PER_CHANNEL];

void ide_write(uint8_t channel, uint8_t reg, uint8_t data) {
    if(reg > 0x07 && reg < 0x0C) {
        ide_write(channel, IDE_REGISTER_CTRL, 0x80 | s_ide_channels[channel].irq);
    }

    if(reg < 0x08) {
        port_write_8((uint16_t)(s_ide_channels[channel].base + reg), data);
    } else if(reg < 0x0C) {
        port_write_8((uint16_t)(s_ide_channels[channel].base + reg - 0x06), data);
    } else if(reg < 0x0E) {
        port_write_8((uint16_t)(s_ide_channels[channel].ctrl + reg - 0x0A), data);
    } else if(reg < 0x16) {
        port_write_8((uint16_t)(s_ide_channels[channel].busm + reg - 0x0E), data);
    }

    if(reg > 0x07 && reg < 0x0C) {
        ide_write(channel, IDE_REGISTER_CTRL, s_ide_channels[channel].irq); 
    }
}

uint8_t ide_read(uint8_t channel, uint8_t reg) {
    uint8_t data;
    if(reg > 0x07 && reg < 0x0C) {
        ide_write(channel, IDE_REGISTER_CTRL, 0x80 | s_ide_channels[channel].irq);
    }

    if(reg < 0x08) {
        data = port_read_8((uint16_t)(s_ide_channels[channel].base + reg));
    } else if(reg < 0x0C) {
        data = port_read_8((uint16_t)(s_ide_channels[channel].base + reg - 0x06));
    } else if(reg < 0x0E) {
        data = port_read_8((uint16_t)(s_ide_channels[channel].ctrl + reg - 0x0A));
    } else if(reg < 0x16) {
        data = port_read_8((uint16_t)(s_ide_channels[channel].busm + reg - 0x0E));
    }

    if(reg > 0x07 && reg < 0x0C) {
        ide_write(channel, IDE_REGISTER_CTRL, s_ide_channels[channel].irq); 
    }

    return data;
}

void ide_delay(uint8_t channel) {
    // 100 ns per call
    ide_read(channel, IDE_REGISTER_STATUS);
    ide_read(channel, IDE_REGISTER_STATUS);
    ide_read(channel, IDE_REGISTER_STATUS);
    ide_read(channel, IDE_REGISTER_STATUS);
}

int32_t ide_wait(uint8_t channel, bool advanced) {
    uint8_t status = 0;
    ide_delay(channel);

    while((status = port_read_8(s_ide_channels[channel].base + IDE_REGISTER_STATUS)) & IDE_ATA_BUSY) {
        // Wait
    }

    if(advanced) {
        status = port_read_8(s_ide_channels[channel].base + IDE_REGISTER_STATUS);

        if(status & IDE_ERROR_FAULT) {
            return -1;
        }

        if(status & IDE_ERROR_GENERIC) {
            return -2;
        }

        if(status & IDE_ERROR_DRQ) {
            return -3;
        }
    }

    return 0;
}

int32_t ide_pio_readwrite(uint8_t rw, uint8_t drive, uint64_t lba, uint8_t* buf, uint32_t numsectors) {
    if(rw > 1 || buf == nullptr) {
        return 0;
    }

    uint8_t lba_mode = 1;
    uint8_t channel = s_ide_devices[drive].channel;
    uint32_t slave = s_ide_devices[drive].drive;
    uint32_t bus = s_ide_channels[channel].base;
    uint32_t words = (numsectors * 512) / 2;

    port_write_8(bus + IDE_REGISTER_CTRL, 0x02);

    ide_wait(channel, false);

    if(s_ide_devices[drive].flags & 0x1) {
        lba_mode = 2;
    }

    uint8_t cmd = rw == IDE_READ ? IDE_COMMAND_PIO_READ : IDE_COMMAND_PIO_WRITE;
    s_ide_channels[channel].irq_wait = 0;

    if(lba_mode == 2) {
        cmd += 0x04;

        ide_write(channel, IDE_REGISTER_HDDSEL, (0x40 | (slave << 4)));
        ide_wait(channel, false);
        ide_write(channel, IDE_REGISTER_SECCOUNT0, 0x00);
        ide_write(channel, IDE_REGISTER_LBA0, (uint8_t)((lba >> 24) & 0xFF));
        ide_write(channel, IDE_REGISTER_LBA1, (uint8_t)((lba >> 32) & 0xFF));
        ide_write(channel, IDE_REGISTER_LBA2, (uint8_t)((lba >> 40) & 0xFF));
    } else if (lba_mode == 1) {
        ide_write(channel, IDE_REGISTER_HDDSEL, 0xE0 | (slave << 4) | ((lba & 0x0F000000) >> 24));
        ide_wait(channel, false);
        ide_write(channel, IDE_REGISTER_FEATURES, 0x00);
    }

    ide_write(channel, IDE_REGISTER_SECCOUNT0, (uint8_t)numsectors);
    ide_write(channel, IDE_REGISTER_LBA0, (uint8_t)(lba & 0xFF));
    ide_write(channel, IDE_REGISTER_LBA1, (uint8_t)((lba >> 8) & 0xFF));
    ide_write(channel, IDE_REGISTER_LBA2, (uint8_t)((lba >> 16) & 0xFF));

    ide_write(channel, IDE_REGISTER_COMMAND, cmd);

    if(ide_wait(channel, true) != 0) {
        return 0;
    }

    if(rw == IDE_READ) {
        port_read(bus + IDE_REGISTER_DATA, (256 * numsectors), buf);
    } else {
        port_write(bus + IDE_REGISTER_DATA, (256 * numsectors), buf);
        port_write_8(bus + IDE_REGISTER_COMMAND, IDE_COMMAND_FLUSH);
    }

    ide_wait(channel, false);

    return words * 2;
}

uint32_t ide_get_sectorsize(gbd_t* disk) {
    device_t *dev = (device_t*)disk->device;
    uint8_t type = s_ide_devices[(uint8_t)dev->io_address].type;

    if(type == 0) {
        //HDD
        return 512;
    }

    // CDROM
    return 2048;
}

uint32_t ide_get_sectorcount(gbd_t* disk) {
    device_t *dev = (device_t*)disk->device;
    uint64_t sectors = s_ide_devices[(uint8_t)dev->io_address].total_sectors;

    return (uint32_t)sectors;
}

// int ide_read_block(gbd_t* gbd, gbd_request_t* request) {
//     device_t *disk = (device_t*)gbd->device;
//     uint8_t drive = (uint8_t)disk->io_address;
//     uint8_t *buf = (uint8_t*)request->buf;
//     uint64_t sector = (uint64_t)request->start;

//     if(drive > 3 || s_ide_devices[drive].present == 0) {
//         return -6;
//     }

//     if(sector > s_ide_devices[drive].total_sectors || s_ide_devices[drive].type != 0) {
//         return -5;
//     }

//     return ide_pio_readwrite(IDE_READ, drive, sector, buf, 1);
// }

// int ide_write_block(gbd_t* gbd, gbd_request_t* request) {
//     device_t* disk = (device_t *)gbd->device;
//     uint8_t drive = (uint8_t)disk->io_address;
//     uint8_t* buf = (uint8_t *)request->buf;
//     uint64_t sector = (uint64_t)request->start;

//     if(drive > 3 || s_ide_devices[drive].present == 0) {
//         return -1;
//     }

//     if(sector > s_ide_devices[drive].total_sectors || s_ide_devices[drive].type != 0) {
//         return -2;
//     }

//     return ide_pio_readwrite(IDE_WRITE, drive, sector, buf, 1);
// }

int disk_init(void* base) {
    pci_device_t* pci = (pci_device_t *)base;

    uint16_t iobase1 = IDE_PRIMARY_CMD_BASE;
    uint16_t iobase2 = IDE_SECONDARY_CMD_BASE;
    uint16_t ctrlbase1 = IDE_PRIMARY_CTRL_BASE;
    uint16_t ctrlbase2 = IDE_SECONDARY_CTRL_BASE;
    uint16_t busmaster = pci->bar[4];
    uint16_t buf[256];

    if(pci->common.prog_interface & 0x1) {
        iobase1 = (uint16_t)pci->bar[0];
        ctrlbase1 = (uint16_t)pci->bar[1];

        if(iobase1 & 0x1) {
            iobase1--;
        }

        if(ctrlbase1 & 0x1) {
            ctrlbase1--;
        }
    }

    if(pci->common.prog_interface & 0x4) {
        iobase2 = (uint16_t)pci->bar[2];
        ctrlbase2 = (uint16_t)pci->bar[3];

        if(iobase2 & 0x1) {
            iobase2--;
        }

        if(ctrlbase2 & 0x1) {
            ctrlbase2--;
        }
    }

    s_ide_channels[IDE_PRIMARY].busm = busmaster;
    s_ide_channels[IDE_PRIMARY].base = iobase1;
    s_ide_channels[IDE_PRIMARY].ctrl = ctrlbase1;
    s_ide_channels[IDE_PRIMARY].irq = IDE_PRIMARY_IRQ;
    s_ide_channels[IDE_PRIMARY].irq_wait = 0;
    s_ide_channels[IDE_PRIMARY].dma_phys = 0;
    s_ide_channels[IDE_PRIMARY].dma_virt = 0;
    s_ide_channels[IDE_PRIMARY].dma_buf_phys = 0;
    s_ide_channels[IDE_PRIMARY].dma_buf_virt = 0;

    s_ide_channels[IDE_SECONDARY].busm = busmaster;
    s_ide_channels[IDE_SECONDARY].base = iobase2;
    s_ide_channels[IDE_SECONDARY].ctrl = ctrlbase2;
    s_ide_channels[IDE_SECONDARY].irq = IDE_SECONDARY_IRQ;
    s_ide_channels[IDE_SECONDARY].irq_wait = 0;
    s_ide_channels[IDE_SECONDARY].dma_phys = 0;
    s_ide_channels[IDE_SECONDARY].dma_virt = 0;
    s_ide_channels[IDE_SECONDARY].dma_buf_phys = 0;
    s_ide_channels[IDE_SECONDARY].dma_buf_virt = 0;

    interrupt_register(IDE_PRIMARY_IRQ, ide_irq_handler0);
    interrupt_register(IDE_SECONDARY_IRQ, ide_irq_handler1);

    // Disable IRQs, use polling instead
    ide_write(IDE_PRIMARY, IDE_REGISTER_CTRL, 2);
    ide_write(IDE_SECONDARY, IDE_REGISTER_CTRL, 2);

    // Enumerate devices by pinging each of them with IDE_IDENTIFY
    int count = 0, type = 0;
    bool error = false;
    for(int i = 0; i < IDE_CHANNELS_PER_CTRL; i++) {
        for(int j = 0; j < IDE_DEVICES_PER_CHANNEL; j++) {
            s_ide_devices[count].present = 0;

            // Select drive
            ide_write(i, IDE_REGISTER_HDDSEL, 0xA0 | (j << 4));
            ide_delay(i);

            // Send IDE_IDENTIFY
            ide_write(i, IDE_REGISTER_SECCOUNT0, 0);
            ide_write(i, IDE_REGISTER_LBA0, 0);
            ide_write(i, IDE_REGISTER_LBA1, 0);
            ide_write(i, IDE_REGISTER_LBA2, 0);
            ide_write(i, IDE_REGISTER_COMMAND, IDE_COMMAND_IDENTIFY);
            ide_delay(i);

            // Poll for response
            uint8_t status = port_read_8(s_ide_channels[i].base + IDE_REGISTER_STATUS);
            if(status == 0 || status == 0x7F || status == 0xFF) {
                count++;
                continue;
            }

            while(true) {
                status = port_read_8(s_ide_channels[i].base + IDE_REGISTER_STATUS);
                if(status & 0x1) {
                    error = true;
                    break;
                }

                if(!(status & IDE_ATA_BUSY) && (status & IDE_ATA_DRQ)) {
                    break;
                }
            }

            // ATAPI information
            if(!error) {
                uint8_t cl = port_read_8(s_ide_channels[i].base + IDE_REGISTER_LBA1);
                uint8_t ch = port_read_8(s_ide_channels[i].base + IDE_REGISTER_LBA2);

                if(cl == 0x14 && ch == 0xEB) {
                    //PATAPI
                    type = 1;
                } else if(cl == 0x69 && ch == 0x96) {
                    //SATAPI
                    type = 1;
                } else {
                    count++;
                    continue;
                }

                ide_write(i, IDE_REGISTER_COMMAND, IDE_COMMAND_PACKET);
                ide_delay(i);
            }

            // Identification space
            port_read(s_ide_channels[i].base + IDE_REGISTER_DATA, 256, (uint8_t *)buf);

            // Device parameters
            s_ide_devices[count].present = 1;
            s_ide_devices[count].type = type;
            s_ide_devices[count].channel = i;
            s_ide_devices[count].drive = j;
            s_ide_devices[count].signature = (*(uint16_t *)(buf));
            s_ide_devices[count].capabilities = (*(uint16_t*)(buf + 49));
            s_ide_devices[count].commandset = (*(uint32_t*)(buf + 82));

            // Geometry
            uint32_t lba28 = (*(uint32_t *)(buf + 60));
            uint64_t lba48 = (*(uint64_t*)(buf + 100));

            if(lba48) {
                s_ide_devices[count].total_sectors = lba48;
                s_ide_devices[count].cylinders = *((uint16_t *)(buf + 1));
                s_ide_devices[count].heads_per_cylinder = *((uint16_t *)(buf + 3));
                s_ide_devices[count].secs_per_head = *((uint64_t *)(buf + 6));
                s_ide_devices[count].flags |= 0x1;
            } else if(lba28) {
                s_ide_devices[count].total_sectors = lba28;
                s_ide_devices[count].cylinders = *((uint16_t *)(buf + 1));
                s_ide_devices[count].heads_per_cylinder = *((uint16_t *)(buf + 3));
                s_ide_devices[count].secs_per_head = *((uint64_t *)(buf + 6));
            } else {
                s_ide_devices[count].total_sectors = 0;
                s_ide_devices[count].cylinders = 0;
                s_ide_devices[count].heads_per_cylinder = 0;
                s_ide_devices[count].secs_per_head = 0;
            }

            // Register filesystem
            s_ide_dev[count].real_device = &(s_ide_channels[0]);
            s_ide_dev[count].type = TYPECODE_DISK;
            s_ide_dev[count].generic_device = &s_ide_gbd[count];
            s_ide_dev[count].io_address = count;

            // IDE Device
            s_ide_gbd[count].device = &s_ide_dev[count];
            // s_ide_gbd[count].write_block = ide_write_block;
            // s_ide_gbd[count].read_block = ide_read_block;
            // s_ide_gbd[count].block_size = ide_get_sectorsize;
            // s_ide_gbd[count].total_blocks = ide_get_sectorcount;

            count++;
        }
    }

    return 0;
}