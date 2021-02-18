#pragma once

#ifndef __cplusplus
#error C++ only
#endif

#include "thread.h"

class Scheduler {
public:
    static Scheduler* instance() { return &s_instance; }

    void add_ready(tid_t);
    void schedule();
private:
    Scheduler();
    static Scheduler s_instance;
};