#include "acpi/xsdt.h"
#include "acpi/fadt.h"
#include "KernelUtil.h"
#include "Memory.h"
#include "io/rtc.h"

#include <cstddef>

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
    
    read_rtc();

    while(true);
}