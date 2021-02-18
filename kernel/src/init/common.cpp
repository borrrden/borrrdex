#include "common.h"
#include "graphics/BasicRenderer.h"
#include "Panic.h"

void init_startup_thread(uint32_t arg) {
    arg = arg;

    GlobalRenderer->Printf("Starting initial program.\n");

    

    Panic("Should never return from initprog");
    __builtin_unreachable();
}