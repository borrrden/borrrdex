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
#include "fs/vfs.h"
#include "stdatomic.h"

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

    // Before Uncommenting the below you will need to use KUDOS tfstool to create a disk
    // and attach it to the virtual machine.  More details about Trivial Filesystem here:
    // https://kudos.readthedocs.io/en/latest/trivial-filesystem.html
    //
    // I entrust you to be able to figure out how to build and use tfstool from their repo
    // (it is in the kudos/utils/ folder)
    
    /*
    openfile_t shellFile = vfs_open("[disk]/shell");
    if(shellFile >= 0) {
        VirtualFilesystemFile vf(shellFile);
        GlobalRenderer->Printf("Opened file 'shell' on '[disk]' (size %d bytes [%d KiB])!\n", vf.size(), vf.size() / 1024);
    }

    GlobalRenderer->Printf("Trying to open test.txt\n");
    openfile_t testFile = vfs_open("[disk]/test.txt");
    if(testFile == VFS_NOT_FOUND) {
        const char* testText = "This is a test!";
        GlobalRenderer->Printf("Creating test.txt\n");
        int r = vfs_create("[disk]/test.txt", strnlen(testText, 64));
        testFile = vfs_open("[disk]/test.txt");
        if(testFile >= 0) {
            VirtualFilesystemFile vf(testFile);
            vf.write((void *)testText, strnlen(testText, 64));
        }
    }
    
    testFile = vfs_open("[disk]/test.txt");
    if(testFile >= 0) {
        VirtualFilesystemFile vf(testFile);
        char buffer[64];
        int num_read = vf.read(buffer, 64);
        buffer[num_read] = 0;
        GlobalRenderer->Printf("Read contents of test.txt: %s\n", buffer);
    }
    
    GlobalRenderer->Printf("Removing test.txt\n");
    vfs_remove("[disk]/test.txt");
    */


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