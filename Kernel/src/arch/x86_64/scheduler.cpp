#include <scheduler.h>
#include <cpu.h>
#include <smp.h>
#include <elf.h>
#include <timer.h>
#include <thread.h>
#include <kstring.h>
#include <kassert.h>
#include <paging.h>
#include <physical_allocator.h>
#include <mm/address_space.h>
#include <fs/fs_node.h>
#include <apic.h>
#include <tss.h>
#include <logging.h>
#include <abi.h>
#include <debug.h>

extern "C" void idle_process();
void kernel_process();

extern "C" [[noreturn]] void task_switch(register_context* regs, uint64_t pml4);

using thread_state = threading::thread::thread_state;

constexpr uint64_t LINKER_BASE_ADDR = 0x7FC0000000;

namespace scheduler {
    lock_t scheduler_lock;
    bool scheduler_ready = false;

    list<process_t *>* processes;
    list<process_t *>* dead_processes;
    lock_t dead_process_lock = 0;

    pid_t next_pid = 1;

    void schedule(void*, register_context*);

    static void insert_new_thread(threading::thread* thread) {
        cpu* c = smp::get_cpu(0);
        for(unsigned i = 1; i < smp::get_proc_count(); i++) {
            cpu* next = smp::get_cpu(i);
            if(next->run_queue_count < c->run_queue_count) {
                c = next;
            }

            if(!c->run_queue_count) {
                break;
            }
        }

        asm("sti");
        {
            lock_t l(c->run_queue_lock);
            asm("cli");
            c->run_queue.push_back(thread);
            c->run_queue_count++;
        }
        asm("sti");
    }

    process_t* initialize_process() {
        process_t* proc = new process_t();

        proc->threads.add(new threading::thread());
        timer::get_system_uptime(&proc->creation_time);
        proc->parent = nullptr;
        proc->uid = 0;
        proc->euid = 0;
        proc->gid = 0;
        proc->egid = 0;
        proc->pid = next_pid++;

        auto* thread = proc->threads.get(0);
        memset(thread, 0, sizeof(threading::thread));
        thread->priority = 1;
        thread->time_slice_default = 1;
        thread->time_slice = thread->time_slice_default;
        thread->state = threading::thread::thread_state::running;
        thread->parent = proc;

        register_context* regs = &thread->registers;
        regs->rflags = 0x202;
        regs->cs = GDT_SELECTOR_KERNEL_CODE;
        regs->ss = GDT_SELECTOR_KERNEL_DATA;

        thread->fx_state = memory::kernel_allocate_4k_pages(1);
        memory::kernel_map_virtual_memory_4k(memory::allocate_physical_block(), (uintptr_t)thread->fx_state, 1);
        memset(thread->fx_state, 0, memory::PAGE_SIZE_4K);

        void* kernel_stack = memory::kernel_allocate_4k_pages(32);
        for(unsigned i = 0; i < 32; i++) {
            memory::kernel_map_virtual_memory_4k(memory::allocate_physical_block(), 
                reinterpret_cast<uintptr_t>(kernel_stack) + memory::PAGE_SIZE_4K * i, 1);
        }

        memset(kernel_stack, 0, memory::PAGE_SIZE_4K * 32);
        thread->kernel_stack = (void *)((uintptr_t)kernel_stack + memory::PAGE_SIZE_4K * 32);



        auto* fx_state = (fx_state_t *)thread->fx_state;
        asm volatile("fxsave64 (%0)" :: "r"((uintptr_t)fx_state) : "memory");
        if(fx_state->mxcsr_mask == 0) {
            fx_state->mxcsr_mask = 0xffbf; // Default, unless already set, According to SDM Vol. 1 11.6.6
        }

        fx_state->mxcsr = 0x1f80;   // Default - SDM Vol. 1 Table 11-2
        fx_state->fcw = 0x37f;      // Default - SDM Vol. 1 8.1.5

        strncpy(proc->working_dir, "/", 2);
        strncpy(proc->name, "unknown", 8);
        
        return proc;
    }

