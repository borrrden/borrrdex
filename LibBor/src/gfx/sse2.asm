global memcpy_sse2
global memset32_sse2
global memset64_sse2
global memcpy_sse2_unaligned
global memcpy_optimized

section .text

; (void* dst, void* src, uint64_t count)
memcpy_optimized:
    mov rcx, rdx
    shr rcx, 2      ; Get number of 16 byte chunks

    jz .pad2        ; Bytes to transfer less than 16, go to end

    mov rcx, rdi
    and rcx, 0xF    ; Alignment check
    jz .smlcpy

    ; Align to 32-bit (4 - (count & 3))
    shr rcx, 2
    neg rcx
    add rcx, 4
    sub rdx, rcx

    ; Pad out destination to 16-bytes
.pad1:
    lodsd
    stosd
    loop .pad1

.smlcpy:
    mov rax, rsi
    and rax, 0xF
    jnz .smlcpyua

    mov rcx, rdx
    and rcx, ~0x3
    jz .pad2
    shl rcx, 2

    lea rsi, [rsi + rcx]
    lea rdi, [rdi + rcx]
    neg rcx

.smlcpyloop:
    prefetchnta [rsi + rcx + 16]
    movdqu xmm0, [rsi + rcx]
    movntdq [rdi + rcx], xmm0

    add rcx, 16
    jnz .smlcpyloop
    and rdx, 0x3

.pad2:
    mov rcx, rdx
    jz ._ret

.pad2l:
    lodsd
    stosd
    loop .pad2l

._ret:
    sfence
    ret

.smlcpyua:
    prefetchnta [rsi + rcx + 16]

    mov rcx, rdx
    and rcx, ~0x3
    jz .pad2u
    shl rcx, 2

    lea rsi, [rsi + rcx]
    lea rdi, [rdi + rcx]
    neg rcx

.smlcpyloopua:
    movdqu xmm0, [rsi + rcx]
    movntdq [rdi + rcx], xmm0

    add rcx, 16
    jnz .smlcpyloopua

    and rdx, 0x3

.pad2u:
    mov rcx, rdx
    jz .retua

.pad2lua:
    lodsd
    stosd
    loop .pad2lua

.retua:
    sfence
    ret

memcpy_sse2:
    xor rax, rax
    mov rcx, rdx
    test rcx, rcx
    jz .ret

.loop:
    movdqa xmm0, [rsi + rax]
    movdqa [rdi + rax], xmm0
    add rax, 16
    loop .loop

.ret:
    ret

memcpy_sse2_unaligned:
    xor rax, rax
    mov rcx, rdx
    test rcx, rcx
    jz .ret

.loop:
    movdqu xmm0, [rsi + rax]
    movdqu [rdi + rax], xmm0
    add rax, 16
    loop .loop

.ret:
    ret

memset32_sse2:
    push rbp
    mov rbp, rsp

    mov rcx, rdx
    cmp rcx, 1
    jle .ret

    movq xmm1, rsi
    pxor xmm0, xmm0

    paddq xmm0, xmm1
    pslldq xmm0, 4
    paddq xmm0, xmm1
    pslldq xmm0, 4
    paddq xmm0, xmm1
    pslldq xmm0, 4
    paddq xmm0, xmm1

.loop:
    movdqa [rdi], xmm0
    add rdi, 16
    loop .loop

.ret:
    mov rsp, rbp
    pop rbp
    ret

memset64_sse2:
    push rbp
    mov rbp, rsp

    mov rax, rcx
    mov rbx, rdx
    mov rcx, r8

    cmp rcx, 1
    jle .ret

    movq xmm1, rbx
    pxor xmm0, xmm0

    paddq xmm0, xmm1
    pslldq xmm0, 8
    paddq xmm0, xmm1

.loop:
    movdqa [rax], xmm0
    add rax, 16
    loop .loop

.ret:
    mov rsp, rbp
    pop rbp
    ret
