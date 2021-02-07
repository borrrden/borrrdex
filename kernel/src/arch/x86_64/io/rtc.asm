[bits 64]
GLOBAL _rtc_init_interrupt
GLOBAL _rtc_set_interrupt_frequency

_rtc_init_interrupt:
    cli
    mov al, 0x8B
    out 0x70, al
    in al, 0x71
    push rax
    mov al, 0x8B
    out 0x70, al
    pop rax
    or al, 0x40
    out 0x71, al
    sti
    ret

_rtc_set_interrupt_frequency:
    cli
    mov al, 0x8A
    out 0x70, al
    in al, 0x71
    push rax
    mov al, 0x8A
    out 0x70, al
    pop rax
    and al, 0xF0
    and di, 0x0F
    or ax, di
    out 0x71, al
    sti
    ret
