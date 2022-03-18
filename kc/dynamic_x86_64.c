#include <kc.h>

void kc_dynamic_init(void *base, Elf64_Dyn *dyn)
{
    struct rela
    {
        Elf64_Rela *entries;
        size_t size;
        size_t entsize;
    };

    struct rela reladyn = {NULL, 0, 0};
    struct rela relaplt = {NULL, 0, 0};

    Elf64_Addr *gotplt = NULL;

    while (dyn && dyn->d_tag != DT_NULL)
    {
        switch(dyn->d_tag)
        {
            case DT_RELA:
                reladyn.entries = (Elf64_Rela *)(dyn->d_ptr + (uintptr_t)base);
                break;
            case DT_RELASZ:
                reladyn.size = dyn->d_val;
                break;
            // NOTE: ELF x86_64 only uses RELA
            // possibly support doing REL also for platforms that want it
            // (am i ever going to use other platforms?)
            case DT_JMPREL:
                relaplt.entries = (Elf64_Rela *)(dyn->d_ptr + (uintptr_t)base);
                break;
            case DT_PLTRELSZ:
                relaplt.size = dyn->d_val;
                break;
            case DT_RELAENT:
                reladyn.entsize = dyn->d_val;
                relaplt.entsize = dyn->d_val;
                break;
            case DT_PLTGOT:
                gotplt = (uintptr_t *)(dyn->d_ptr + (uintptr_t)base);
                break;
        }
        dyn++;
    }

    // peform relocations
    //
    if (gotplt)
    {
        gotplt[1] = (uintptr_t)NULL;
        gotplt[2] = (uintptr_t)kc_resolve_symbol;
    }
    kc_reloc(base, relaplt.entries, relaplt.size, relaplt.entsize);
    kc_reloc(base, reladyn.entries, reladyn.size, reladyn.entsize);
}

