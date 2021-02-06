[bits 64]
LoadGDT:
    lgdt [rdi]

    ; Load KernelData
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    ; Load KernelCode
    pop rdi
    mov rax, 0x08
    push rax
    push rdi
    retfq

GLOBAL LoadGDT