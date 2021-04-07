#include "loader_efi.h"

#include <loader/data.h>
#include <kernel/entry.h>
#include <kernel/memory/paging.h>

#include <elf/elf64.h>

#include <stdbool.h>
#include <stddef.h>

#define kernel_path L"\\adasoft\\sophia\\kernel.os"

struct system_buffer
{
    EFI_PHYSICAL_ADDRESS base;
    UINTN size;
};

struct system_image
{
    struct system_buffer buffer;
    EFI_FILE_PROTOCOL *file;
    Elf64_Ehdr ehdr;
    Elf64_Phdr *phdrs;
    uint64_t *maps;
};

static struct system_image *open_system_image(CHAR16 *path);
static bool load_system_image(struct system_image *image);
static struct efi_memory_map_data *get_memory_map_data(void);
static struct efi_framebuffer_data *get_framebuffer_data(void);
static void *get_acpi_rsdp(void);

enum page_type
{
    INVALID_PAGE_TYPE = 0x0,
    CODE_PAGE_TYPE = PAGE_PR,
    RODATA_PAGE_TYPE = PAGE_PR|PAGE_NX,
    DATA_PAGE_TYPE = PAGE_PR|PAGE_WR|PAGE_NX
};

static uint64_t *new_page_table(void)
{
    EFI_STATUS status;
    EFI_PHYSICAL_ADDRESS table_address;

    status = e_bs->AllocatePages(AllocateAnyPages,
                                 SystemMemoryType,
                                 1,
                                 &table_address);

    if (EFI_ERROR(status))
    {
        Print(L"fatal: failed allocating page table: %r", status);
        efi_exit(status);
    }

    e_bs->SetMem((void *)table_address, EFI_PAGE_SIZE, 0);

    return (uint64_t *)table_address;
}

static uint64_t *get_page_map(void)
{
    uint64_t map;
    __asm__("mov %%cr3, %0" : "=r"(map));
    return (uint64_t *)(map & PAGE_ADDRESS_MASK);
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
        if (!map[page_table_index(virt, i)])
        {
            uint64_t *next_map = new_page_table();
            map[page_table_index(virt, i)] =
                (uint64_t)next_map|PAGE_PR|PAGE_WR;
            map = next_map;
        }
        else
        {
            uint64_t next_map_entry = map[page_table_index(virt, i)];
            next_map_entry &= PAGE_ADDRESS_MASK;
            map = (uint64_t *)next_map_entry;
        }
    }

    return map;
}

static uint64_t *get_page_entry(uint64_t *map, uint64_t virt)
{
    int level = 1;
    uint64_t *table = get_page_table(map, virt, level);
    return &table[page_table_index(virt, level)];
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
    for (size_t offset = 0; offset < size; offset += EFI_PAGE_SIZE)
    {
        Print(L"mapping page %16.0lx to %16.0lx\r\n", virt + offset, phys + offset);
        map_page(map, virt + offset, phys + offset, type);
    }
}

#define KERNEL_SPACE_LOWER 0xffffffff80000000
#define KERNEL_SPACE_UPPER 0xffffffffc0000000

static void create_kernel_maps(struct system_image *image)
{
    Elf64_Ehdr *ehdr = &image->ehdr;
    Elf64_Phdr *phdrs = image->phdrs;

    for (int i = 0; i < ehdr->e_phnum; i++)
    {
        if (phdrs[i].p_type == PT_LOAD)
        {
            uint64_t virt_begin = phdrs[i].p_vaddr;
            uint64_t phys_begin = image->buffer.base + phdrs[i].p_paddr;
            size_t size = phdrs[i].p_memsz;
            Print(L"mapping segment %16.0lx to %16.0lx %d bytes\r\n",
                  virt_begin,
                  phys_begin,
                  size);
            map_pages(image->maps,
                      virt_begin,
                      phys_begin,
                      get_page_type(&phdrs[i]),
                      size);
        }
    }


    // Create fractal mappings
    uint64_t *lower_page_dir = get_page_table(image->maps,
                                              KERNEL_SPACE_LOWER,
                                              2);
    uint64_t *upper_page_dir = get_page_table(image->maps,
                                              KERNEL_SPACE_UPPER,
                                              2);

    upper_page_dir[PAGE_TABLE_INDEX_MASK] =
        (uint64_t)upper_page_dir|PAGE_PR|PAGE_WR;
    upper_page_dir[PAGE_TABLE_INDEX_MASK - 1] =
        (uint64_t)lower_page_dir|PAGE_PR|PAGE_WR;

}

static struct system_buffer get_system_buffer(UINTN size)
{
    EFI_STATUS status;
    struct system_buffer buffer;

    buffer.size = size;

    status = e_bs->AllocatePages(AllocateAnyPages,
                                 SystemMemoryType,
                                 EFI_SIZE_TO_PAGES(buffer.size),
                                 &buffer.base);

    if (EFI_ERROR(status))
    {
        Print(L"error allocating system buffer: %r\r\n", status);
        buffer.base = 0;
        buffer.size = 0;
    }

