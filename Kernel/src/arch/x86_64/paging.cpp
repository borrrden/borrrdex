#include <paging.h>
#include <idt.h>
#include <kstring.h>
#include <panic.h>
#include <logging.h>
#include <kassert.h>
#include <stacktrace.h>

constexpr uint16_t KERNEL_HEAP_PDPT_INDEX = 511;
constexpr uint16_t KERNEL_HEAP_PML4_INDEX = 511;

static void page_fault_handler(void*, register_context* regs) {
    uint64_t fault_address;
    asm volatile("movq %%cr2, %0" : "=r"(fault_address));

    int error_code = idt::get_err_code();
    int present = !(error_code & 0x1);
    int rw = error_code & 0x2;
    int us = error_code & 0x4;
    int reserved = error_code & 0x8;
    int id = error_code & 0x10;

    asm("cli");

    log::error("page fault");
    log::info("Register Dump:\nrip:%x, rax: %x, rbx: %x, rcx: %x, rdx: %x, rsi: %x, rdi: %x, rsp: %x, rbp: %x",
                regs->rip, regs->rax, regs->rbx, regs->rcx, regs->rdx, regs->rsi, regs->rdi, regs->rsp, regs->rbp);
    log::info("Fault address: %x", fault_address);
    if(present) {
        log::info("Page not present");
    }

    if(rw) {
        log::info("Read Only");
    }

    if(us) {
        log::info("User mode process tried to access kernel memory");
    }

    if(reserved) {
        log::info("Reserved");
    }

    if(id) {
        log::info("Instruction Fetch");
    }

    log::info("Stack trace:");
    print_stack_trace(regs->rbp);
    log::info("End stack trace.");

    char temp[19];
    itoa(regs->rip, temp, 16);
    temp[18] = 0;

    char temp2[19];
    itoa(fault_address, temp2, 16);
    temp2[18] = 0;

    const char* reasons[]{"Page Fault", "RIP: ", temp, "Address: ", temp2};

    kernel_panic(reasons, 5);
    __builtin_unreachable();
}

uint64_t kernel_pml4_phys;

namespace memory {
    pml4_t kernel_pml4 __attribute__((aligned(4096)));
    pdpt_t kernel_pdpt __attribute__((aligned(4096)));
    page_dir_t kernel_dir __attribute__((aligned(4096)));
    page_dir_t kernel_heap_dir __attribute__((aligned(4096)));
    page_t kernel_heap_dir_tables[TABLES_PER_DIR][PAGES_PER_TABLE] __attribute__((aligned(4096)));
    page_dir_t io_dirs[4] __attribute__((aligned(4096)));

