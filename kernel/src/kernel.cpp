#include "acpi/xsdt.h"
#include "acpi/fadt.h"
#include "acpi/mcfg.h"
#include "KernelUtil.h"
#include "Memory.h"
#include "io/rtc.h"
#include "io/serial.h"
#include "graphics/Clock.h"
#include "userinput/mouse.h"
#include "arch/x86_64/cpuid.h"

#include <cstddef>

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

    KernelInfo kernelInfo = InitializeKernel(bootInfo);
    PageTableManager* pageTableManager = kernelInfo.pageTableManager;

    CPUIDFeatures features;
    GlobalRenderer->Printf("CPU identifies as: %s (%s)\n", features.vendor(), features.brand());
    GlobalRenderer->Printf("ECX1\t\t\tEDX1\t\tEBX7\t\tECX7\t\tECX81\t\tEDX81\n");
    GlobalRenderer->Printf("SSE3: %d\tMSR: %d\tFSGSBASE: %d\tPREFETCHWT1: %d\tLAHF: %d\tSYSCALL: %d\n", 
        features.SSE3(), features.MSR(), features.FSGSBASE(), features.PREFETCHWT1(), features.LAHF(), features.SYSCALL());

    if(bootInfo->rsdp) {
        XSDT* xsdt = (XSDT *)bootInfo->rsdp->xdst_address;
        FADT* fadt = (FADT *)xsdt_get_table(xsdt, FADT_SIGNATURE);
        if(fadt && fadt_valid(fadt)) {
            century_register = fadt->century;
        }

        // MCFG* mcfg = (MCFG *)xsdt_get_table(xsdt, MCFG_SIGNATURE);
        // if(mcfg && mcfg_valid(mcfg)) {
        //     mcfg_print(mcfg);
        // }
    }

    Clock clk;
    KernelUpdateEntries u {
        GlobalRenderer,
        &clk,
        0
    };

    rtc_init_interrupt();
    rtc_chain_t renderChain = {
        render,
        &u,
        NULL
    };

    register_rtc_cb(&renderChain);
    while(true) {
        asm("hlt");
    }
}