#include <kc.h>

#define STARTCODE __attribute__((section(".text.start")))

STARTCODE 
static void do_rela(void *base, Elf64_Rela *rela)
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
        case R_X86_64_JUMP_SLOT:
            fixup = (void *)(rela->r_offset + (uintptr_t)base);
            fixup->word64 += (uintptr_t)base;
            break;
        case R_X86_64_RELATIVE:
            fixup = (void *)(rela->r_offset + (uintptr_t)base);
            fixup->word64 = (rela->r_addend + (uintptr_t)base);
            break;
        default:
            break;
    }
}

STARTCODE
void kc_reloc(void *base, Elf64_Rela *entries, size_t size, size_t entsize)
{
    // perform the actual relocation

    for (size_t i = 0; entries && (i < size / entsize); i++)
    {
        do_rela(base, &entries[i]);
    }
}