    uintptr_t load_elf(process_t* proc, uintptr_t* stack_pointer, void* elf, int argc, char** argv, int envc, char** envp, const char* exec_path) {
        elf_info_t elf_info = elf::load_elf_segments(proc, elf, 0);
        uintptr_t rip = elf_info.entry;
        if(elf_info.linker_path) {
            uintptr_t linker_base_addr = LINKER_BASE_ADDR;
            fs::fs_node* node = fs::resolve_path("/lib/ld.so");
            assert(node);

            void* linker_elf = malloc(node->size);
            fs::read(node, 0, node->size, linker_elf);
            if(!elf::verify(linker_elf)) {
                log::warning("Invalid dynamic linker!");
                asm volatile("mov %%rax, %%cr3" :: "a"(get_current_process()->get_page_map()->pml4_phys));
                asm("sti");
                return 0;
            }

            elf_info_t linker_elf_info = elf::load_elf_segments(proc, linker_elf, linker_base_addr);
            rip = linker_elf_info.entry;
            free(linker_elf);
        }

        char* temp_argv[argc];
        char* temp_envp[envc];

        asm("cli");
        asm volatile("mov %%rax, %%cr3" :: "a"(proc->get_page_map()->pml4_phys));

        // ELF ABI Spec
        uint64_t* stack = (uint64_t *)(*stack_pointer);
        char* stack_str = (char *)stack;
        for(int i = 0; i < argc; i++) {
            size_t len = strnlen(argv[i], fs::NAME_MAX) + 1;
            stack_str -= len;
            temp_argv[i] = stack_str;
            strncpy(stack_str, argv[i], len);
        }

        if(envp) {
            for(int i = 0; i < envc; i++) {
                size_t len = strnlen(envp[i], fs::NAME_MAX) + 1;
                stack_str -= len;
                temp_envp[i] = stack_str;
                strncpy(stack_str, envp[i], len);
            }
        }

        char* exec_path_val = nullptr;
        if(exec_path) {
            size_t len = strnlen(exec_path, fs::NAME_MAX) + 1;
            stack_str -= len;
            strncpy(stack_str, exec_path_val, len);
            exec_path_val = stack_str;
        }

        stack_str -= (uintptr_t)stack_str & 0xF; // Align
        stack = (uint64_t *)stack_str;
        stack -= ((argc + envc) % 2); // If the sum of these is odd, stack will be misaligned
        stack--;
        *stack = 0; // AT_NULL

        stack -= sizeof(auxv_t) / sizeof(uint64_t);
        *((auxv_t *)stack) = { .a_type = AT_PHDR, .a_val = elf_info.phdr_segment };

        stack -= sizeof(auxv_t) / sizeof(uint64_t);
        *((auxv_t *)stack) = { .a_type = AT_PHENT, .a_val = elf_info.phdr_entry_size };

        stack -= sizeof(auxv_t) / sizeof(uint64_t);
        *((auxv_t *)stack) = { .a_type = AT_PHNUM, .a_val = elf_info.phdr_num };

        stack -= sizeof(auxv_t) / sizeof(uint64_t);
        *((auxv_t *)stack) = { .a_type = AT_ENTRY, .a_val = elf_info.entry };

        if(exec_path && exec_path_val) {
            stack -= sizeof(auxv_t) / sizeof(uint64_t);
            *((auxv_t *)stack) = { .a_type = AT_EXECPATH, .a_val = (uint64_t)exec_path_val };
        }

        stack--;
        *stack = 0;

        stack -= envc;
        for(int i = 0; i < envc; i++) {
            *(stack + i) = (uint64_t)temp_envp[i];
        }

        stack--;
        *stack = 0;

        stack -= argc;
        for(int i = 0; i < argc; i++) {
            *(stack + i) = (uint64_t)temp_argv[i];
        }

        stack--;
        *stack = argc;

        asm volatile("mov %%rax, %%cr3" :: "a"(get_current_process()->get_page_map()->pml4_phys));
        asm("sti");

        *stack_pointer = (uintptr_t)stack;
        return rip;
    }

