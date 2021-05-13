#include <elf.h>
#include <kstring.h>
#include <logging.h>
#include <cpu.h>

namespace elf {
    bool verify(void* elf) {
        Elf64_Ehdr* elf_hdr = (Elf64_Ehdr *)elf;
        if(elf_hdr->e_ident.i != MAGIC) {
            log::warning("Invalid ELF Header: 0x%x (%c%c%c%c)", elf_hdr->e_ident.i, elf_hdr->e_ident.c[0], 
                elf_hdr->e_ident.c[1], elf_hdr->e_ident.c[2], elf_hdr->e_ident.c[3]);
            return false;
        }

        return true;
    }

    // elf_info_t load_elf_segments(process_t* proc, void* _elf, uintptr_t base) {
    //     uint8_t* elf = reinterpret_cast<uint8_t *>(_elf);
    //     cpu* cpu = get_cpu_local();

    //     elf_info_t elf_info{};
    //     if(!verify(elf)) {
    //         return elf_info;
    //     }

    //     Elf64_Ehdr* elf_hdr = (Elf64_Ehdr *)elf;
    //     elf_info.entry = elf_hdr->e_entry;
    //     elf_info.phdr_entry_size = elf_hdr->e_phentsize;
    //     elf_info.phdr_num = elf_hdr->e_phnum;

    //     for(uint16_t i = 0; i < elf_hdr->e_phnum; i++) {
    //         Elf64_Phdr* elf_phdr = (Elf64_Phdr *)(elf + elf_hdr->e_phoff + i * elf_hdr->e_phentsize);
    //         if(elf_phdr->p_memsz == 0 || elf_phdr->p_type != PT_LOAD) {
    //             continue;
    //         }

            
    //     }
    // }
}