    Print(L"allocated system buffer at %16.0lx of %d bytes\r\n",
          buffer.base,
          buffer.size);

    return buffer;
}

#define KERNEL_ENTRY_STACK_SIZE 0x10000
#define KERNEL_ENTRY_STACK_HEAD 0xffffffffffc00000
#define KERNEL_ENTRY_STACK_BASE (KERNEL_ENTRY_STACK_HEAD - KERNEL_ENTRY_STACK_SIZE)

static void enter_kernel(struct system_image *kernel_image)
{
    struct efi_boot_data data;
    kernel_entry_func kernel_entry;
    kernel_entry = (kernel_entry_func)(kernel_image->ehdr.e_entry);


    struct system_buffer kernel_entry_stack =
        get_system_buffer(KERNEL_ENTRY_STACK_SIZE);

    // get a stack here for now idfk
    map_pages(kernel_image->maps,
              KERNEL_ENTRY_STACK_BASE,
              kernel_entry_stack.base,
              DATA_PAGE_TYPE,
              kernel_entry_stack.size);


    data.system_table = e_st;
    data.framebuffer = get_framebuffer_data();
    data.acpi_rsdp = get_acpi_rsdp();
    data.memory_map = get_memory_map_data();

    EFI_STATUS status = e_bs->ExitBootServices(e_image_handle,
                                               data.memory_map->key);

    if (EFI_ERROR(status))
    {
        Print(L"failed exiting boot services environment: %r\r\n", status);
        efi_exit(status);
    }

    // parasitic map of uefi page tables is this ok???????
    uint64_t *efi_map = get_page_map();
    kernel_image->maps[0] = efi_map[0];

    set_page_map(kernel_image->maps);

    __asm__
    (
        "mov %0, %%rsp"
        :
        : "r"((uint64_t)KERNEL_ENTRY_STACK_HEAD)
    );

    kernel_entry(&data);
}

void loader_main(void)
{
    struct system_image *kernel_image = open_system_image(kernel_path);

    Print(L"loading %s\r\n", kernel_path);

    if (load_system_image(kernel_image))
    {
        Print(L"loaded kernel image at base %16.0lx size %d bytes\r\n",
              kernel_image->buffer.base,
              kernel_image->buffer.size);
    }
    else
    {
        Print(L"failed loading kernel image\r\n");
        efi_exit(EFI_ABORTED);
    }
    
    kernel_image->maps = new_page_table();
    Print(L"kernel pml4 %16.0lx\r\n", kernel_image->maps);
    create_kernel_maps(kernel_image);
    Print(L"kernel maps created. entering kernel\r\n");
    enter_kernel(kernel_image);

    efi_exit(EFI_ABORTED);
}

static bool validate_image(Elf64_Ehdr *ehdr)
{
    if (ehdr->e_ident[EI_MAG0] != ELFMAG0 ||
        ehdr->e_ident[EI_MAG1] != ELFMAG1 ||
        ehdr->e_ident[EI_MAG2] != ELFMAG2 ||
        ehdr->e_ident[EI_MAG3] != ELFMAG3 ||
        ehdr->e_ident[EI_CLASS] != ELFCLASS64 ||
        ehdr->e_ident[EI_DATA] != ELFDATA2LSB ||
        ehdr->e_ident[EI_VERSION] != EV_CURRENT ||
        ehdr->e_machine != EM_X86_64)
    {
        Print(L"invalid image format\r\n");
        return false;
    }

    Print(L"elf x64 image detected\r\n");

    return true;
}

static struct system_image *open_system_image(CHAR16 *path)
{
    EFI_STATUS status;
    EFI_FILE_PROTOCOL *root;
    EFI_FILE_PROTOCOL *file;
    struct system_image *image = NULL;

    status = e_system_partition->OpenVolume(e_system_partition, &root);

    if (EFI_ERROR(status))
    {
        Print(L"failed opening system partition root: %r\r\n", status);
        return NULL;
    }

    status = root->Open(root, &file, path, EFI_FILE_MODE_READ, 0);

    if (EFI_ERROR(status))
    {
        Print(L"failed opening image file %s: %r\r\n", path, status);
        return NULL;
    }

    image = efi_allocate(sizeof(*image));

    if (!image)
    {
        Print(L"failed allocating image data: %r\r\n", e_last_error);
        goto failure;
    }

    image->file = file;

    UINTN ehdr_size = sizeof(image->ehdr);
    status = file->Read(file, &ehdr_size, &image->ehdr);

    if (EFI_ERROR(status))
    {
        Print(L"failed reading image file %s: %r\r\n", path, status);
        goto failure;
    }

    if (!validate_image(&image->ehdr))
    {
        Print(L"failed validating image file %s: %r\r\n", path, status);
        goto failure;
    }

    UINTN phdrs_size = image->ehdr.e_phentsize * image->ehdr.e_phnum;
    image->phdrs = efi_allocate(phdrs_size);

    if (!image->phdrs)
    {
        Print(L"failed allocating segment data: %r", e_last_error);
        goto failure;
    }

    status = file->Read(file, &phdrs_size, image->phdrs);

