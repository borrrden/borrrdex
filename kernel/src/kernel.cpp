#include "acpi/xsdt.h"
#include "acpi/fadt.h"
#include "KernelUtil.h"
#include "Memory.h"
#include "io/rtc.h"
#include "graphics/Clock.h"
#include "userinput/mouse.h"

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

    ps2_mouse_process_packet();
}

extern "C" void _start(BootInfo* bootInfo) {
    KernelInfo kernelInfo = InitializeKernel(bootInfo);
    PageTableManager* pageTableManager = kernelInfo.pageTableManager;

    if(bootInfo->rsdp) {
        XSDT* xsdt = (XSDT *)bootInfo->rsdp->xdst_address;
        FADT* fadt = (FADT *)xsdt_get_table(xsdt, FADT_SIGNATURE);
        if(fadt && fadt_valid(fadt)) {
            century_register = fadt->century;
        }
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