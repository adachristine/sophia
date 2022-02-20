#include <kc.h>

#include <elf/elf64.h>

#include <stddef.h>

#define STARTCODE __attribute__((section(".text.start")))

STARTCODE 
static void do_rela(Elf64_Rela *rela, void *base)
{
    union
    {
        Elf_Byte word8;
        Elf64_Half word16;
        Elf64_Word word32;
        Elf64_Xword word64;
    }
    *fixup;

    switch (ELF64_R_TYPE(rela->r_info))
    {
        case R_AMD64_RELATIVE:
            fixup = (void *)(rela->r_offset + (uintptr_t)base);
            fixup->word64 = (rela->r_addend + (uintptr_t)base);
            break;
        default:
            break;
    }
}

STARTCODE
void kc_reloc(Elf64_Dyn *dyn, void *base)
{
    struct
    {
        Elf64_Rela *entries;
        size_t *size;
        size_t *entsize;
    }
    rela = {NULL, NULL, NULL};

    // parse the dynamic segment
    while (dyn && dyn++->d_tag != DT_NULL)
    {
        switch (dyn->d_tag)
        {
            case DT_RELA:
                rela.entries = (Elf64_Rela *)(dyn->d_ptr + (uintptr_t)base);
                break;
            case DT_RELASZ:
                rela.size = &dyn->d_val;
                break;
            case DT_RELAENT:
                rela.entsize = &dyn->d_val;
                break;
            default:
                break;
        }
    }

    // perform the actual relocation

    for (size_t i = 0; rela.entries && (i < *rela.size / *rela.entsize); i++)
    {
        do_rela(&rela.entries[i], base);
    }
}

