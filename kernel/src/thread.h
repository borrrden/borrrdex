#pragma once

#ifndef __cplusplus
#error C++ Only
#endif

#include "cswitch.h"
#include <cstdint>

typedef int tid_t;

constexpr tid_t IDLE_THREAD_TID = 0;
constexpr uint8_t THREAD_FLAG_USERMODE = 0x1;
constexpr uint8_t THREAD_FLAG_ENTERUSER = 0x2;


typedef enum {
    THREAD_FREE,
    THREAD_RUNNING,
    THREAD_READY,
    THREAD_SLEEPING,
    THREAD_NONREADY,
    THREAD_DYING
} thread_state_t;

typedef struct {
    ThreadContext* context;
    ThreadContext* user_context;
    thread_state_t state;
    uint32_t sleeps_on;
    PageTable* pagetable;
    tid_t next;
    uint32_t attribs;
    uint32_t padding[5];
} thread_table_t;

void thread_table_init();