#pragma once

#include "Paging.h"

class Framebuffer;

class PageTableManager {
public:
    static void SetSystemMemorySize(uint64_t bytes);
    static void SetFramebuffer(Framebuffer* buffer);

    PageTableManager(PageTable* PML4Address);
    
    void MapMemory(void* virtualMemory, void* physicalMemory, bool forUser);
    void WriteToCR3();

    void InvalidatePage(uint64_t virtualAddress);

private:
    PageTable* _pml4;
};