    if (EFI_ERROR(status))
    {
        Print(L"failed reading segment data: %r", status);
        goto failure;
    }

    return image;

failure:
    file->Close(file);
    return NULL;
}

static UINTN get_image_size(struct system_image *image)
{
    UINTN size = 0;
    Elf64_Phdr *phdrs = image->phdrs;

    for (int i = 0; i < image->ehdr.e_phnum; i++)
    {
        if (phdrs[i].p_type != PT_LOAD)
        {
            continue;
        }

        if (phdrs[i].p_paddr + phdrs[i].p_memsz > size)
        {
            size = phdrs[i].p_paddr + phdrs[i].p_memsz;
            if (phdrs[i].p_align > 1)
            size = (size + phdrs[i].p_align - 1) & ~(phdrs[i].p_align - 1);
        }
    }

    return size;
}

static bool load_system_image(struct system_image *image)
{
    image->buffer = get_system_buffer(get_image_size(image));

    if (!image->buffer.base)
    {
        return false;
    }

    e_bs->SetMem((VOID *)image->buffer.base, image->buffer.size, 0);

    Elf64_Ehdr *ehdr = &image->ehdr;
    Elf64_Phdr *phdrs = image->phdrs;

    Print(L"found %d segments in image\r\n", ehdr->e_phnum);

    EFI_STATUS status = EFI_SUCCESS;

    for (UINTN i = 0; i < ehdr->e_phnum && !EFI_ERROR(status); i++)
    {
        switch (phdrs[i].p_type)
        {
            case PT_LOAD:
                Print(L"loadable segment %16.0lx at offset %x of %d bytes\r\n",
                      phdrs[i].p_vaddr,
                      phdrs[i].p_offset,
                      phdrs[i].p_memsz);

                void *segment = (void *)(image->buffer.base + phdrs[i].p_paddr);
                UINTN segment_size = phdrs[i].p_filesz;

                image->file->SetPosition(image->file, phdrs[i].p_offset);
                status = image->file->Read(image->file, &segment_size, segment);
                break;
            default:
                continue;
        }
    }

    if (EFI_ERROR(status))
    {
        Print(L"failure reading segment from file: %r\r\n", status);
        return false;
    }

    return true;
}

static struct efi_memory_map_data *get_memory_map_data(void)
{
    EFI_STATUS status;
    UINTN mapsize = 0;
    struct efi_memory_map_data *map = NULL;

    status = e_bs->GetMemoryMap(&mapsize, NULL, NULL, NULL, NULL);

    if (status == EFI_BUFFER_TOO_SMALL)
    {
        map = efi_allocate(sizeof(*map));
    }

    if (map)
    {
        status = e_bs->GetMemoryMap(&map->size,
                                    map->data,
                                    &map->key,
                                    &map->descsize,
                                    &map->descver);
    }
    else
    {
        status = e_last_error;
    }
     
    if (EFI_ERROR(status))
    {
        Print(L"failed getting memory map: %r\r\n", status);
        return NULL;
    }

    return map;
}

static struct efi_framebuffer_data *get_framebuffer_data(void)
{
    struct efi_framebuffer_data *framebuffer;

    if (!e_graphics_output)
    {
        return NULL;
    }

    framebuffer = efi_allocate(sizeof(*framebuffer));

    if (framebuffer)
    {
        EFI_GRAPHICS_OUTPUT_PROTOCOL_MODE *mode = e_graphics_output->Mode;

        framebuffer->bitmask = mode->Info->PixelInformation;
        framebuffer->width = mode->Info->HorizontalResolution;
        framebuffer->height = mode->Info->VerticalResolution;
        framebuffer->base = mode->FrameBufferBase;
        framebuffer->size = mode->FrameBufferSize;
        framebuffer->pixel_format = mode->Info->PixelFormat;

        switch (framebuffer->pixel_format)
        {
            //falthrough
            case PixelBlueGreenRedReserved8BitPerColor:
            case PixelRedGreenBlueReserved8BitPerColor:
                framebuffer->pixel_size = sizeof(EFI_GRAPHICS_OUTPUT_BLT_PIXEL);
                break;
            default:
                // TODO: calculate pixel size for bitmask format
                framebuffer->pixel_size = 0;
        }

        framebuffer->pitch = framebuffer->width * framebuffer->pixel_size;
    }
    else
    {
        Print(L"failed getting framebuffer data: %r\r\n", e_last_error);
        return NULL;
    }

    return framebuffer;
}

static void *get_acpi_rsdp(void)
{
    EFI_GUID acpi_20_guid = ACPI_20_TABLE_GUID;

    for (UINTN i = 0; i < e_st->NumberOfTableEntries; i++)
    {
        if (!CompareGuid(&acpi_20_guid, &e_st->ConfigurationTable[i].VendorGuid))
        {
            Print(L"acpi rsdp: 0x%16.0lx\r\n",
                  e_st->ConfigurationTable[i].VendorTable);
            
            return e_st->ConfigurationTable[i].VendorTable;
        }
    }

    Print(L"failed getting acpi rsdp\r\n");
    return NULL;
}

