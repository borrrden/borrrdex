#include "KernelUtil.h"
#include "arch/x86_64/gdt/gdt.h"
#include "arch/x86_64/interrupt/interrupt.h"
#include "arch/x86_64/pic.h"
#include "drivers/x86_64/pit.h"
#include "drivers/x86_64/keyboard.h"
#include "userinput/keymaps.h"
#include "arch/x86_64/io/io.h"

PageTableManager gPageTableManager(NULL);

static void PrepareMemory(BootInfo* bootInfo) {
    uint64_t mMapEntries = bootInfo->mMapSize / bootInfo->mMapDescriptorSize;
    PageFrameAllocator* allocator = PageFrameAllocator::SharedAllocator();
    allocator->ReadEFIMemoryMap(bootInfo->mMap, bootInfo->mMapSize, bootInfo->mMapDescriptorSize);
    allocator->LockPages(0x0, 256);

    PageTable* PML4 = (PageTable *)allocator->RequestPage();
    memset(PML4, 0, 0x1000);
    gPageTableManager = PageTableManager(PML4);
    
    for(uint64_t t = 0; t < GetMemorySize(bootInfo->mMap, mMapEntries, bootInfo->mMapDescriptorSize); t += 0x1000) {
        gPageTableManager.MapMemory((void *)t, (void *)t);
    }

    for(uint64_t t = 0xb0000000; t < 0xb0000000 + 0x10000000; t += 0x1000) {
        gPageTableManager.MapMemory((void *)t, (void *)t);
    }

    uint64_t fbBase = (uint64_t)bootInfo->framebuffer->baseAddress;
    uint64_t fbSize = (uint64_t)bootInfo->framebuffer->bufferSize + 0x1000;
    PageFrameAllocator::SharedAllocator()->LockPages((void *)fbBase, fbSize / 0x1000 + 1);
    for(uint64_t t = fbBase; t < fbBase + fbSize; t += 0x1000) {
        gPageTableManager.MapMemory((void *)t, (void *)t);
    }

    asm ("mov %0, %%cr3" : : "r" (PML4));
}

void PrepareInterrupts() {
    interrupt_init();

    keyboard_init();
    //ps2_mouse_init();
    //pit_init();
    rtc_init();
}

static BasicRenderer r(NULL, NULL);
KernelInfo InitializeKernel(BootInfo* bootInfo) {
    r = BasicRenderer(bootInfo->framebuffer, bootInfo->psf1_font);
    GlobalRenderer = &r;
    
    GlobalRenderer->Printf("borrrdex - An operating system for learning operating systems\n");
    GlobalRenderer->Printf("=============================================================\n");
    GlobalRenderer->Printf("\n");
    GlobalRenderer->Printf("borrrdex is heavily based on PonchoOS and KUDOS\n");
    GlobalRenderer->Printf("\n");

    GlobalRenderer->Printf("Setting up memory...\n");
    PrepareMemory(bootInfo);
    
    GlobalRenderer->Printf("Setting up interrupts...\n");
    KeyboardMapFunction = JP109Keyboard::translate;
    PrepareInterrupts();
    
    return {
        &gPageTableManager
    };
}
