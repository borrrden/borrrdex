#include <scheduler.h>
#include <cpu.h>
#include <smp.h>
#include <elf.h>
#include <timer.h>
#include <thread.h>
#include <kstring.h>
#include <paging.h>
#include <physical_allocator.h>
#include <mm/address_space.h>
#include <apic.h>
#include <tss.h>

extern "C" void idle_process();
void kernel_process();

extern "C" [[noreturn]] void task_switch(register_context* regs, uint64_t pml4);

using thread_state = threading::thread::thread_state;

namespace scheduler {
    lock_t scheduler_lock;
    bool scheduler_ready = false;

    list<process_t *>* processes;
    list<process_t *>* dead_processes;

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
        }
        asm("sti");
    }

    process_t* initialize_process() {
        process_t* proc = new process_t();

        proc->file_descriptors_lock = 0;
        proc->file_descriptors.clear();
        proc->children.clear();
        proc->threads.clear();

        proc->threads.add(new threading::thread());
        timer::get_system_uptime(&proc->creation_time);
        proc->parent = nullptr;
        proc->uid = 0;
        proc->pid = next_pid++;

        auto* thread = proc->threads.get(0);
        thread->stack = 0;
        thread->state = threading::thread::thread_state::running;
        thread->parent = proc;

        register_context* regs = &thread->registers;
        memset((uint8_t *)regs, 0, sizeof(register_context));
        regs->rflags = 0x202;
        regs->cs = GDT_SELECTOR_KERNEL_CODE;
        regs->ss = GDT_SELECTOR_KERNEL_DATA;

        void* kernel_stack = memory::kernel_allocate_4k_pages(32);
        for(unsigned i = 0; i < 32; i++) {
            memory::kernel_map_virtual_memory_4k(memory::allocate_physical_block(), 
                reinterpret_cast<uintptr_t>(kernel_stack) + memory::PAGE_SIZE_4K * i, 1);
        }

        memset(kernel_stack, 0, memory::PAGE_SIZE_4K * 32);
        thread->kernel_stack = (void *)((uintptr_t)kernel_stack + memory::PAGE_SIZE_4K * 32);

        strncpy(proc->working_dir, "/", 2);
        strncpy(proc->name, "unknown", 8);
        
        return proc;
    }

    process_t* create_process(void* entry) {
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

        insert_new_thread(proc->threads.get(0));

        processes->add(proc);
        return proc;
    }

    void initialize() {
        processes = new list<process_t *>();
        dead_processes = new list<process_t *>();

        cpu* c = get_cpu_local();

        for(unsigned i = 0; i < smp::get_proc_count(); i++) {
            process_t* idle_proc = create_process((void *)idle_process);
            strncpy(idle_proc->name, "IdleProcess", 11);
            smp::get_cpu(i)->idle_process = idle_proc;
        }

        idt::register_interrupt_handler(IPI_SCHEDULE, schedule);
        auto kproc = create_process((void *)kernel_process);
        strncpy(kproc->name, "Kernel", 6);
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
            return;
        }

        if(__builtin_expect(acquire_test_lock(&c->run_queue_lock), 0)) {
            return;
        }

        if(__builtin_expect(c->run_queue_count <= 0 || !c->current_thread, 0)) {
            c->current_thread = c->idle_process->threads.get(0);
        } else {
            if(__builtin_expect(c->current_thread->state == thread_state::dying, 0)) {
                for(auto* thread : c->run_queue) {
                    c->run_queue.erase(c->run_queue.iterator_to(thread));
                    delete thread;
                }
            } else if(__builtin_expect(c->current_thread->parent != c->idle_process, 1)) {
                asm volatile("fxsave64 (%0)" :: "r"((uintptr_t)c->current_thread->fx_state) : "memory");
                c->current_thread->registers = *regs;
                c->current_thread = c->current_thread->hook.next;
            } else {
                c->current_thread = c->run_queue.front();
            }

            if(c->current_thread->state == thread_state::blocked) {
                threading::thread* first = c->current_thread;
                do {
                    c->current_thread = c->current_thread->hook.next;
                } while(c->current_thread->state != thread_state::blocked && c->current_thread != first);

                if(c->current_thread->state == thread_state::blocked) {
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
}