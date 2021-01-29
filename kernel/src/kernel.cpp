#include "acpi/xsdt.h"
#include "acpi/mcfg.h"
#include "KernelUtil.h"
#include "Memory.h"

#include <cstddef>

extern "C" void _start(BootInfo* bootInfo) {
    KernelInfo kernelInfo = InitializeKernel(bootInfo);
    PageTableManager* pageTableManager = kernelInfo.pageTableManager;

    // if(bootInfo->rsdp) {
    //     XSDT* xsdt = (XSDT *)bootInfo->rsdp->xdst_address;
    //     MCFG* mcfg = (MCFG *)xsdt_get_table(xsdt, MCFG_SIGNATURE);
    //     if(mcfg && mcfg_valid(mcfg)) {
    //         mcfg_print(mcfg);
    //     }
    // }
    

    while(true);
}