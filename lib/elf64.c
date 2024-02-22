#include <stdbool.h>
#include <lib/elf.h>

bool elf64_validate(Elf64_Ehdr *ehdr, unsigned type, unsigned machine)
{
    if (ehdr->e_ident[EI_MAG0] != ELFMAG0
			|| ehdr->e_ident[EI_MAG1] != ELFMAG1
			|| ehdr->e_ident[EI_MAG2] != ELFMAG2 
			|| ehdr->e_ident[EI_MAG3] != ELFMAG3
			|| ehdr->e_ident[EI_CLASS] != ELFCLASS64
			|| ehdr->e_ident[EI_DATA] != ELFDATA2LSB
			|| ehdr->e_ident[EI_VERSION] != EV_CURRENT
			|| ehdr->e_machine != machine
			|| ehdr->e_type != type)
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

        size = phdrs[i].p_offset + phdrs[i].p_memsz;
        if (phdrs[i].p_align > 1)
        {
            size = (size + phdrs[i].p_align - 1) & ~(phdrs[i].p_align - 1);
        }
    }

    return size;
}

static Elf64_Dyn *get_dynent(Elf64_Dyn *dyntab, unsigned long dt_type)
{
	while (dyntab && dyntab->d_tag != DT_NULL)
	{
		if (dyntab->d_tag == dt_type) 
		{
			return dyntab;
		}

		dyntab++;
	}

	return nullptr;
}

void *elf64_dt_ptr(void *base, Elf64_Dyn *dyntab, unsigned long dt_type)
{
	Elf64_Dyn *dynent = get_dynent(dyntab, dt_type);

	if (dynent == nullptr)
	{
		return nullptr;
	}

	return dynent->d_ptr + (char *)base;
}

uintptr_t elf64_dt_val(Elf64_Dyn *dyntab, unsigned long dt_type)
{
	Elf64_Dyn *dynent = get_dynent(dyntab, dt_type);

	if (dynent == nullptr)
	{
		return (uintptr_t)-1;
	}

	return dynent->d_val;
}

