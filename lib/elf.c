#include "elf.h"

bool elf64_validate(Elf64_Ehdr *ehdr, unsigned type, unsigned machine)
{
    if (ehdr->e_ident[EI_MAG0] != ELFMAG0 ||
            ehdr->e_ident[EI_MAG1] != ELFMAG1 ||
            ehdr->e_ident[EI_MAG2] != ELFMAG2 ||
            ehdr->e_ident[EI_MAG3] != ELFMAG3 ||
            ehdr->e_ident[EI_CLASS] != ELFCLASS64 ||
            ehdr->e_ident[EI_DATA] != ELFDATA2LSB ||
            ehdr->e_ident[EI_VERSION] != EV_CURRENT ||
            ehdr->e_machine != machine ||
            ehdr->e_type != type)
    {
        return false;
    }

    return true;
}

size_t elf64_size(Elf64_Ehdr *ehdr, Elf64_Phdr *phdrs)
{
    size_t size = 0;

    for (int i = 0; i < ehdr->e_phnum; i++)
    {
        if (phdrs[i].p_type != PT_LOAD)
        {
            continue;
        }

        if (phdrs[i].p_paddr + phdrs[i].p_memsz > size)
        {
            size = phdrs[i].p_paddr + phdrs[i].p_memsz;
            if (phdrs[i].p_align > 1)
            {
                size = (size + phdrs[i].p_align - 1) &
                    ~(phdrs[i].p_align - 1);
            }
        }
    }

    return size;
}

void elf64_reloc(Elf64_Ehdr *ehdr)
{
    // locate the dynamic segment
    Elf64_Phdr *phdrs = (Elf64_Phdr *)(ehdr->e_phoff + (uintptr_t)ehdr);
    Elf64_Dyn *dyn = NULL;
    Elf64_Rela *rela = NULL;
    size_t rela_size;
    int rela_count;

    for (int i = 0; i < ehdr->e_phnum; i++)
    {
        if (phdrs[i].p_type != PT_DYNAMIC) continue;

        dyn = (Elf64_Dyn *)(phdrs[i].p_offset + (uintptr_t)ehdr);
    }

    // parse the dynamic segment
    while (dyn->d_tag != DT_NULL)
    {
        switch (dyn->d_tag)
        {
            case DT_RELA:
                rela = (Elf64_Rela *)(dyn->d_ptr + (uintptr_t)ehdr);
                break;
            case DT_RELASZ:
                rela_size = dyn->d_val;
                break;
            case DT_RELAENT:
                rela_count = rela_size / dyn->d_val;
                break;
            default:
                break;
        }
        dyn++;
    }

    // perform the actual relocations

    for (int i; i < rela_count; i++)
    {
        union
        {
            Elf_Byte word8;
            Elf64_Half word16;
            Elf64_Word word32;
            Elf64_Xword word64;
        }
        *reloc_ptr = NULL;

        switch (ELF64_R_TYPE(rela[i].r_info))
        {
            case R_AMD64_RELATIVE:
                reloc_ptr = (void *)(rela[i].r_offset + (uintptr_t)ehdr);
                reloc_ptr->word64 = (rela[i].r_addend + (uintptr_t)ehdr);
                break;
            default:
                break;
        }
    }
}