    void initialize_virtual_memory() {
        idt::register_interrupt_handler(14, page_fault_handler);

        memset(kernel_pml4, 0, sizeof(pml4_t));
        memset(kernel_pdpt, 0, sizeof(pdpt_t));
        memset(kernel_heap_dir, 0, sizeof(page_dir_t));
        set_page_frame(&(kernel_pml4[PML4_GET_INDEX(KERNEL_VIRTUAL_BASE)]), ((uint64_t)kernel_pdpt - KERNEL_VIRTUAL_BASE));
        kernel_pml4[PML4_GET_INDEX(KERNEL_VIRTUAL_BASE)] |= (TABLE_WRITEABLE | TABLE_PRESENT);
        kernel_pml4[0] = kernel_pml4[PML4_GET_INDEX(KERNEL_VIRTUAL_BASE)];

        kernel_pdpt[PDPT_GET_INDEX(KERNEL_VIRTUAL_BASE)] = ((uint64_t)kernel_dir - KERNEL_VIRTUAL_BASE) | (TABLE_WRITEABLE | TABLE_PRESENT);
        for(int j = 0; j < TABLES_PER_DIR; j++) {
            kernel_dir[j] = (PAGE_SIZE_2M * j) | (PDE_2M | TABLE_WRITEABLE | TABLE_PRESENT);
        }

        kernel_pdpt[KERNEL_HEAP_PDPT_INDEX] = TABLE_WRITEABLE | TABLE_PRESENT;
        set_page_frame(&(kernel_pdpt[KERNEL_HEAP_PDPT_INDEX]), (uint64_t)kernel_heap_dir - KERNEL_VIRTUAL_BASE);

        for(int i = 0; i < 4; i++) {
            kernel_pdpt[PDPT_GET_INDEX(IO_VIRTUAL_BASE) + i] = ((uint64_t)io_dirs[i] - KERNEL_VIRTUAL_BASE) | (TABLE_WRITEABLE | TABLE_PRESENT);
            for(int j = 0; j < TABLES_PER_DIR; j++) {
                io_dirs[i][j] = (PAGE_SIZE_1G * i + PAGE_SIZE_2M * j) | (PDE_2M | TABLE_WRITEABLE | TABLE_PRESENT | PDE_CACHE_DISABLED);
            }
        }

        kernel_pdpt[0] = kernel_pdpt[PDPT_GET_INDEX(KERNEL_VIRTUAL_BASE)]; // Map low memory for SMP
        for(int i = 0; i < TABLES_PER_DIR; i++) {
            memset(&(kernel_heap_dir_tables[i]), 0, sizeof(page_t)*PAGES_PER_TABLE);
        }

        kernel_pml4_phys = (uint64_t)kernel_pml4 - KERNEL_VIRTUAL_BASE;
        asm("mov %%rax, %%cr3" :: "a"(kernel_pml4_phys));
    }
    void* kernel_allocate_4k_pages(uint64_t amount) {
        uint64_t offset = 0, page_dir_offset = 0, counter = 0, address = 0;

        uint64_t pml4_index = KERNEL_HEAP_PML4_INDEX;
        uint64_t pdpt_index = KERNEL_HEAP_PDPT_INDEX;

        // First pass through already allocated tables
        for(int i = 0; i < TABLES_PER_DIR; i++) {
            if(kernel_heap_dir[i] & TABLE_PRESENT && !(kernel_heap_dir[i] & PDE_2M)) {
                for(int j = 0; j < TABLES_PER_DIR; j++) {
                    if(kernel_heap_dir_tables[i][j] & TABLE_PRESENT) {
                        page_dir_offset = i;
                        offset = j + 1;
                        counter = 0;
                        continue;
                    }

                    counter++;
                    if(counter >= amount) {
                        address = (PDPT_SIZE * pml4_index) + (pdpt_index * PAGE_SIZE_1G) + (page_dir_offset * PAGE_SIZE_2M) + (offset * PAGE_SIZE_4K);
                        address |= 0xFFFF000000000000;
                        while(counter--) {
                            if(offset >= 512) {
                                page_dir_offset++;
                                offset = 0;
                            }

                            kernel_heap_dir_tables[page_dir_offset][offset] = TABLE_WRITEABLE | TABLE_PRESENT;
                            offset++;
                        }

                        return (void *)address;
                    }
                }
            } else {
                page_dir_offset = i + 1;
                offset = 0;
                counter = 0;
            }
        }

        page_dir_offset = 0;
        offset = 0;
        counter = 0;

        // First pass failed, allocate new tables
        for(int i = 0; i < TABLES_PER_DIR; i++) {
            if(!(kernel_heap_dir[i] & TABLE_PRESENT)) {
                counter += 512;
                if(counter >= amount) {
                    address = (PDPT_SIZE * pml4_index) + (pdpt_index * PAGE_SIZE_1G) + (page_dir_offset * PAGE_SIZE_2M) + (offset * PAGE_SIZE_4K);
                    address |= 0xFFFF000000000000;
                    set_page_frame(&(kernel_heap_dir[page_dir_offset]), ((uintptr_t)&(kernel_heap_dir_tables[page_dir_offset]) - KERNEL_VIRTUAL_BASE));
                    kernel_heap_dir[page_dir_offset] |= (TABLE_WRITEABLE | TABLE_PRESENT);
                    while(amount--) {
                        if(offset >= 512) {
                            page_dir_offset++;
                            offset = 0;
                            set_page_frame(&(kernel_heap_dir[page_dir_offset]), ((uintptr_t)&(kernel_heap_dir_tables[page_dir_offset]) - KERNEL_VIRTUAL_BASE));
                            kernel_heap_dir[page_dir_offset] |= (TABLE_WRITEABLE | TABLE_PRESENT);
                        }

                        kernel_heap_dir_tables[page_dir_offset][offset] = TABLE_WRITEABLE | TABLE_PRESENT;
                        offset++;
                    }

                    return (void *)address;
                } 
            } else {
                page_dir_offset = i + 1;
                offset = 0;
                counter = 0;
            }
        }

        assert(!"Kernel Out of Virtual Memory");
    }

