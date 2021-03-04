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
#include <elf.h>

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

extern "C" void user_landing(int status) {
     GlobalRenderer->Printf("User process return with status %d", status);
     while(true) {
        asm("hlt");
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
    
    openfile_t shellFile = vfs_open("[disk]/usertest");
    if(shellFile >= 0) {
        VirtualFilesystemFile vf(shellFile, false);
        GlobalRenderer->Printf("Opened file 'usertest' on '[disk]' (size %d bytes [%d KiB])!\n", vf.size(), vf.size() / 1024);
    }

    VirtualFilesystemFile vf(shellFile, true);
    Elf64_Ehdr hdr;
    int read = vf.read(&hdr, sizeof(Elf64_Ehdr));
    if(read == sizeof(Elf64_Ehdr)) {
        GlobalRenderer->Printf("Read the %d byte ELF header", read);
    }
    
    if(
		memcmp(&hdr.e_ident[EI_MAG0], ELFMAG, SELFMAG) ||
		hdr.e_ident[EI_CLASS] != ELFCLASS64 ||
		hdr.e_ident[EI_DATA] != ELFDATA2LSB ||
		hdr.e_type != ET_EXEC ||
		hdr.e_machine != EM_X86_64 ||
		hdr.e_version != EV_CURRENT
	) {
        GlobalRenderer->Printf("Invalid file...", read);
    }

    vf.seek(hdr.e_phoff);
	size_t size = hdr.e_phnum * hdr.e_phentsize;
    Elf64_Phdr* phdrs = (Elf64_Phdr *)PageFrameAllocator::SharedAllocator()->RequestPages((size + 4095) / 4096);
    read = vf.read(phdrs, size);

    PageTable* newPage = (PageTable *)PageFrameAllocator::SharedAllocator()->RequestPage();
    memset(newPage, 0, 0x1000);
    PageTableManager userPages(newPage);

    for(
		Elf64_Phdr* phdr = phdrs;
		(char *)phdr < (char *)phdrs + hdr.e_phnum * hdr.e_phentsize;
		phdr = (Elf64_Phdr*)((char *)phdr + hdr.e_phentsize)
	) {
        switch(phdr->p_type) {
            case PT_LOAD:
            {
                int pages = (phdr->p_memsz + 0x1000 - 1) / 0x1000;
				Elf64_Addr segment = phdr->p_vaddr;
                void* newPages = PageFrameAllocator::SharedAllocator()->RequestPages(pages);
                for(int i = 0; i < pages; i++) {
                    uint64_t p = (phdr->p_vaddr + (i * 0x1000));
                    userPages.MapMemory((void *)p, (void *)((uint64_t)newPages + (i * 0x1000)), true);
                }
                vf.seek(phdr->p_offset);
                vf.read(newPages, phdr->p_filesz);
            }
        }
    }

    void* user_stack = PageFrameAllocator::SharedAllocator()->RequestPage();
    uint64_t rsp;
    asm volatile("mov %%rsp, %0" : "=d"(rsp));
    tss_install(0, rsp);

    userPages.MapMemory((void *)0x8000000, user_stack, true);
    GlobalRenderer->Printf("\nEntering userland...\n\n");

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
    userPages.WriteToCR3();
    __enter_ring3(0x8001000 - 0x10, hdr.e_entry, newPage);
}