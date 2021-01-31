#include "Memory.h"
#include "paging/PageFrameAllocator.h"

uint64_t GetMemorySize(EFI_MEMORY_DESCRIPTOR* mMap, uint64_t mMapEntries, uint64_t mMapDescriptorSize) {
    static uint64_t memorySizeBytes = 0;
    if(memorySizeBytes > 0) {
        return memorySizeBytes;
    }

    for(int i = 0; i < mMapEntries; i++) {
        EFI_MEMORY_DESCRIPTOR* desc = (EFI_MEMORY_DESCRIPTOR *)((uint64_t)mMap + (i * mMapDescriptorSize));
        memorySizeBytes += desc->numPages * 4096;
    }

    return memorySizeBytes;
}

void memset(void* start, uint8_t value, uint64_t c) {
    uint8_t* current = (uint8_t *)start;
    for(uint64_t i = 0; i < c; i++) {
        *current++ = value;
    }
}

int memcmp(const void* aptr, const void* bptr, size_t n) {
    uint8_t* a = (uint8_t *)aptr;
    uint8_t* b = (uint8_t *)bptr;
    for(size_t i = 0; i < n; i++) {
        if(*a != *b) {
            return *a - *b;
        }

        a++;
        b++;
    }

    return 0;
}

void memcpy(void* dest, const void* src, size_t n) {
    if(!((uintptr_t)dest & (sizeof(intptr_t) - 1)) && !((uintptr_t)src & sizeof(intptr_t) - 1)) {
        uintptr_t* d = (uintptr_t *)dest;
        uintptr_t* s = (uintptr_t *)src;
        while(n >= sizeof(intptr_t)) {
            *d++ = *s++;
            n -= sizeof(intptr_t);
        }
    }

    uint8_t* d = (uint8_t *)dest;
    uint8_t* s = (uint8_t *)src;
    while(n--) {
        *d++ = *s++;
    }
}

size_t strnlen(const char* str, size_t max) {
    size_t ret = 0;
    while(*str++ && max--) {
        ret++;
    }

    return ret;
}