#include <kc.h>

void kc_dynamic_init(void *base, Elf64_Dyn *dyn)
{
    struct rela
    {
        Elf64_Rela *entries;
        size_t size;
        size_t entsize;
    };

    struct rela reladyn = {NULL,};
    struct rela relaplt = {NULL,};

    Elf64_Addr *gotplt;

    while (dyn && dyn->d_tag != DT_NULL)
    {
        switch(dyn->d_tag)
        {
            // check for basic relocations first
            case DT_RELA:
                reladyn.entries = (Elf64_Rela *)(dyn->d_ptr + (uintptr_t)base);
                break;
            case DT_RELASZ:
                reladyn.size = dyn->d_val;
                break;
            // might be combined with JMPRELs so check those too
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
    gotplt[1] = (uintptr_t)NULL;
    gotplt[2] = (uintptr_t)kc_resolve_symbol;
    kc_reloc(base, relaplt.entries, relaplt.size, relaplt.entsize);
    kc_reloc(base, reladyn.entries, reladyn.size, reladyn.entsize);
}

