#include "acpi/xsdt.h"
#include "acpi/fadt.h"
#include "acpi/mcfg.h"
#include "KernelUtil.h"
#include "Memory.h"
#include "arch/x86_64/io/rtc.h"
#include "arch/x86_64/tss.h"
#include "arch/x86_64/io/serial.h"
#include "graphics/Clock.h"
#include "arch/x86_64/cpuid.h"
#include "arch/x86_64/interrupt/interrupt.h"
#include "drivers/x86_64/pit.h"
#include "Panic.h"
#include "string.h"
#include "ring3test.h"
#include "paging/PageFrameAllocator.h"

#include <cstddef>

extern "C" __attribute__((noreturn)) void __enter_ring3(uint64_t new_stack, uint64_t jump_addr);

struct KernelUpdateEntries {
    BasicRenderer *renderer;
    Clock* clock;
    uint16_t tickCount;
};


void render(datetime_t* dt, void* context) {
    KernelUpdateEntries* updateEntries = (KernelUpdateEntries *)context;
    uint16_t tickCount = updateEntries->tickCount++;
    if((tickCount % updateEntries->renderer->get_update_ticks()) == 0) {
        updateEntries->renderer->tick(dt);
    }

    if((tickCount % updateEntries->clock->get_update_ticks()) == 0) {
        updateEntries->clock->tick(dt);
    }
}

extern "C" void _start(BootInfo* bootInfo) {
    uart_init();
    memset(bootInfo->framebuffer->baseAddress, 0, bootInfo->framebuffer->bufferSize);

    KernelInfo kernelInfo = InitializeKernel(bootInfo);
    if(bootInfo->rsdp) {
        XSDT xsdt((void *)bootInfo->rsdp->xdst_address);
        FADT fadt(xsdt.get(FADT::signature));
        if(fadt.is_valid()) {
            century_register = fadt.data()->century;
        }
    }


    uint64_t rsp;
    asm volatile("mov %%rsp, %0" : "=d"(rsp));
    tss_install(0, rsp);

    Clock clk;
    KernelUpdateEntries u {
        GlobalRenderer,
        &clk,
        0
    };

    rtc_chain_t renderChain = {
        render,
        &u,
        NULL
    };

    register_rtc_cb(&renderChain);
    void* user_stack = PageFrameAllocator::SharedAllocator()->RequestPage();
    GlobalRenderer->Printf("\nEntering userland...\n\n");
    __enter_ring3((uint64_t)user_stack, (uint64_t)main);
    
    while(true) {
        asm("hlt");
    }
}