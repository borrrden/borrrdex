#include "KernelUtil.h"
#include "arch/x86_64/gdt/gdt.h"
#include "interrupts/idt.h"
#include "interrupts/interrupts.h"
#include "userinput/keyboard.h"
#include "userinput/keymaps.h"
#include "userinput/mouse.h"
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

IDTR idtr;
void SetIDTGate(void(*handler)(struct interrupt_frame *), uint8_t entryOffset, uint8_t type_attr, uint8_t selector) {
    IDTDescEntry* interrupt = (IDTDescEntry *)(idtr.offset + entryOffset * sizeof(IDTDescEntry));
    interrupt->SetOffset((uint64_t)handler);
    interrupt->type_attr = type_attr;
    interrupt->ist = 0;
    interrupt->selector = selector;
}

void PrepareInterrupts() {
    idtr.limit = 0x0FFFF;
    idtr.offset = (uint64_t)PageFrameAllocator::SharedAllocator()->RequestPage();

    SetIDTGate(PageFault_Handler, 0x0E, IDT_TA_InterruptGate, 0x08);
    SetIDTGate(DoubleFault_Handler, 0x08, IDT_TA_InterruptGate, 0x08);
    SetIDTGate(GPFault_Handler, 0x0D, IDT_TA_InterruptGate, 0x08);
    SetIDTGate(KeyboardInt_Handler, 0x21, IDT_TA_InterruptGate, 0x08);
    SetIDTGate(TimerInt_Handler, 0x20, IDT_TA_InterruptGate, 0x08);

    port_write_8(0x43, 0x36);
    uint16_t divisor = get_pit_divisor(20);
    port_write_8(0x40, divisor & 0xFF);
    port_write_8(0x40, divisor >> 8);

    SetIDTGate(RTCInt_Handler, 0x28, IDT_TA_InterruptGate, 0x08);

    ps2_mouse_init();
    SetIDTGate(MouseInt_Handler, 0x2C, IDT_TA_InterruptGate, 0x08);


    asm ("lidt %0" :: "m" (idtr));

    RemapPIC();

    port_write_8(PIC1_DATA, 0b11111001);
    port_write_8(PIC2_DATA, 0b11101110);

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
    
    PrepareInterrupts();

    KeyboardMapFunction = JP109Keyboard::translate;
    
    return {
        &gPageTableManager
    };
}
