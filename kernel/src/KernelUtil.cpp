#include "KernelUtil.h"
#include "arch/x86_64/gdt/gdt.h"
#include "arch/x86_64/interrupt/interrupt.h"
#include "arch/x86_64/pic.h"
#include "userinput/keyboard.h"
#include "userinput/keymaps.h"
#include "userinput/mouse.h"
#include "arch/x86_64/io/io.h"

constexpr uint32_t PIT_FREQUENCY = 1193180U;

constexpr uint16_t get_pit_divisor(uint16_t hz) {
    uint32_t result = PIT_FREQUENCY / hz;
    return result > UINT16_MAX ? UINT16_MAX : result;
}

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
    port_write_8(0x43, 0x36);
    uint16_t divisor = get_pit_divisor(20);
    port_write_8(0x40, divisor & 0xFF);
    port_write_8(0x40, divisor >> 8);

    ps2_mouse_init();

    interrupt_init();
}

static BasicRenderer r(NULL, NULL);
KernelInfo InitializeKernel(BootInfo* bootInfo) {
    r = BasicRenderer(bootInfo->framebuffer, bootInfo->psf1_font);
    GlobalRenderer = &r;
    uint64_t test = (uint64_t)GlobalRenderer;

    KeyboardMapFunction = JP109Keyboard::translate;

    PrepareMemory(bootInfo);
    PrepareInterrupts();
    
    return {
        &gPageTableManager
    };
}
