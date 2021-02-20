#include "PageTableManager.h"
#include "PageMapIndexer.h"
#include "PageFrameAllocator.h"
#include "string.h"

#include <cstdint>

PageTableManager::PageTableManager(PageTable* PML4Address)
    :_pml4(PML4Address)
{

}

void PageTableManager::MapMemory(void* virtualMemory, void* physicalMemory, bool forUser) {
    PageMapIndexer indexer((uint64_t)virtualMemory);
    PageDirectoryEntry pde = _pml4->entries[indexer.GetPDP()];

    PageTable* pdp;
    if(!pde.GetFlag(PT_Flag::Present)) {
        pdp = (PageTable *)PageFrameAllocator::SharedAllocator()->RequestPage();
        memset(pdp, 0, 0x1000);
        pde.SetAddress((uint64_t)pdp >> 12);
        pde.SetFlag(PT_Flag::Present, true);
        pde.SetFlag(PT_Flag::ReadWrite, true);
        pde.SetFlag(PT_Flag::UserSuper, forUser);
        _pml4->entries[indexer.GetPDP()] = pde;
    } else {
        if(forUser) {
            pde.SetFlag(PT_Flag::UserSuper, true);
        }

        pdp = (PageTable *)((uint64_t)pde.GetAddress() << 12);
    }

    pde = pdp->entries[indexer.GetPD()];
    PageTable* pd;
    if(!pde.GetFlag(PT_Flag::Present)) {
        pd = (PageTable *)PageFrameAllocator::SharedAllocator()->RequestPage();
        memset(pd, 0, 0x1000);
        pde.SetAddress((uint64_t)pd >> 12);
        pde.SetFlag(PT_Flag::Present, true);
        pde.SetFlag(PT_Flag::ReadWrite, true);
        pde.SetFlag(PT_Flag::UserSuper, forUser);
        pdp->entries[indexer.GetPD()] = pde;
    } else {
        if(forUser) {
            pde.SetFlag(PT_Flag::UserSuper, true);
        }

        pd = (PageTable *)((uint64_t)pde.GetAddress() << 12);
    }

    pde = pd->entries[indexer.GetPT()];
    PageTable* pt;
    if(!pde.GetFlag(PT_Flag::Present)) {
        pt = (PageTable *)PageFrameAllocator::SharedAllocator()->RequestPage();
        memset(pt, 0, 0x1000);
        pde.SetAddress((uint64_t)pt >> 12);
        pde.SetFlag(PT_Flag::Present, true);
        pde.SetFlag(PT_Flag::ReadWrite, true);
        pde.SetFlag(PT_Flag::UserSuper, forUser);
        pd->entries[indexer.GetPT()] = pde;
    } else {
        if(forUser) {
            pde.SetFlag(PT_Flag::UserSuper, true);
        }

        pt = (PageTable *)((uint64_t)pde.GetAddress() << 12);
    }

    pde = pt->entries[indexer.GetP()];
    pde.SetAddress((uint64_t)physicalMemory >> 12);
    pde.SetFlag(PT_Flag::Present, true);
    pde.SetFlag(PT_Flag::ReadWrite, true);
    pde.SetFlag(PT_Flag::UserSuper, forUser);
    pt->entries[indexer.GetP()] = pde;
    

}