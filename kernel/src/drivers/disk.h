#pragma once

#ifndef __cplusplus
#error C++ only
#endif

#include "spinlock.h"
#include "gbd.h"

struct DiskRealDevice {
    Spinlock slock;
    volatile gbd_request_t* request_queue;
    volatile gbd_request_t* request_served;
};

int disk_init(void* base);