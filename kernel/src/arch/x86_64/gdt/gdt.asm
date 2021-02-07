[bits 64]

GLOBAL __load_gdt

; This should probably be done earlier in the boot process
__load_gdt:
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