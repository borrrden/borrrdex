#include "KernelUtil.h"
#include "gdt/gdt.h"
#include "interrupts/idt.h"
#include "interrupts/interrupts.h"
#include "userinput/keyboard.h"
#include "userinput/keymaps.h"
#include "io/io.h"

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

    uint64_t kernelSize = (uint64_t)&_KernelEnd - (uint64_t)&_KernelStart;
    uint64_t kernelPages = (uint64_t)kernelSize / 4096 + 1;
    allocator->LockPages(&_KernelStart, kernelPages);

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

IDTR idtr;
void PrepareInterrupts() {
    idtr.limit = 0x0FFFF;
    idtr.offset = (uint64_t)PageFrameAllocator::SharedAllocator()->RequestPage();

    IDTDescEntry* int_PageFault = (IDTDescEntry *)(idtr.offset + 0x0E * sizeof(IDTDescEntry));
    int_PageFault->SetOffset((uint64_t)PageFault_Handler);
    int_PageFault->type_attr = IDT_TA_InterruptGate;
    int_PageFault->selector = 0x08;

    IDTDescEntry* int_DoubleFault = (IDTDescEntry *)(idtr.offset + 0x8 * sizeof(IDTDescEntry));
    int_DoubleFault->SetOffset((uint64_t)DoubleFault_Handler);
    int_DoubleFault->type_attr = IDT_TA_InterruptGate;
    int_DoubleFault->selector = 0x08;

    IDTDescEntry* int_GPFault = (IDTDescEntry *)(idtr.offset + 0xD * sizeof(IDTDescEntry));
    int_GPFault->SetOffset((uint64_t)GPFault_Handler);
    int_GPFault->type_attr = IDT_TA_InterruptGate;
    int_GPFault->selector = 0x08;

    IDTDescEntry* int_Keyboard = (IDTDescEntry *)(idtr.offset + 0x21 * sizeof(IDTDescEntry));
    int_Keyboard->SetOffset((uint64_t)KeyboardInt_Handler);
    int_Keyboard->type_attr = IDT_TA_InterruptGate;
    int_Keyboard->selector = 0x08;

    outb(0x43, 0x36);
    uint16_t divisor = get_pit_divisor(20);
    outb(0x40, divisor & 0xFF);
    outb(0x40, divisor >> 8);

    IDTDescEntry* int_Timer = (IDTDescEntry *)(idtr.offset + 0x20 * sizeof(IDTDescEntry));
    int_Timer->SetOffset((uint64_t)TimerInt_Handler);
    int_Timer->type_attr = IDT_TA_InterruptGate;
    int_Timer->selector = 0x08;

    asm ("lidt %0" :: "m" (idtr));

    RemapPIC();

    outb(PIC1_DATA, 0b11111100);
    outb(PIC2_DATA, 0b11111111);

    asm ("sti");
}

BasicRenderer r(NULL, NULL);
KernelInfo InitializeKernel(BootInfo* bootInfo) {
    r = BasicRenderer(bootInfo->framebuffer, bootInfo->psf1_font);
    GlobalRenderer = &r;

    GDTDescriptor gdtDescriptor;
    gdtDescriptor.Size = sizeof(GDT) - 1;
    gdtDescriptor.Offset = (uint64_t)&DefaultGDT;
    LoadGDT(&gdtDescriptor);

    PrepareMemory(bootInfo);

    memset(bootInfo->framebuffer->baseAddress, 0, bootInfo->framebuffer->bufferSize);
    
    PrepareInterrupts();

    KeyboardMapFunction = JP109Keyboard::translate;
    
    return {
        &gPageTableManager
    };
}