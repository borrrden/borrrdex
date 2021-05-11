#pragma once

#include <logging.h>
#include <paging.h>

inline void print_stack_trace(uint64_t _rbp) {
    uint64_t* rbp = (uint64_t *)_rbp;
    uint64_t rip = 0;
    while(rbp && memory::check_kernel_pointer((uintptr_t)rbp, 16)) {
        rip = *(rbp + 1);
        log::info("0x%llx", rip);
        rbp = (uint64_t *)(*rbp);
    }
}