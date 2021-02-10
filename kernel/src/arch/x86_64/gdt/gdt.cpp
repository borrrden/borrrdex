#include "gdt.h"

static __attribute__((aligned(0x1000))) gdt_desc_t s_gdt_descriptors[GDT_MAX_DESCRIPTORS];
static gdt_t s_gdt;
static uint32_t s_gdt_index;

constexpr uint8_t BASE_DESC = GDT_DESC_PRESENT | GDT_DESC_READWRITE | GDT_DESC_CODEDATA;
constexpr uint8_t BASE_GRAN = GDT_GRAN_64BIT | GDT_GRAN_4K;

extern "C" void __load_gdt(gdt_t* gdt);

extern "C" void gdt_init() {
    s_gdt.limit = (sizeof(gdt_desc_t) * GDT_MAX_DESCRIPTORS) - 1;
    s_gdt.base = (uint64_t)&s_gdt_descriptors[0];

    // NULL first
    gdt_install_descriptor(0, 0, 0, 0);

    // KERNEL CODE
    gdt_install_descriptor(0, 0, BASE_DESC | GDT_DESC_EXECUTABLE, BASE_GRAN);

    // KERNEL DATA
    gdt_install_descriptor(0, 0, BASE_DESC, BASE_GRAN);

    // USER CODE
    gdt_install_descriptor(0, 0, BASE_DESC | GDT_DESC_EXECUTABLE | GDT_DESC_DPL, BASE_GRAN);

    // USER DATA
    gdt_install_descriptor(0, 0, BASE_DESC | GDT_DESC_DPL, BASE_GRAN);

    __load_gdt(&s_gdt);
}

extern "C" void gdt_install_descriptor(uint64_t base, uint64_t limit, uint8_t access, uint8_t granularity) {
    if(s_gdt_index >= GDT_MAX_DESCRIPTORS) {
        return;
    }

    s_gdt_descriptors[s_gdt_index].base_low = (uint16_t)(base & 0xFFFF);
    s_gdt_descriptors[s_gdt_index].base_mid = (uint8_t)((base >> 16) & 0xFF);
    s_gdt_descriptors[s_gdt_index].base_high = (uint8_t)((base >> 24) & 0xFF);
    s_gdt_descriptors[s_gdt_index].limit = (uint16_t)(limit & 0xFFFF);

    s_gdt_descriptors[s_gdt_index].flags = access;
    s_gdt_descriptors[s_gdt_index].granularity = (uint8_t)((limit >> 16) & 0x0F);
    s_gdt_descriptors[s_gdt_index].granularity |= granularity & 0xF0;

    s_gdt_index++;
}