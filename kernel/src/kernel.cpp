#include "acpi/xsdt.h"
#include "acpi/fadt.h"
#include "acpi/mcfg.h"
#include "KernelUtil.h"
#include "memory/Memory.h"
#include "arch/x86_64/io/rtc.h"
#include "arch/x86_64/tss.h"
#include "arch/x86_64/io/serial.h"
#include "graphics/Clock.h"
#include "arch/x86_64/cpuid.h"
#include "arch/x86_64/interrupt/interrupt.h"
#include "drivers/x86_64/pit.h"
#include "Panic.h"
#include "string.h"
#include "paging/PageFrameAllocator.h"
#include "fs/vfs.h"
#include "stdatomic.h"
#include "init/common.h"
#include "thread.h"
#include "drivers/polltty.h"
#include "stalloc.h"

#include <cstddef>

extern "C" __attribute__((noreturn)) void __enter_ring3(uint64_t new_stack, uint64_t jump_addr, void* pageTable);

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
    stalloc_init();
    uart_init();

    KernelInfo kernelInfo = InitializeKernel(bootInfo);
    if(bootInfo->rsdp) {
        XSDT xsdt((void *)bootInfo->rsdp->xdst_address);
        FADT fadt(xsdt.get(FADT::signature));
        if(fadt.is_valid()) {
            century_register = fadt.data()->century;
        }
    }

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

    tid_t startup_thread = thread_create(init_startup_thread, 0);
    thread_run(startup_thread);

    thread_switch();
    //process_start("[disk]/usertest", nullptr);
    // userPages.WriteToCR3();
    // __enter_ring3(0x8001000 - 0x10, hdr.e_entry, newPage);

    
}