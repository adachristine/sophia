#include <kc.h>

#include <libc/string.h>
#include <lib/elf.h>

extern void _rtld_link(void);

struct elf64_relatab
{
	Elf64_Rela *entries;
	size_t size;
	size_t entsize;
};

struct elf64_symtab
{
	Elf64_Sym *entries;
	const char *strings;
	size_t strsize;
};

struct elf64_dso_info
{
	char *base;
	struct elf64_relatab jmprel;
	struct elf64_relatab reladyn;
	struct elf64_symtab dynsym;
	Elf64_Addr *pltgot;
};

static struct elf64_dso_info dso_info;

void kc_dynamic_init(char *base, Elf64_Dyn *dyn)
{
	dso_info = (struct elf64_dso_info)
	{
		base,
		{
			elf64_dt_ptr(base, dyn, DT_JMPREL),
			elf64_dt_val(dyn, DT_PLTRELSZ),
			elf64_dt_val(dyn, DT_PLTREL) == DT_RELA ? sizeof(Elf64_Rela) : -1 
		},
		{
			elf64_dt_ptr(base, dyn, DT_RELA),
			elf64_dt_val(dyn, DT_RELASZ),
			elf64_dt_val(dyn, DT_RELAENT)
		},
		{
			elf64_dt_ptr(base, dyn, DT_SYMTAB),
			elf64_dt_ptr(base, dyn, DT_STRTAB),
			elf64_dt_val(dyn, DT_STRSZ)
		},
		elf64_dt_ptr(base, dyn, DT_PLTGOT)
	};

	if (dso_info.pltgot != nullptr)
	{
		dso_info.pltgot[1] = (uintptr_t)&dso_info;
		dso_info.pltgot[2] = (uintptr_t)_rtld_link;
	}

	kc_dynamic_reloc(base, &dso_info.jmprel);
	kc_dynamic_reloc(base, &dso_info.reladyn);
}

void *kc_dynamic_link(struct elf64_dso_info *dso, int index)
{
	struct kjarna_interface *ftab = entry_params->interface;

	int symindex = ELF64_R_SYM(dso->jmprel.entries[index].r_info);
	int strindex = dso->dynsym.entries[symindex].st_name;
	const char *name = &dso->dynsym.strings[strindex];
	uintptr_t *fixup = (uintptr_t *)(dso->base + dso->jmprel.entries[index].r_offset);

	// TODO: generate a symbol table for kjarna interface
	// TODO: implement symbol lookup (maybe hashed too)
	if (strcmp(name, "open") == 0)
	{
		*fixup = (uintptr_t)*ftab->open;
	}
	else if (strcmp(name, "close") == 0)
	{
		*fixup = (uintptr_t)*ftab->close;
	}
	else if (strcmp(name, "read") == 0)
	{
		*fixup = (uintptr_t)*ftab->read;
	}
	else if (strcmp(name, "write") == 0)
	{
		*fixup = (uintptr_t)*ftab->write;
	}
	else if (strcmp(name, "lseek") == 0)
	{
		*fixup = (uintptr_t)*ftab->lseek;
	}
	else if (strcmp(name, "mmap") == 0)
	{
		*fixup = (uintptr_t)*ftab->mmap;
	}
	else if (strcmp(name, "munmap") == 0)
	{
		*fixup = (uintptr_t)*ftab->munmap;
	}
	else
	{
		return nullptr;
	}

	return fixup;
}

static void do_rela(char *base, Elf64_Rela *rela)
{
	uintptr_t *fixup = (uintptr_t *)(base + rela->r_offset);

    switch (ELF64_R_TYPE(rela->r_info))
    {
        case R_X86_64_JUMP_SLOT:
            *fixup += (uintptr_t)base;
            break;
        case R_X86_64_RELATIVE:
            *fixup = (uintptr_t)(rela->r_addend + base);
            break;
        default:
            break;
    }
}

void kc_dynamic_reloc(char *base, struct elf64_relatab *rela)
{
    // perform the actual relocation

    for (size_t i = 0; rela->entries && (i < rela->size); i += rela->entsize)
    {
        do_rela(base, &rela->entries[i]);
    }
}

