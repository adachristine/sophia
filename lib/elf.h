#pragma once

#include <elf/elf64.h>

#include <stdbool.h>
#include <stddef.h>

#ifdef __x86_64__
# define elf_validate elf64_validate
# define elf_size elf64_size
# define elf_reloc elf64_reloc
#endif

bool elf64_validate(Elf64_Ehdr *ehdr, unsigned type, unsigned machine);
size_t elf64_size(Elf64_Ehdr *ehdr, Elf64_Phdr *phdr);
void elf64_reloc(Elf64_Ehdr *ehdr);

