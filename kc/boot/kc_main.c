#include <loader/efi/shim.h>
#include <kernel/entry.h>
#include <kernel/memory/range.h>
#include <kernel/memory/paging.h>

#include <stdbool.h>

#include "../lib/elf.h"

struct efi_memory_map
{
    struct memory_range buffer;
    UINTN size;
    UINTN key;
    UINTN descsize;
    UINT32 descver;
};

struct efi_video_data
{
    struct memory_range buffer;
    UINT32 width;
    UINT32 height;
    UINT32 pitch;
    EFI_GRAPHICS_PIXEL_FORMAT format;
    EFI_PIXEL_BITMASK mask;
};

struct efi_boot_data
{
    struct efi_memory_map memory_map;
    struct efi_video_data video_data;
    void *acpi_rsdp;
    unsigned long acpi_version;
};

struct efi_loader_image kernel_image =
{
    .path = L"\\adasoft\\sophia\\kernel.os"
};

static struct efi_loader_interface *loader_interface;
static struct efi_boot_data boot_data;
static EFI_BOOT_SERVICES *e_bs = NULL;
static uint64_t *system_page_map = NULL;
static SIMPLE_TEXT_OUTPUT_INTERFACE *eto;

enum page_type
{
    INVALID_PAGE_TYPE = 0x0,
    CODE_PAGE_TYPE = PAGE_PR,
    RODATA_PAGE_TYPE = PAGE_PR|PAGE_NX,
    DATA_PAGE_TYPE = PAGE_PR|PAGE_WR|PAGE_NX
};

static inline void plog(CHAR16 *message)
{
    if (eto) eto->OutputString(eto, message);
}

static uint64_t *new_page_table(void)
{
    EFI_PHYSICAL_ADDRESS table;
    EFI_STATUS status;
    
    status = loader_interface->page_alloc(SystemMemoryType, page_size(1), &table);

    if (EFI_ERROR(status))
    {
        plog(L"can't allocate page table!");
        return NULL;
    }

    e_bs->SetMem((void *)table, page_size(1), 0);

    return (uint64_t *)table;
}

static uint64_t *get_page_map(void)
{
    uint64_t map;
    __asm__("mov %%cr3, %0" : "=r"(map));
    return (uint64_t *)(page_address(map, 1));
}

void set_page_map(uint64_t *map)
{
    __asm__("cli");
    __asm__("mov %0, %%cr3" :: "r"(map));
}

static uint64_t *get_page_table(uint64_t *map, uint64_t virt, int level)
{
    if ((level < 1) || (level > PAGE_MAP_LEVELS))
    {
        return NULL;
    }

    for (int i = PAGE_MAP_LEVELS; i > level; i--)
    {
        if (!map[pte_index(virt, i)])
        {
            uint64_t *next_map = new_page_table();
            map[pte_index(virt, i)] = (uint64_t)next_map|PAGE_PR|PAGE_WR;
            map = next_map;
        }
        else
        {
            map = (uint64_t *)page_address(map[pte_index(virt, i)], 1);
        }
    }

    return map;
}

static uint64_t *get_page_entry(uint64_t *map, uint64_t virt)
{
    int level = 1;
    uint64_t *table = get_page_table(map, virt, level);
    return &table[pte_index(virt, level)];
}

static void map_page(void *map,
                     uint64_t virt,
                     uint64_t phys,
                     enum page_type type)
{
    *get_page_entry(map, virt) = phys | type;
}

enum page_type get_page_type(Elf64_Phdr *phdr)
{
    switch (phdr->p_flags)
    {
        case (PF_R|PF_X):
            return CODE_PAGE_TYPE;
        case (PF_R):
            return RODATA_PAGE_TYPE;
        case (PF_R|PF_W):
            return DATA_PAGE_TYPE;
    }

    return INVALID_PAGE_TYPE;
}

static void map_pages(void *map,
                      uint64_t virt,
                      uint64_t phys,
                      enum page_type type,
                      size_t size)
{
    for (size_t offset = 0; offset < size; offset += page_size(1))
    {
        map_page(map, virt + offset, phys + offset, type);
    }
}

#define KERNEL_SPACE_LOWER 0xffffffff80000000
#define KERNEL_SPACE_UPPER 0xffffffffc0000000

