#include "syscall.h"
#include "Panic.h"
#include "graphics/BasicRenderer.h"
#include <cstdint>

static int do_write(int filehandle, const uint8_t* buffer, int length) {
    if(filehandle == STDIN_HANDLE) {
        return -1;
    }

    if(filehandle == STDOUT_HANDLE || filehandle == STDERR_HANDLE) {
        // TODO: Need length based string printf
        for(int i = 0; i < length; i++) {
            GlobalRenderer->PutChar(buffer[i]);
        }
    }

    return 0;
}

uintptr_t syscall_entry(uintptr_t syscall, uintptr_t a0, uintptr_t a1, uintptr_t a2) {
    switch(syscall) {
        case SYSCALL_WRITE:
            return do_write((int)a0, (const uint8_t *)a1, (int)a2);
        case SYSCALL_EXIT:
            return 0;
        default:
            Panic("Unhandled system call");
            break;
    }

    __builtin_unreachable();
}