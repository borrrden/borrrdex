[bits 64]

GLOBAL _syscall

_syscall:
    ; We are about to clobber RCX, move it somewhere else
    mov r8, rcx

    syscall
    ret
