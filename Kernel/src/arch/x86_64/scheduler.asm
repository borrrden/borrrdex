[BITS 64]

global task_switch

section .text

%macro popaq 0
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rbp
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    ; Dont pop rax yet
%endmacro

task_switch:
    mov rsp, rdi
    mov rax, rsi
    popaq

    mov cr3, rax

    pop rax
    iretq