    process_t* create_process(void* entry, bool no_queue = false) {
        process_t* proc = initialize_process();
        proc->address_space = new mm::address_space(memory::create_page_map());

        threading::thread* thread = proc->threads.get(0);
        void* stack = (void *)memory::kernel_allocate_4k_pages(32);
        for(int i = 0; i < 32; i++) {
            memory::kernel_map_virtual_memory_4k(memory::allocate_physical_block(), (uintptr_t)stack + i * memory::PAGE_SIZE_4K, 1);
        }

        memset(stack, 0, memory::PAGE_SIZE_4K * 32);

        thread->stack = stack;
        thread->registers.rsp = (uintptr_t)thread->stack + memory::PAGE_SIZE_4K * 32;
        thread->registers.rbp = thread->registers.rsp;
        thread->registers.rip = (uintptr_t)entry;

        if(!__builtin_expect(no_queue, 0)) {
            insert_new_thread(proc->threads.get(0));
        }

        processes->add(proc);
        return proc;
    }

    process_t* create_elf_process(void* elf, int argc, char** argv, int envc, char** envp, const char* exec_path) {
        if(!elf::verify(elf)) {
            return nullptr;
        }

        process_t* proc = initialize_process();
        proc->address_space = new mm::address_space(memory::create_page_map());

        threading::thread* thread = proc->threads.get(0);
        thread->registers.cs = GDT_SELECTOR_USER_CODE | 0x3;
        thread->registers.ss = GDT_SELECTOR_USER_DATA | 0x3;
        thread->time_slice_default = threading::DEFAULT_TIMESLICE;
        thread->time_slice = thread->time_slice_default;
        thread->priority = 4;

        mm::mapped_region* stack_region = proc->address_space->allocate_anonymous_vmo(0x200000, 0, false); // 2 MB max stack size
        thread->stack = (void *)stack_region->base();
        thread->registers.rsp = stack_region->base() + 0x200000;
        thread->registers.rbp = thread->registers.rsp;

        // Pre-allocate 8 KiB
        stack_region->vm_object()->hit(stack_region->base(), 0x200000 - 0x1000, proc->get_page_map());
        stack_region->vm_object()->hit(stack_region->base(), 0x200000 - 0x2000, proc->get_page_map());

        thread->registers.rip = load_elf(proc, &thread->registers.rsp, elf, argc, argv, envc, envp, exec_path);
        if(!thread->registers.rip) {
            delete proc->address_space;
            delete proc;
            return nullptr;
        }

        thread->registers.rbp = thread->registers.rsp;
        assert(!(thread->registers.rsp & 0xF));

        fs::fs_node* null_dev = fs::resolve_path("/dev/null");
        fs::fs_node* log_dev = fs::resolve_path("/dev/klog");

        if(null_dev) {
            proc->allocate_file_desc(fs::open(null_dev, 0)); // stdin
        } else {
            proc->allocate_file_desc(nullptr);
            log::warning("Failed to find /dev/null");
        }

        if(log_dev) {
            proc->allocate_file_desc(fs::open(log_dev, 0)); // stdout
            proc->allocate_file_desc(fs::open(log_dev, 0)); // stderr
        } else {
            proc->allocate_file_desc(nullptr);
            proc->allocate_file_desc(nullptr);

            log::warning("Failed to find /dev/klog");
        }

        processes->add(proc);
        return proc;
    }

