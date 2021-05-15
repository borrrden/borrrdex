#include <elf.h>
#include <kstring.h>
#include <logging.h>
#include <cpu.h>
#include <scheduler.h>
#include <mm/address_space.h>
#include <mm/vm_object.h>

namespace elf {
    bool verify(void* elf) {
        Elf64_Ehdr* elf_hdr = (Elf64_Ehdr *)elf;
        if(__builtin_bswap32(elf_hdr->e_ident.i) != MAGIC) {
            log::warning("Invalid ELF Header: 0x%x (%c%c%c%c)", elf_hdr->e_ident.i, elf_hdr->e_ident.c[0], 
                elf_hdr->e_ident.c[1], elf_hdr->e_ident.c[2], elf_hdr->e_ident.c[3]);
            return false;
        }

        return true;
    }

    elf_info_t load_elf_segments(process_t* proc, void* _elf, uintptr_t base) {
        uint8_t* elf = reinterpret_cast<uint8_t *>(_elf);
        cpu* c = get_cpu_local();

        elf_info_t elf_info{};
        if(!verify(elf)) {
            return elf_info;
        }

        Elf64_Ehdr* elf_hdr = (Elf64_Ehdr *)elf;
        elf_info.entry = base + elf_hdr->e_entry;
        elf_info.phdr_entry_size = elf_hdr->e_phentsize;
        elf_info.phdr_num = elf_hdr->e_phnum;

        for(uint16_t i = 0; i < elf_hdr->e_phnum; i++) {
            Elf64_Phdr* elf_phdr = (Elf64_Phdr *)(elf + elf_hdr->e_phoff + i * elf_hdr->e_phentsize);
            if(elf_phdr->p_memsz == 0 || elf_phdr->p_type != PT_LOAD) {
                continue;
            }

            uintptr_t used_mem_size = elf_phdr->p_memsz + (elf_phdr->p_vaddr & (memory::PAGE_SIZE_4K - 1)) + memory::PAGE_SIZE_4K - 1;
            uintptr_t page_base = (base + elf_phdr->p_vaddr) & memory::PAGE_SIZE_4K_MASK;
            proc->address_space->map_vmo(new mm::process_image_vm_object(page_base, (used_mem_size & memory::PAGE_SIZE_4K_MASK),
                true), page_base, true);
        }

        uintptr_t pml4_phys = scheduler::get_current_process()->get_page_map()->pml4_phys;
        for(int i = 0; i < elf_hdr->e_phnum; i++) {
            Elf64_Phdr* elf_phdr = (Elf64_Phdr *)(elf + elf_hdr->e_phoff + i * elf_hdr->e_phentsize);
            if(elf_phdr->p_type == PT_LOAD && elf_phdr->p_memsz > 0) {
                acquire_lock(&c->run_queue_lock);
                asm("cli");
                asm volatile("mov %%rax, %%cr3" :: "a"(proc->get_page_map()->pml4_phys));
                memset((void *)(base + elf_phdr->p_vaddr), 0, elf_phdr->p_memsz);
                memcpy((void *)(base + elf_phdr->p_vaddr), (void *)(elf + elf_phdr->p_offset), elf_phdr->p_filesz);
                asm volatile("mov %%rax, %%cr3" :: "a"(pml4_phys));
                asm("sti");
                release_lock(&c->run_queue_lock);
            } else if(elf_phdr->p_type == PT_PHDR) {
                elf_info.phdr_segment = base + elf_phdr->p_vaddr;
            } else if(elf_phdr->p_type == PT_INTERP) {
                char* link_path = (char *)malloc(elf_phdr->p_filesz + 1);
                strncpy(link_path, (const char *)elf + elf_phdr->p_offset, elf_phdr->p_filesz);
                link_path[elf_phdr->p_filesz] = 0;
                elf_info.linker_path = link_path;
            }
        }

        return elf_info;
    }
}