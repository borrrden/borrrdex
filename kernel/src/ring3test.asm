[bits 64]

GLOBAL __enter_ring3

__enter_ring3:
    mov rsp, rdi
    mov rcx, rsi
    xor rdi, rdi
    xor rsi, rsi
    mov r11, 0x0202

    o64 sysret
