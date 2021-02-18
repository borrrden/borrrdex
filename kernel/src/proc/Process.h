#pragma once

#ifndef __cplusplus
#error C++ Only
#endif

#include "thread.h"

class Process {
public:
    Process(const char* executable, const char** argv);
    void start();
private:
};