#include "proc/syscall.h"
#include "arch/x86_64/interrupt/idt.h"
#include "arch/x86_64/interrupt/interrupt.h"
#include "arch/x86_64/gdt/gdt.h"


extern uintptr_t syscall_entry(uintptr_t, uintptr_t, uintptr_t, uintptr_t);

extern "C" {
    void __syscall_setup();
}

extern "C" void syscall_init() {
    __syscall_setup();
}

extern "C" void syscall_handle(regs_t* registers) {
    interrupt_status_t status = interrupt_get_state();
    interrupt_enable();

    registers->rax = syscall_entry(registers->rdi, registers->rsi, registers->rdx, registers->rcx);
    interrupt_set_state(status);
}