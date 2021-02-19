[bits 64]

GLOBAL __load_tss

__load_tss:
    ltr di
    ret