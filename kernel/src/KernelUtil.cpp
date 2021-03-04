#include "KernelUtil.h"
#include "string.h"
#include "arch/x86_64/gdt/gdt.h"
#include "arch/x86_64/interrupt/interrupt.h"
#include "arch/x86_64/pic.h"
#include "drivers/x86_64/pit.h"
#include "drivers/x86_64/keyboard.h"
#include "userinput/keymaps.h"
#include "arch/x86_64/io/io.h"
#include "proc/syscall.h"
#include "stalloc.h"
#include "arch/x86_64/io/serial.h"
#include "pci/pci.h"
#include "acpi/xsdt.h"
#include "acpi/mcfg.h"
#include "drivers/disk.h"
#include "drivers/ahci/AHCIController.h"
#include "fs/vfs.h"
#include "memory/heap.h"

PageTableManager gPageTableManager(NULL);

static void PrepareMemory(BootInfo* bootInfo) {
    uint64_t mMapEntries = bootInfo->mMapSize / bootInfo->mMapDescriptorSize;
    PageFrameAllocator* allocator = PageFrameAllocator::SharedAllocator();
    allocator->ReadEFIMemoryMap(bootInfo->mMap, bootInfo->mMapSize, bootInfo->mMapDescriptorSize);

    uint64_t memorySize = GetMemorySize(bootInfo->mMap, mMapEntries, bootInfo->mMapDescriptorSize);
    PageTableManager::SetSystemMemorySize(memorySize);
    PageTableManager::SetFramebuffer(bootInfo->framebuffer);

    PageTable* PML4 = (PageTable *)allocator->RequestPage();
    memset(PML4, 0, 0x1000);
    gPageTableManager = PageTableManager(PML4);
    gPageTableManager.WriteToCR3();
}

PageTableManager* KernelPageTableManager() {
    return &gPageTableManager;
}

void PrepareInterrupts() {
    interrupt_init();
    syscall_init();

    keyboard_init();
    //ps2_mouse_init();
    pit_init();
    rtc_init();
}

void PrepareDisks() {
    vfs_init();
    vfs_mount_all();
}

static BasicRenderer r(NULL, NULL);
static void* s_system_rsdp_address;
KernelInfo InitializeKernel(BootInfo* bootInfo) {
    stalloc_init();

    r = BasicRenderer(bootInfo->framebuffer, bootInfo->psf1_font);
    GlobalRenderer = &r;

    GlobalRenderer->Clear();
    
    GlobalRenderer->Printf("borrrdex - An operating system for learning operating systems\n");
    GlobalRenderer->Printf("=============================================================\n");
    GlobalRenderer->Printf("\n");
    GlobalRenderer->Printf("borrrdex is heavily based on PonchoOS and KUDOS\n");
    GlobalRenderer->Printf("\n");

    GlobalRenderer->Printf("Setting up memory...\n");
    PrepareMemory(bootInfo);

    heap_init((void *)0x90000000, 0x10);

    GlobalRenderer->Printf("Setting up interrupts...\n");
    KeyboardMapFunction = JP109Keyboard::translate;
    PrepareInterrupts();
    s_system_rsdp_address = bootInfo->rsdp;

    modules_init();

    PrepareDisks();
    
    return {
        &gPageTableManager
    };
}

const void* SystemRSDPAddress() {
    return s_system_rsdp_address;
}

void* operator new(unsigned long size) {
    return kmalloc(size);
}

void operator delete(void* address) {
    return kfree(address);
}