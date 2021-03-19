#include "loader_efi.h"

#include <boot/entry/entry_efi.h>
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
};

static struct system_image *open_system_image(CHAR16 *path);
static bool load_system_image(struct system_image *image);
static struct efi_memory_map_data *get_memory_map_data(void);
static struct efi_framebuffer_data *get_framebuffer_data(void);
static void *get_acpi_rsdp(void);

void loader_main(void)
{
    struct efi_boot_data data;

    struct system_image *kernel_image = open_system_image(kernel_path);

    Print(L"loading %s\r\n", kernel_path);

    if (load_system_image(kernel_image))
    {
        Print(L"loaded kernel image at base %16.0x size %d bytes\r\n",
              kernel_image->buffer.base,
              kernel_image->buffer.size);
    }
    else
    {
        Print(L"failed loading kernel image\r\n");
        efi_exit(EFI_ABORTED);
    }

    kernel_entry_func kernel_entry;
    kernel_entry = (kernel_entry_func)(kernel_image->buffer.base +
                                       kernel_image->ehdr.e_entry);

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

    kernel_entry(&data);
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

    Print(L"allocated system buffer at %16.0x of %d bytes\r\n",
          buffer.base,
          buffer.size);

    return buffer;
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
                Print(L"loadable segment at offset %x of %d bytes\r\n",
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
            Print(L"acpi rsdp: 0x%16.0x\r\n",
                  e_st->ConfigurationTable[i].VendorTable);
            
            return e_st->ConfigurationTable[i].VendorTable;
        }
    }

    Print(L"failed getting acpi rsdp\r\n");
    return NULL;
}