static void create_kernel_maps(Elf64_Ehdr *ehdr, void *base)
{
    Elf64_Phdr *phdrs = (Elf64_Phdr *)(ehdr->e_phoff + (char *)ehdr);

    for (int i = 0; i < ehdr->e_phnum; i++)
    {
        if (phdrs[i].p_type == PT_LOAD)
        {
            uint64_t virt_begin = phdrs[i].p_offset + (uintptr_t)base;
            uint64_t phys_begin = phdrs[i].p_offset + (uintptr_t)ehdr;
            size_t size = phdrs[i].p_memsz;
            map_pages(system_page_map,
                      virt_begin,
                      phys_begin,
                      get_page_type(&phdrs[i]),
                      size);
        }
    }


    // Create fractal mappings
    uint64_t *lower_page_dir = get_page_table(system_page_map,
                                              KERNEL_SPACE_LOWER,
                                              2);
    uint64_t *upper_page_dir = get_page_table(system_page_map,
                                              KERNEL_SPACE_UPPER,
                                              2);

    upper_page_dir[PAGE_TABLE_INDEX_MASK] =
        (uint64_t)upper_page_dir|DATA_PAGE_TYPE;
    upper_page_dir[PAGE_TABLE_INDEX_MASK - 1] =
        (uint64_t)lower_page_dir|DATA_PAGE_TYPE;

}

static void collect_boot_data(void)
{
    EFI_STATUS status;

    boot_data.memory_map.size = 0;

    status = e_bs->GetMemoryMap(&boot_data.memory_map.size,
            NULL,
            &boot_data.memory_map.key,
            NULL,
            NULL);

    if (EFI_BUFFER_TOO_SMALL == status)
    {
        plog(L"getting memory map size\r\n");
        boot_data.memory_map.buffer.size = boot_data.memory_map.size += 
            EFI_PAGE_SIZE;
        status = loader_interface->page_alloc(SystemMemoryType,
                boot_data.memory_map.buffer.size,
                (EFI_PHYSICAL_ADDRESS *)&boot_data.memory_map.buffer.base);
    }

    if (EFI_ERROR(status))
    {
        plog(L"failed allocating pages for memory map\r\n");
    }

    if (!EFI_ERROR(status))
    {
        plog(L"got the memory map size\r\n");
        boot_data.memory_map.size = boot_data.memory_map.buffer.size;

        plog(L"getting memory map\r\n");
        status = e_bs->GetMemoryMap(&boot_data.memory_map.size,
                (EFI_MEMORY_DESCRIPTOR *)boot_data.memory_map.buffer.base,
                &boot_data.memory_map.key,
                &boot_data.memory_map.descsize,
                &boot_data.memory_map.descver);
    }

    if (EFI_ERROR(status))
    {
        plog(L"failed getting memory map\r\n");
        __asm__ volatile
        (
            "cli\n\t"
            "int3\n\t"
        );

        while (1)
        {
            __asm__ volatile ("hlt\n\t");
        }
    }

    plog(L"boot data collected successfully\r\n");
}

EFI_STATUS kc_main(struct efi_loader_interface *interface)
{
    // 1. load kernel and boot modules into memory
    // 2. generate page tables for kernel and boot modules
    // 2. gather boot data, store as type SystemMemoryType
    // 3. exit boot services
    // 5. ???????????

    EFI_STATUS status;

    if (!interface)
    {
        return EFI_INVALID_PARAMETER;
    }

    loader_interface = interface;
    e_bs = loader_interface->system_table->BootServices;
    eto = loader_interface->system_table->ConOut;

    if (!EFI_ERROR((status = interface->image_open(&kernel_image))) &&
            !EFI_ERROR((status = interface->image_alloc(&kernel_image))) &&
            !EFI_ERROR((status = interface->image_load(&kernel_image))))
    {
        plog(L"collecting boot data\r\n");
        collect_boot_data();

        plog(L"creating page tables\r\n");
        system_page_map = new_page_table();
        // parasitic map of uefi page tables is this ok???????
        uint64_t *efi_map = get_page_map();
        system_page_map[0] = efi_map[0];

        Elf64_Ehdr *ehdr = (Elf64_Ehdr *)kernel_image.buffer_base;

        plog(L"mapping kernel pages\r\n");
        create_kernel_maps(ehdr, (void *)KERNEL_SPACE_LOWER);


        plog(L"exiting boot services\r\n");
        e_bs->ExitBootServices(loader_interface->image_handle,
                boot_data.memory_map.key);

        set_page_map(system_page_map);

        kc_entry_func kernel_entry = (kc_entry_func)
            (KERNEL_SPACE_LOWER + ehdr->e_entry);
        kernel_entry(NULL);
    }

    __asm__ volatile (
            "cli\n\t"
            "hlt\n\t"
            );

    return status;
}

