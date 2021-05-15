BITS 64

global load_tss

load_tss: ;  RDI - address, RSI - GDT, RDX - selector
    push rbx

    ; Populate the TSS components (limit and addr)
    ; uint16_t limit_0
    ; uint16_t addr_0
    ; uint8_t addr_1
    ; uint8_t type_0
    ; uint8_t limit_1
    ; uint8_t addr_2
    ; uint32_t addr_3
    ; uint32_t reserved

    lea rbx, [rsi + rdx] ; limit_0
    mov word [rbx], 108

    mov eax, edi
    lea rbx, [rsi + rdx + 2] ; addr_0
    mov word [rbx], ax

    mov eax, edi
    shr eax, 16
    lea rbx, [rsi + rdx + 4] ; addr_1
    mov byte [rbx], al

    mov eax, edi
    shr eax, 24
    lea rbx, [rsi + rdx + 7] ; addr_2
    mov byte [rbx], al

    mov rax, rdi
    shr rax, 32
    lea rbx, [rsi + rdx + 8] ; addr_3
    mov dword [rbx], eax

    pop rbx
    ret