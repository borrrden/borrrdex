BITS 64

global GDT64Pointer64
global entry

extern kinit_stivale2

KERNEL_VIRTUAL_BASE equ 0xFFFFFFFF80000000
KERNEL_BASE_PML4_INDEX equ (((KERNEL_VIRTUAL_BASE) >> 39) & 0x1FF)
KERNEL_BASE_PDPT_INDEX equ  (((KERNEL_VIRTUAL_BASE) >> 30) & 0x1FF)

section .boot.data
align 4096
kernel_pm4:
    times 512 dq 0

align 4096
kernel_pde:
    times 512 dq 0

align 4096
kernel_pdpt:
     times 512 dq 0

align 4096
kernel_pdpt2:
    times KERNEL_BASE_PDPT_INDEX dq 0
    dw 0

align 16
GDT64:
    .Null1: equ $ - GDT64        ; Null
    dw 0xFFFF
    dw 0
    db 0
    db 0
    db 0
    db 0
    .Code: equ $ - GDT64        ; Kernel Code
    dw 0
    dw 0
    db 0
    db 10011010b
    db 00100000b
    db 0
    .Data: equ $ - GDT64        ; Kernel Data
    dw 0
    dw 0
    db 0
    db 10010010b
    db 00000000b
    db 0
    .Null2: equ $ - GDT64        ; Null
    dw 0xFFFF
    dw 0
    db 0
    db 0
    db 0
    db 0
    .UserData: equ $ - GDT64    ; User Data
    dw 0
    dw 0
    db 0
    db 11110010b
    db 00000000b
    db 0
    .UserCode: equ $ - GDT64    ; User Code
    dw 0
    dw 0
    db 0
    db 11111010b
    db 00100000b
    db 0
    .TSS: 
    .len:
    dw 108                      ; x86_64 TSS is 108 bytes long
    .low:
    dw 0
    .mid:
    db 0
    db 10001001b
    db 00000000b
    .high:
    db 0
    .high32:
    dd 0
    dd 0                        ; Reserved
GDT64Pointer64:
    dw GDT64Pointer64 - GDT64 - 1
    dq GDT64 + KERNEL_VIRTUAL_BASE

entry:
    cli
    hlt

section .stivale2hdr
stivale2hdr:
    dq entryst2 ; entry_point
    dq 0        ; stack
    dq 0        ; flags
    dq st2fbtag ; tags

section .data
st2fbtag:
    dq 0x3ecc1bc43d0f7971   ; framebuffer tag
    dq 0                    ; next
    dw 0                    ; width [automatic]
    dw 0                    ; height [automatic]
    dw 0                    ; bpp [automatic]

extern _bss
extern _bss_end

section .text
entryst2:
    lgdt [GDT64Pointer64]

    mov rbx, rdi ; Save RDI bootloader info

    mov rdi, _bss
    mov rcx, _bss_end
    sub rcx, _bss
    xor rax, rax
    rep stosb

    mov rdi, rbx

    mov rsp, stack_top
    mov rbp, rsp

    push 0x10
    push rbp
    pushf
    push 0x8
    push .cont
    iretq

.cont:
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    mov rax, cr0
    and ax, 0xFFB   ; Clear coprocessor emulation
    or ax, 0x2      ; Set coprocessor monitoring
    mov cr0, rax

    ; Enable SSE
    mov rax, cr4
    or ax, 3 << 9
    mov cr4, rax

    ; PAT MSR
    mov rcx, 0x277
    rdmsr
    mov rbx, 0xFFFFFFFFFFFFFF
    and rax, rbx
    mov rbx, 0x100000000000000
    or rax, rbx     ; PA7 write combining
    wrmsr

    xor rbp, rbp
    call kinit_stivale2

    cli
    hlt

global __cxa_atexit
__cxa_atexit:
    ret

section .bss
align 64
stack_bottom:
resb 32768
stack_top:

global __dso_handle
__dso_handle: resq 1