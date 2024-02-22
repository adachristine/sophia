#pragma once

#include <elf/elf64.h>

#include <stdbool.h>
#include <stddef.h>

#ifdef __x86_64__
# define elf_validate elf64_validate
# define elf_size elf64_size
#endif

bool elf64_validate(Elf64_Ehdr *ehdr, unsigned type, unsigned machine);
size_t elf64_size(Elf64_Ehdr *ehdr, Elf64_Phdr *phdr);
void *elf64_dt_ptr(void *base, Elf64_Dyn *dyntab, unsigned long dt_type);
uintptr_t elf64_dt_val(Elf64_Dyn *dyntab, unsigned long dt_type);

