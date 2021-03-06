#pragma once

#include <kernel/memory/range.h>
#include <kernel/memory/paging.h>

#include <elf/elf64.h>

#include <efi.h>
#include <efilib.h>

#include <stdbool.h>
#include <stddef.h>
#include <stdnoreturn.h>

enum page_type
{
    INVALID_PAGE_TYPE = 0x0,
    CODE_PAGE_TYPE = PAGE_PR,
    RODATA_PAGE_TYPE = PAGE_PR|PAGE_NX,
    DATA_PAGE_TYPE = PAGE_PR|PAGE_WR|PAGE_NX
};

struct system_image
{
    struct memory_range buffer;
    EFI_FILE_PROTOCOL *file;
    Elf64_Ehdr ehdr;
    Elf64_Phdr *phdrs;
};

extern EFI_SYSTEM_TABLE *e_st;
extern EFI_BOOT_SERVICES *e_bs;
extern EFI_RUNTIME_SERVICES *e_rt;

extern EFI_HANDLE e_image_handle;
extern EFI_LOADED_IMAGE_PROTOCOL *e_loaded_image;
extern EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *e_system_partition;
extern EFI_GRAPHICS_OUTPUT_PROTOCOL *e_graphics_output;
extern EFI_STATUS e_last_error;

noreturn void efi_exit(EFI_STATUS status);

struct memory_range system_allocate(size_t size);

void *efi_allocate(size_t size);
void efi_free(void *buffer);

void paging_init(void);
void paging_enter(void);
void *paging_map_page(void *vaddr, phys_addr_t paddr, enum page_type type);
void *paging_map_pages(void *vaddr,
                       phys_addr_t paddr,
                       enum page_type type,
                       size_t size);
void *paging_map_range(void *vaddr,
                       struct memory_range *range,
                       enum page_type type);

struct system_image *system_image_open(CHAR16 *path);
bool system_image_load(struct system_image *image);
void system_image_map(struct system_image *image);

void loader_main(void);

