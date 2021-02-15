#include "cswitch.h"
#include "gdt/gdt.h"
#include "arch/x86_64/irq.h"
#include "paging/PageTableManager.h"

constexpr uint32_t THREAD_FLAGS = 0x200202;

ThreadContext::ThreadContext(uint64_t entry, uint64_t endentry, uint64_t stack, uint32_t args) {
    if(stack & 0xF) {
        stack = (stack & 0xFFFFFFFFFFFFFFF0);
        stack += 0x10;
    }

    uint64_t* rsp = (uint64_t *)stack;
    uint64_t rbp = stack;

    *(rsp--) = endentry;
    *(rsp--) = GDT_SELECTOR_KERNEL_DATA;
    *(rsp--) = rbp;
    *(rsp--) = THREAD_FLAGS;
    *(rsp--) = GDT_SELECTOR_KERNEL_CODE;
    *(rsp--) = entry;

    rbp = (uint64_t)rsp;

    // Registers
    *(rsp--) = 0; // R15
    *(rsp--) = 0; // R14
    *(rsp--) = 0; // R13
    *(rsp--) = 0; // R12
    *(rsp--) = 0; // R11
    *(rsp--) = 0; // R10
    *(rsp--) = 0; // R9
    *(rsp--) = 0; // R8
    *(rsp--) = (uint64_t)args; // RDI
    *(rsp--) = 0; // RSI
    *(rsp--) = rbp; // RBP
    *(rsp--) = 0; // RSP
    *(rsp--) = 0; // RBX
    *(rsp--) = 0; // RDX
    *(rsp--) = 0; // RCX
    *(rsp--) = 0; // RAX

    _stack = rsp;
    _flags = 0;
}

void ThreadContext::enable_interrupts() {
    _flags |= EFLAGS_INTERRUPT_FLAG;


}