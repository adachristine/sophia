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

	kc_reloc(dso_info.base, dso_info.reladyn.entries, dso_info.reladyn.size, dso_info.reladyn.entsize);
	kc_reloc(dso_info.base, dso_info.jmprel.entries, dso_info.jmprel.size, dso_info.jmprel.entsize);
}

void *kc_dynamic_link(struct elf64_dso_info *dso, int index)
{
	int symindex = ELF64_R_SYM(dso->jmprel.entries[index].r_info);
	int strindex = dso->dynsym.entries[symindex].st_name;
	const char *name = &dso->dynsym.strings[strindex];
	uintptr_t *fixup = (uintptr_t *)dso->base + dso->jmprel.entries[index].r_offset;

	if (strcmp(name, "write") == 0)
	{
		*fixup = (uintptr_t)*entry_params->interface->write;
		return fixup;
	}

	return nullptr;
}

