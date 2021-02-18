#pragma once

#ifndef __cplusplus
#error C++ Only
#endif

#include "paging/PageTableManager.h"
#include <cstdint>

class ThreadContext {
public:
    void initialize(uint64_t entry, uint64_t endentry, uint64_t stack, uint32_t args);

    void set_ip(uint64_t ip) { _rip = ip; }
    void set_sp(uint64_t sp) { _stack = (uint64_t *)sp; }

    void enter_userland();
    void enable_interrupts();

    void* get_prev_context() const { return _prevContext; }
    void set_prev_context(void* prev) { _prevContext = prev; }
private:
    uint64_t* _stack;
    uint64_t _rip;
    uint64_t _flags;
    PageTableManager* _pageTableManager;
    void* _prevContext;
};

void cswitch_vector_code();
void cswitch_to_userland(ThreadContext* usercontext);