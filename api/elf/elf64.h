#pragma once

#include "elf.h"

typedef uint64_t Elf64_Addr;
typedef uint64_t Elf64_Off;
typedef uint16_t Elf64_Section;
typedef uint16_t Elf64_Versym;
typedef uint16_t Elf64_Half;
typedef int32_t Elf64_Sword;
typedef uint32_t Elf64_Word;
typedef int64_t Elf64_Sxword;
typedef uint64_t Elf64_Xword;

typedef struct Elf64_Ehdr Elf64_Ehdr;
typedef struct Elf64_Phdr Elf64_Phdr;
typedef struct Elf64_Shdr Elf64_Shdr;
typedef struct Elf64_Sym Elf64_Sym;
typedef struct Elf64_Dyn Elf64_Dyn;

typedef struct Elf64_Rel Elf64_Rel;
typedef struct Elf64_Rela Elf64_Rela;

#define EM_X86_64 62

struct Elf64_Ehdr
{
    Elf_Byte e_ident[EI_NIDENT];
    Elf64_Half e_type;
    Elf64_Half e_machine;
    Elf64_Word e_version;
    Elf64_Addr e_entry;
    Elf64_Off e_phoff;
    Elf64_Off e_shoff;
    Elf64_Word e_flags;
    Elf64_Half e_ehsize;
    Elf64_Half e_phentsize;
    Elf64_Half e_phnum;
    Elf64_Half e_shentsize;
    Elf64_Half e_shnum;
    Elf64_Half e_shstrndx;
};

struct Elf64_Phdr
{
    Elf64_Word p_type;
    Elf64_Word p_flags;
    Elf64_Off p_offset;
    Elf64_Addr p_vaddr;
    Elf64_Addr p_paddr;
    Elf64_Xword p_filesz;
    Elf64_Xword p_memsz;
    Elf64_Xword p_align;
};

struct Elf64_Shdr
{
    Elf64_Word sh_name;
    Elf64_Word sh_type;
    Elf64_Xword sh_flags;
    Elf64_Addr sh_addr;
    Elf64_Off sh_offset;
    Elf64_Xword sh_size;
    Elf64_Word sh_link;
    Elf64_Word sh_info;
    Elf64_Off sh_addralign;
    Elf64_Xword sh_entsize;
};

struct Elf64_Sym
{
    Elf64_Word st_name;
    Elf_Byte st_info;
    Elf_Byte st_other;
    Elf64_Half st_shndx;
    Elf64_Addr st_value;
    Elf64_Xword st_size;
};

struct Elf64_Dyn
{
    Elf64_Xword d_tag;
    union
    {
        Elf64_Xword d_val;
        Elf64_Addr d_ptr;
    };
};

#define ELF64_R_SYM(info) ((info)>>32)
#define ELF64_R_TYPE(info) ((Elf64_Word)(info))
#define ELF64_R_INFO(sym, type) (((Elf64_Xword)(sym)<<32)+(Elf64_Xword)(type))

#define R_X86_64_JUMP_SLOT 7
#define R_X86_64_RELATIVE 8

struct Elf64_Rel
{
    Elf64_Addr r_offset;
    Elf64_Xword r_info;
};

struct Elf64_Rela
{
    Elf64_Addr r_offset;
    Elf64_Xword r_info;
    Elf64_Sxword r_addend;
};

