#pragma once

#include "Paging.h"

class PageTableManager {
public:
    PageTableManager(PageTable* PML4Address);
    
    void MapMemory(void* virtualMemory, void* physicalMemory);

private:
    PageTable* _pml4;
};