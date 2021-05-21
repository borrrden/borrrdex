#include <paging.h>
#include <idt.h>
#include <kstring.h>
#include <panic.h>
#include <logging.h>
#include <kassert.h>
#include <stacktrace.h>
#include <scheduler.h>
#include <apic.h>
#include <physical_allocator.h>
#include <liballoc/liballoc.h>
#include <mm/address_space.h>

constexpr uint16_t KERNEL_HEAP_PDPT_INDEX = 511;
constexpr uint16_t KERNEL_HEAP_PML4_INDEX = 511;

static void page_fault_handler(void*, register_context* regs) {
    uint64_t fault_address;
    asm volatile("movq %%cr2, %0" : "=r"(fault_address));

    int error_code = idt::get_err_code();
    int present = !(error_code & 0x1);
    int read_only = error_code & 0x2;
    int us = error_code & 0x4;
    int reserved = error_code & 0x8;
    int id = error_code & 0x10;

    process_t* process = scheduler::get_current_process();

    // Only call this if we can't reactively map the page
    auto fault_dump = [&]() {
        log::error("page fault");
        log::info("Register Dump:\nrip:0x%016llx, rax: 0x%016llx, rbx: 0x%016llx, rcx: 0x%016llx, rdx: 0x%016llx, rsi: 0x%016llx, rdi: 0x%016llx, rsp: 0x%016llx, rbp: 0x%016llx",
                    regs->rip, regs->rax, regs->rbx, regs->rcx, regs->rdx, regs->rsi, regs->rdi, regs->rsp, regs->rbp);
        log::info("Fault address: 0x%016llx", fault_address);
        if(present) {
            log::info("Page not present");
        }

        if(read_only) {
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
    };

    if(process) {
        mm::address_space* addr_space = process->address_space;
        asm("sti");
        mm::mapped_region* fault_region = addr_space->address_to_region(fault_address);
        asm("cli");
        if(fault_region && fault_region->vm_object()) {
            kstd::ref_counted<mm::vm_object> vmo = fault_region->vm_object();
            if(vmo->is_copy_on_write() && read_only) {
                // Attempted to write to a read-only page, needs to be copied to a new vm object
                if(vmo->use_count() <= 1) {
                    vmo->set_copy_on_write(false);
                    vmo->map_allocated_blocks(fault_region->base(), addr_space->get_page_map());
                    vmo->hit(fault_region->base(), fault_address - fault_region->base(), addr_space->get_page_map());
                    fault_region->lock().release_read();
                    asm("sti");
                    return;
                }

                asm("sti");
                mm::vm_object* clone = vmo->clone();
                vmo->remove_use();
                fault_region->set_vm_object(clone);
                asm("cli");

                clone->map_allocated_blocks(fault_region->base(), addr_space->get_page_map());
                clone->hit(fault_region->base(), fault_address - fault_region->base(), addr_space->get_page_map());
                fault_region->lock().release_read();
                asm("sti");
                return;
            }

            // Attempt to map the page
            int status = fault_region->vm_object()->hit(fault_region->base(), fault_address - fault_region->base(), addr_space->get_page_map());
            fault_region->lock().release_read();
            if(status == 0) {
                // mapping successful
                asm("sti");
                return;
            }
        }
    }

    if(regs->ss & 0x3) {
        assert(process);

        log::info("Process %s (PID: %x) page fault.", process->name, process->pid);
        fault_dump();

        log::info("Stack trace:");
        user_print_stack_trace(regs->rbp, process->address_space);
        log::info("End stack trace.");

        scheduler::end_process(process);

        return;
    }

    asm("cli");
    fault_dump();

    //OMG, everyone STOPPPP
    apic::local::send_ipi(0, apic::ICR_DSH_OTHER, apic::ICR_MESSAGE_TYPE_FIXED, IPI_HALT);

    log::info("Stack trace:");
    print_stack_trace(regs->rbp);
    log::info("End stack trace.");

    char temp[19] = { '0', 'x' };
    itoa(regs->rip, temp + 2, 16);
    temp[18] = 0;

    char temp2[19] = { '0', 'x' };
    itoa(fault_address, temp2 + 2, 16);
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

    static page_table_t allocate_page_table() {
        void* virt = kernel_allocate_4k_pages(1);
        uint64_t phys = allocate_physical_block();
        kernel_map_virtual_memory_4k(phys, (uintptr_t)virt, 1);

        page_table_t table = { .phys = phys, .virt = (page_t *)virt };
        for(int i = 0; i < PAGES_PER_TABLE; i++) {
            ((page_t *)virt)[i] = 0;
        }

        return table;
    }

    static page_table_t create_page_table(uint16_t pdpt_index, uint16_t page_dir_index, page_map_t* map) {
        page_table_t table = allocate_page_table();

        set_page_frame(&(map->page_dirs[pdpt_index][page_dir_index]), table.phys);
        map->page_dirs[pdpt_index][page_dir_index] |= TABLE_PRESENT | TABLE_WRITEABLE | PAGE_USER;
        map->page_tables[pdpt_index][page_dir_index] = table.virt;

        return table;
    }

    page_map_t* create_page_map() {
        page_map_t* addr_space = (page_map_t *)malloc(sizeof(page_map_t));
        pdpt_entry_t* pdpt = (pdpt_entry_t *)kernel_allocate_4k_pages(1);
        uintptr_t pdpt_phys = allocate_physical_block();
        kernel_map_virtual_memory_4k(pdpt_phys, (uintptr_t)pdpt, 1);

        pd_entry_t** page_dirs = (pd_entry_t **)kernel_allocate_4k_pages(1);
        kernel_map_virtual_memory_4k(memory::allocate_physical_block(), (uintptr_t)page_dirs, 1);
        uint64_t* page_dirs_phys = (uint64_t *)kernel_allocate_4k_pages(1);
        kernel_map_virtual_memory_4k(memory::allocate_physical_block(), (uintptr_t)page_dirs_phys, 1);
        page_t*** page_tables = (page_t ***)kernel_allocate_4k_pages(1);
        kernel_map_virtual_memory_4k(memory::allocate_physical_block(), (uintptr_t)page_tables, 1);

        pml4_entry_t* pml4 = (pml4_entry_t *)kernel_allocate_4k_pages(1);
        uintptr_t pml4_phys = allocate_physical_block();
        kernel_map_virtual_memory_4k(pml4_phys, (uintptr_t)pml4, 1);
        memcpy(pml4, kernel_pml4, PAGE_SIZE_4K);

        for(int i = 0; i < 512; i++) {
            page_dirs[i] = (pd_entry_t *)kernel_allocate_4k_pages(1);
            page_dirs_phys[i] = allocate_physical_block();
            kernel_map_virtual_memory_4k(page_dirs_phys[i], (uintptr_t)page_dirs[i], 1);

            page_tables[i] = (page_t **)malloc(PAGE_SIZE_4K);
            set_page_frame(&(pdpt[i]), page_dirs_phys[i]);
            pdpt[i] |= TABLE_WRITEABLE | TABLE_PRESENT | PDPT_USER;

            memset(page_dirs[i], 0, PAGE_SIZE_4K);
            memset(page_tables[i], 0, PAGE_SIZE_4K);
        }

        addr_space->page_dirs = page_dirs;
        addr_space->page_dirs_phys = page_dirs_phys;
        addr_space->page_tables = page_tables;
        addr_space->pml4 = pml4;
        addr_space->pml4_phys = pml4_phys;
        addr_space->pdpt = pdpt;
        addr_space->pdpt_phys = pdpt_phys;

        pml4[0] = pdpt_phys | TABLE_PRESENT | TABLE_WRITEABLE | PAGE_USER;

        return addr_space;
    }

    void destroy_page_map(page_map_t* pm) {
        for(int i = 0; i < DIRS_PER_PDPT; i++) {
            if(!pm->page_dirs[i]) {
                continue;
            }

            if(pm->page_dirs_phys[i] < PHYS_BLOCK_SIZE) {
                continue;
            }

            for(int j = 0; j < TABLES_PER_DIR; j++) {
                pd_entry_t dir_ent = pm->page_dirs[i][j];
                if(dir_ent & TABLE_PRESENT) {
                    uint64_t phys = get_page_frame(dir_ent);
                    if(phys < PHYS_BLOCK_SIZE) {
                        continue;
                    }

                    free_physical_block(phys);
                    kernel_free_4k_pages(pm->page_tables[i][j], 1);
                }

                pm->page_dirs[i][j] = 0;
            }

            free(pm->page_tables[i]);

            pm->pdpt[i] = 0;
            kernel_free_4k_pages(pm->page_dirs[i], 1);
            free_physical_block(pm->page_dirs_phys[i]);
        }

        free_physical_block(pm->pdpt_phys);
    }

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

    void map_virtual_memory_4k(uint64_t phys, uint64_t virt, uint64_t amount, page_map_t* map, uint64_t flags) {
        uint64_t pml4_index, pdpt_index, page_dir_index, page_index;
        while(amount--) {
            pml4_index = PML4_GET_INDEX(virt);
            pdpt_index = PDPT_GET_INDEX(virt);
            page_dir_index = PDE_GET_INDEX(virt);
            page_index = PT_GET_INDEX(virt);

            if(pdpt_index > MAX_PDPT_INDEX || pml4_index) {
                const char* panic[1] = {"Process address space cannot be >512GB"};
                kernel_panic(panic, 1);
                __builtin_unreachable();
            }

            assert(map->page_dirs[pdpt_index]);
            if(!(map->page_dirs[pdpt_index][page_dir_index] & TABLE_PRESENT)) {
                create_page_table(pdpt_index, page_dir_index, map);
            }

            map->page_tables[pdpt_index][page_dir_index][page_index] = flags;
            set_page_frame(&(map->page_tables[pdpt_index][page_dir_index][page_index]), phys);
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

    uintptr_t get_io_mapping(uintptr_t addr) {
        // Typically most MMIO will not reside > 4GB, but check just in case
        if(addr > 0xFFFFFFFF) {
            log::error("MMIO >4GB not supported");
            return 0xFFFFFFFF;
        }

        return addr + IO_VIRTUAL_BASE;
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

    bool check_usermode_pointer(uintptr_t addr, uint64_t len, mm::address_space* addr_space) {
        return addr_space->range_in_region(addr, len);
    }
}

