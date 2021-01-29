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