#pragma once

/* SYSTEM */
#define SYSCALL_HALT    0x001

/* FILE I/O */
#define SYSCALL_READ    0x101
#define SYSCALL_WRITE   0x102

#define STDIN_HANDLE    0x0
#define STDOUT_HANDLE   0x1
#define STDERR_HANDLE   0x2