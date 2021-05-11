#pragma once

#include <stdint.h>

typedef struct {
    uint32_t prev_tss __attribute__((aligned(16)));
    uint64_t rsp[3];
    uint64_t reserved0;
    uint64_t ist[7];
    uint64_t reserved1;
    uint16_t reserved2;
    uint16_t io_map;
} __attribute__((packed)) tss_t;

namespace tss {
    void initialize_tss(tss_t* tss, void* gdt);
}