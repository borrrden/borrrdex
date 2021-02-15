#include "PageFrameAllocator.h"
#include "../graphics/BasicRenderer.h"
#include "stalloc.h"

PageFrameAllocator sAllocator;
PageFrameAllocator* PageFrameAllocator::SharedAllocator() {
     return &sAllocator;
}

void PageFrameAllocator::ReadEFIMemoryMap(EFI_MEMORY_DESCRIPTOR* mMap, size_t mMapSize, size_t mMapDescSize) {
    if(_initialized) {
        return;
    }

    _initialized = true;
    uint64_t mMapEntries = mMapSize / mMapDescSize;
    uint64_t memorySize = GetMemorySize(mMap, mMapEntries, mMapDescSize);
    _freeMemory = memorySize;
    uint64_t bitmapSize = memorySize / 4096 / 8 + 1;
    void* bitmapAddr = stalloc(bitmapSize);
    _pageBitmap = Bitmap(bitmapSize, (uint8_t *)bitmapAddr);
    uint64_t last_address = (uint64_t)stalloc(1);
    stalloc_disable();

    LockPages(bitmapAddr, _pageBitmap.GetSize() / 4096 + 1);

    for(int i = 0; i < mMapEntries; i++) {
        EFI_MEMORY_DESCRIPTOR* desc = (EFI_MEMORY_DESCRIPTOR *)((uint64_t)mMap + i * mMapDescSize);
        if(desc->type != 7) { // type = EfiConventionalMemory
            ReservePages(desc->physAddr, desc->numPages);
        }
    }

}

void* PageFrameAllocator::RequestPage() {
    for(; _pageBitmapIndex < _pageBitmap.GetSize() * 8; _pageBitmapIndex++) {
        if(!_pageBitmap[_pageBitmapIndex]) {
            LockPage((void *)(_pageBitmapIndex * 4096));
            return (void *)(_pageBitmapIndex * 4096);
        }
    }

    return NULL; // Page Frame Swap
}

void PageFrameAllocator::FreePage(void* address) {
    uint64_t index = (uint64_t)address / 4096;
    if(!_pageBitmap[index]) {
        return;
    }

    if(_pageBitmap.Set(index, false)) {
        _freeMemory += 4096;
        _usedMemory -= 4096;
        if(_pageBitmapIndex > index) {
            _pageBitmapIndex = index;
        }
    }
}

void PageFrameAllocator::FreePages(void* address, uint64_t pageCount) {
    for(int t = 0; t < pageCount; t++) {
        FreePage((void *)((uint64_t)address + (t * 4096)));
    }
}

void PageFrameAllocator::LockPage(void* address) {
    uint64_t index = (uint64_t)address / 4096;
    if(_pageBitmap[index]) {
        return;
    }

    if(_pageBitmap.Set(index, true)) {
        _freeMemory -= 4096;
        _usedMemory += 4096;
    }
}

void PageFrameAllocator::LockPages(void* address, uint64_t pageCount) {
    for(int t = 0; t < pageCount; t++) {
        LockPage((void *)((uint64_t)address + (t * 4096)));
    }
}

void PageFrameAllocator::UnreservePage(void* address) {
    uint64_t index = (uint64_t)address / 4096;
    if(!_pageBitmap[index]) {
        return;
    }

    if(_pageBitmap.Set(index, false)) {
        _freeMemory += 4096;
        _reservedMemory -= 4096;
        if(_pageBitmapIndex > index) {
            _pageBitmapIndex = index;
        }
    }
}

void PageFrameAllocator::UnreservePages(void* address, uint64_t pageCount) {
    for(int t = 0; t < pageCount; t++) {
        UnreservePage((void *)((uint64_t)address + (t * 4096)));
    }
}

void PageFrameAllocator::ReservePage(void* address) {
    uint64_t index = (uint64_t)address / 4096;
    if(_pageBitmap[index]) {
        return;
    }

    if(_pageBitmap.Set(index, true)) {
        _freeMemory -= 4096;
        _reservedMemory += 4096;
    }
}

void PageFrameAllocator::ReservePages(void* address, uint64_t pageCount) {
    for(int t = 0; t < pageCount; t++) {
        ReservePage((void *)((uint64_t)address + (t * 4096)));
    }
}