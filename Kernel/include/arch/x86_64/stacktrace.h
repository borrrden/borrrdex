#pragma once

#include <logging.h>
#include <paging.h>
#include <mm/address_space.h>

inline void print_stack_trace(uint64_t _rbp) {
    uint64_t* rbp = (uint64_t *)_rbp;
    uint64_t rip = 0;
    while(rbp && memory::check_kernel_pointer((uintptr_t)rbp, 16)) {
        rip = *(rbp + 1);
        log::info("0x%llx", rip);
        rbp = (uint64_t *)(*rbp);
    }
}

inline void user_print_stack_trace(uint64_t _rbp, mm::address_space* addr_space) {
    uint64_t* rbp = (uint64_t *)_rbp;
    uint64_t rip = 0;
    while(rbp && memory::check_usermode_pointer((uintptr_t)rbp, 16, addr_space)) {
        rip = *(rbp + 1);
        log::info("0x%llx", rip);
        rbp = (uint64_t *)(*rbp);
    }
}