    void kernel_free_4k_pages(void* addr, uint64_t amount) {
        uint64_t page_dir_index, page_index;
        uint64_t virt = (uint64_t)addr;
        while(amount--) {
            page_dir_index = PDE_GET_INDEX(virt);
            page_index = PT_GET_INDEX(virt);
            kernel_heap_dir_tables[page_dir_index][page_index] = 0;
            invlpg(virt);
            virt += PAGE_SIZE_4K;
        }
    }

    void kernel_map_virtual_memory_4k(uint64_t phys, uint64_t virt, uint64_t amount, uint64_t flags) {
        uint64_t page_dir_index, page_index;
        while(amount--) {
            page_dir_index = PDE_GET_INDEX(virt);
            page_index = PT_GET_INDEX(virt);
            kernel_heap_dir_tables[page_dir_index][page_index] = flags;
            set_page_frame(&(kernel_heap_dir_tables[page_dir_index][page_index]), phys);
            invlpg(virt);
            phys += PAGE_SIZE_4K;
            virt += PAGE_SIZE_4K;
        }
    }

    uint64_t virtual_to_physical_addr(uint64_t addr) {
        uint64_t address = 0;
        uint32_t pml4_index = PML4_GET_INDEX(addr);
        uint32_t page_dir_index = PDE_GET_INDEX(addr);
        uint32_t page_table_index = PT_GET_INDEX(addr);

        if(pml4_index < 511) {

        } else {
            if(kernel_heap_dir[page_dir_index] & PDE_2M) {
                address = (get_page_frame(kernel_heap_dir[page_dir_index])) << 12;
            } else {
                address = (get_page_frame(kernel_heap_dir_tables[page_dir_index][page_table_index])) << 12;
            }
        }

        return address;
    }

    bool check_kernel_pointer(uintptr_t addr, uint64_t len) {
        if(PML4_GET_INDEX(addr) != PML4_GET_INDEX(KERNEL_VIRTUAL_BASE)) {
            return false;
        }

        if(!(kernel_pdpt[PDPT_GET_INDEX(addr)] & TABLE_PRESENT)) {
            return false;
        }

        if(PDPT_GET_INDEX(addr) == KERNEL_HEAP_PDPT_INDEX) {
            if(!(kernel_heap_dir[PDE_GET_INDEX(addr)] & TABLE_PRESENT)) {
                return false;
            }

            if(!(kernel_heap_dir[PDE_GET_INDEX(addr)] & PDE_2M)) {
                if(!(kernel_heap_dir_tables[PDE_GET_INDEX(addr)][PT_GET_INDEX(addr)] & TABLE_PRESENT)) {
                    return false;
                }
            }
        } else if(PDPT_GET_INDEX(addr) == PDPT_GET_INDEX(KERNEL_VIRTUAL_BASE)) {
            if(!(kernel_dir[PDE_GET_INDEX(addr)] & TABLE_PRESENT)) {
                return false;
            }
        } else {
            return false;
        }

        return true;
    }
}