    void initialize() {
        processes = new list<process_t *>();
        dead_processes = new list<process_t *>();

        cpu* c = get_cpu_local();

        for(unsigned i = 0; i < smp::get_proc_count(); i++) {
            process_t* idle_proc = create_process((void *)idle_process, true);
            strncpy(idle_proc->name, "IdleProcess", 12);
            idle_proc->threads.get(0)->time_slice_default = 0;
            idle_proc->threads.get(0)->time_slice = 0;
            smp::get_cpu(i)->idle_process = idle_proc;

            // Clear the idle process out of the queue
            smp::get_cpu(i)->run_queue.clear();
            smp::get_cpu(i)->run_queue_count = 0;
        }

        idt::register_interrupt_handler(IPI_SCHEDULE, schedule);
        auto kproc = create_process((void *)kernel_process);
        strncpy(kproc->name, "Kernel", 7);
        c->current_thread = nullptr;
        scheduler_ready = true;
        asm("sti");
        while(true) {
            asm("hlt");
        }
    }

    void tick(register_context* regs) {
        if(!scheduler_ready) {
            return;
        }

        apic::local::send_ipi(smp::get_cpu(0)->id, apic::ICR_DSH_OTHER, apic::ICR_MESSAGE_TYPE_FIXED, IPI_SCHEDULE);
        schedule(nullptr, regs);
    }

    void schedule(__attribute__((unused)) void*, register_context* regs) {
        cpu* c = get_cpu_local();
        if(c->current_thread) {
            c->current_thread->parent->active_ticks++;
            if(c->current_thread->time_slice > 0) {
                c->current_thread->time_slice--;
                return;
            }
        }

        if(!__builtin_expect(acquire_test_lock(&c->run_queue_lock), true)) {
            return;
        }

        if(__builtin_expect(c->run_queue_count <= 0 || !c->current_thread, 0)) {
            c->current_thread = c->idle_process->threads.get(0);
        } else {
            if(__builtin_expect(c->current_thread->state == thread_state::dying, 0)) {
                c->run_queue.erase(c->run_queue.iterator_to(c->current_thread));
                c->run_queue_count--;
                c->current_thread = c->idle_process->threads[0];
            } else if(__builtin_expect(c->current_thread->parent != c->idle_process, 1)) {
                asm volatile("fxsave64 (%0)" :: "r"((uintptr_t)c->current_thread->fx_state) : "memory");
                c->current_thread->registers = *regs;
                c->current_thread->time_slice = c->current_thread->time_slice_default;
                c->current_thread = c->current_thread->hook.next;
                if(!c->current_thread) {
                    c->current_thread = c->run_queue.front();
                }
            } else {
                c->current_thread = c->run_queue.front();
            }

            if(c->current_thread->state == thread_state::blocked) {
                threading::thread* first = c->current_thread;
                do {
                    c->current_thread = c->current_thread->hook.next;
                } while(c->current_thread && c->current_thread->state != thread_state::blocked);

                if(!c->current_thread) {
                    c->current_thread = c->idle_process->threads.get(0);
                }
            }
        }

        release_lock(&c->run_queue_lock);
        asm volatile("fxrstor64 (%0)" :: "r"((uintptr_t)c->current_thread->fx_state) : "memory");
        asm volatile("wrmsr" :: "a"(c->current_thread->fs_base & 0xFFFFFFFF), "d"(c->current_thread->fs_base >> 32), "c"(IA32_FS_BASE));
        tss::set_kernel_stack(&c->tss, (uintptr_t)c->current_thread->kernel_stack);
        task_switch(&c->current_thread->registers, c->current_thread->parent->get_page_map()->pml4_phys);
    }

    void start_process(process_t* proc) {
        insert_new_thread(proc->threads.get(0));
    }

    static void remove_threads(process_t* proc, cpu* c) {
        auto current = c->run_queue.begin();
        auto to_remove =  c->run_queue.end();
        while(current !=  c->run_queue.end()) {
            if(to_remove !=  c->run_queue.end()) {
                c->run_queue.erase(to_remove);
                c->run_queue_count--;
                delete *to_remove;
                to_remove = c->run_queue.end();
            }

            if(*current != c->current_thread && (*current)->parent == proc) {
                to_remove = current;
            }

            current++;
        }

        if(to_remove !=  c->run_queue.end()) {
            c->run_queue.erase(to_remove);
            c->run_queue_count--;
            delete *to_remove;
            to_remove = c->run_queue.end();
        }
    }

