[bits 64]

%macro PUSHAQ 0
    mov [rsp-0x8],  r15
    mov [rsp-0x10], r14
    mov [rsp-0x18], r13
    mov [rsp-0x20], r12
    mov [rsp-0x28], r11
    mov [rsp-0x30], r10
    mov [rsp-0x38], r9
    mov [rsp-0x40], r8
    mov [rsp-0x48], rdi
    mov [rsp-0x50], rsi
    mov [rsp-0x58], rbp
    mov [rsp-0x60], rsp
    mov [rsp-0x68], rbx
    mov [rsp-0x70], rdx
    mov [rsp-0x78], rcx
    mov [rsp-0x80], rax
    sub rsp, 0x80
%endmacro

%macro POPAQ 0
    add rsp, 0x80
    mov r15, [rsp-0x8]
    mov r14, [rsp-0x10]
    mov r13, [rsp-0x18]
    mov r12, [rsp-0x20]
    mov r11, [rsp-0x28]
    mov r10, [rsp-0x30]
    mov r9,  [rsp-0x38]
    mov r8,  [rsp-0x40]
    mov rdi, [rsp-0x48]
    mov rsi, [rsp-0x50]
    mov rbp, [rsp-0x58]
    mov rsp, [rsp-0x60]
    mov rbx, [rsp-0x68]
    mov rdx, [rsp-0x70]
    mov rcx, [rsp-0x78]
    mov rax, [rsp-0x80]
%endmacro

GLOBAL syscall_irq_handler
extern syscall_handle

syscall_irq_handler:
    cli

    push qword 0
    push qword 0x80

    PUSHAQ

    mov rdi, rsp

    call syscall_handle

    POPAQ

    sti

    add rsp, 0x16
    sysretq

GLOBAL __syscall_setup
extern syscall_irq_handler

__syscall_setup:
    ; EFER MSR, enable SCE bit
    mov rcx, 0xc0000080
    rdmsr
    or eax, 1
    wrmsr

    ; STAR MSR, set code segment selectors
    ; User -> 0x18
    ; Kernel -> 0x08
    mov rcx, 0xc0000081
    rdmsr
    mov edx, 0x00180008
    wrmsr

    ; LSTAR MSR, set handler RIP
    mov rcx, 0xc0000082
    mov eax, $syscall_irq_handler
    wrmsr

    ret