    void end_process(process_t* proc) {
        IF_DEBUG(debug_level_scheduler >= debug::LEVEL_VERBOSE, {
            log::info("Ending process: %s (%d)", proc->name, proc->pid);
        })

        cpu* c = get_cpu_local();
        list<threading::thread *> running_threads;
        for(auto thread : proc->threads) {
            if(thread != c->current_thread && thread) {
                thread->state = thread_state::zombie;
                if(!acquire_test_lock(&thread->lock)) {
                    // Unable to get lock, put the thread back
                    running_threads.add(thread);
                } else {
                    // Stop the thread from running further
                    thread->state = thread_state::blocked;
                    thread->time_slice = thread->time_slice_default = 0;
                }
            }
        }

        asm("sti");
        while(true) {
            // TODO: Convert to linked list
            bool still_alive = false;
            for(int i = 0; i < running_threads.size(); i++) {
                threading::thread* thread = running_threads[i];
                if(!thread) {
                    continue;
                }

                still_alive = true;
                if(acquire_test_lock(&thread->lock)) {
                    thread->state = thread_state::blocked;
                    thread->time_slice = thread->time_slice_default = 0;
                    running_threads.set(nullptr, i);
                }
            }

            if(!still_alive) {
                break;
            }

            // Sleep 50 ms so a deadlock does not ravage the CPU
            scheduler::get_current_thread()->sleep(50000);
        }

        IF_DEBUG(debug_level_scheduler >= debug::LEVEL_VERBOSE, {
            log::info("Removing threads from the run queue...");
        })

        acquire_lock(&c->run_queue_lock);
        asm("cli");

        remove_threads(proc, c);

        release_lock(&c->run_queue_lock);
        asm("sti");

        for(unsigned i = 0; i < smp::get_proc_count(); i++) {
            cpu* other = smp::get_cpu(i);
            if(other->id == c->id) {
                continue;
            }

            asm("sti");
            acquire_lock(&other->run_queue_lock);
            asm("cli");

            while(other->current_thread && other->current_thread->parent == proc) {
                // Spin for a few ms
            }

            remove_threads(proc, other);

            release_lock(&other->run_queue_lock);
            asm("sti");
            if(!other->current_thread) {
                apic::local::send_ipi(other->id, apic::ICR_DSH_SELF, apic::ICR_MESSAGE_TYPE_FIXED, IPI_SCHEDULE);
            }
        }

        IF_DEBUG(debug_level_scheduler >= debug::LEVEL_VERBOSE, {
            log::info("Closing fds...");
        })

        proc->destroy_all_files();

        IF_DEBUG(debug_level_scheduler >= debug::LEVEL_VERBOSE, {
            log::info("Removing process...");
        })

        for(int i = 0; i < processes->size(); i++) {
            if(processes->get(i)->pid == proc->pid) {
                processes->remove_at(i);
                break;
            }
        }

        dead_processes->add(proc);
        bool is_process_to_kill = c->current_thread->parent == proc;
        if(is_process_to_kill) {
            asm("cli");
            asm volatile("mov %%rax, %%cr3" :: "a"((uint64_t)memory::kernel_pml4 - memory::KERNEL_VIRTUAL_BASE));
            c->current_thread->state = thread_state::dying;
            c->current_thread->time_slice = 0;
            asm volatile("sti; int $0xFD"); // Send IPI_SCHEDULE to self via interrupt
            assert(!"Failed to exit process, overran interrupt");
        }
    }

    void yield() {
        cpu* c = get_cpu_local();
        if(c->current_thread) {
            c->current_thread->time_slice = 0;
        }

        asm("int $0xFD");
    }

    void gc() {
        kstd::lock l(dead_process_lock);
        for(int i = dead_processes->size() - 1; i >= 0; i--) {
            process_t* p = dead_processes->get(i);
            delete p->address_space;
            delete p;
            dead_processes->remove_at(i);
        }
    }
}