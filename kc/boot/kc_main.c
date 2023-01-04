#include "loader_paging.h"

#include <efi/graphics.h>
#include <efi/acpi.h>
#include <efi/error.h>

#include <loader/efi/shim.h>
#include <kernel/entry.h>
#include <kernel/memory/range.h>
#include <sophialib.h>

#include <stdbool.h>

#include "../lib/elf.h"

#define KERNEL_IMAGE_BASE 0xffffffff80000000
#define OBJECT_SPACE_SIZE 131072

struct efi_memory_map
{
    struct memory_range buffer;
    UINTN size;
    UINTN key;
    UINTN descsize;
    UINT32 descver;
};

struct efi_boot_data
{
    struct efi_memory_map memory;
    struct kc_boot_video_data video;
    struct kc_boot_acpi_data acpi;
};

struct efi_loader_image kernel_image =
{
    .path = L"\\adasoft\\sophia\\kernel.os"
};

static void *boot_data_alloc(size_t size);

static struct efi_loader_interface *loader_interface;
static struct efi_boot_data boot_data;
static EFI_BOOT_SERVICES *e_bs = NULL;
static uint64_t *system_page_map = NULL;
static EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *eto;
static struct kc_boot_data *k_boot_data;

static void *object_space_base;
static size_t object_space_size;

static void *boot_data_alloc(size_t size)
{
    size = (size + sizeof(uint64_t) - 1) & ~(sizeof(uint64_t) - 1);

    if (size > k_boot_data->buffer.available_size)
    {
        return NULL;
    }

    k_boot_data->buffer.available_size -= size;

    void *buffer = k_boot_data->buffer.current;

    k_boot_data->buffer.current += size;

    return buffer;
}

static inline void debug()
{
    __asm__ volatile
        (
         "push %rax\r\n"
         "mov -8(%rsp), %rax\n\t"
         "mov %rax, %dr0\n\t"
         "pop %rax\r\n"
         "pushf\n\t"
         "cli\n\t"
         "int3\n\t"
         "1:\n\t"
         "hlt\n\t"
         "jmp 1b\n\t"
        );
}

static inline void plog(CHAR16 *message)
{
    if (eto) eto->OutputString(eto, message);
}

uint64_t *new_page_table(void)
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

    // create a self-mapped page table at the very top of the virtual
    // address space. this will give us 2MiB - 4KiB of virtual space to work
    // with beginning at -2MiB .
    //
    // this is where any temporary mappings will be set up
    // including the boot data tables that will be passed into the kernel
    // and also the temporary window that the kernel will continue to use

    map_page(
            system_page_map,
            -1ULL,
            (uint64_t)get_page_table(system_page_map, -1ULL, 1),
            DATA_PAGE_TYPE);

    object_space_base = (unsigned char *)-page_size(2);
    object_space_size = page_size(2) - page_size(1);
}

static size_t calculate_pixel_size(EFI_PIXEL_BITMASK bitmask)
{
    (void)bitmask;
    return 0;
}

static size_t get_video_pitch(
        UINTN scanline_pixels,
        EFI_GRAPHICS_PIXEL_FORMAT format,
        EFI_PIXEL_BITMASK bitmask)
{
    switch (format)
    {
        case PixelBlueGreenRedReserved8BitPerColor:
        case PixelRedGreenBlueReserved8BitPerColor:
            return sizeof(EFI_GRAPHICS_OUTPUT_BLT_PIXEL) * scanline_pixels;
        case PixelBitMask:
            return calculate_pixel_size(bitmask) * scanline_pixels;
        default:
            return 0;
    }
}

static enum video_pixel_format get_video_format(
        EFI_GRAPHICS_PIXEL_FORMAT format)
{
    switch (format)
    {
        case PixelBlueGreenRedReserved8BitPerColor:
            return BGRA32BPP_VIDEO_FORMAT;
        case PixelRedGreenBlueReserved8BitPerColor:
            return RGBA32BPP_VIDEO_FORMAT;
        case PixelBitMask:
            return NO_VIDEO_FORMAT;
        default:
            return 0;
    }
}

static void collect_video_data(void)
{
    EFI_GUID gop_guid = EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID;
    EFI_STATUS status;
    EFI_GRAPHICS_OUTPUT_PROTOCOL *gop_interface;

    status = e_bs->LocateProtocol(&gop_guid, NULL, (void **)&gop_interface);

    if (EFI_ERROR(status))
    {
        plog(L"found no graphics devices\r\n");
    }
    else
    {
        EFI_GRAPHICS_OUTPUT_PROTOCOL_MODE *mode = gop_interface->Mode;
        EFI_GRAPHICS_OUTPUT_MODE_INFORMATION *info = mode->Info;
        plog(L"found graphics device\r\n");

        boot_data.video = (struct kc_boot_video_data){
            {
                RESERVED_MEMORY,
                    mode->FrameBufferBase,
                    mode->FrameBufferSize
            },
                info->HorizontalResolution,
                info->VerticalResolution,
                get_video_pitch(
                        info->PixelsPerScanLine,
                        info->PixelFormat,
                        info->PixelInformation),
                get_video_format(info->PixelFormat)
        };
    }
}

static void collect_acpi_data(void)
{
    EFI_GUID acpi2_guid = EFI_ACPI_20_TABLE_GUID;
    EFI_GUID acpi_guid = ACPI_TABLE_GUID;

    UINTN config_table_count = 
        loader_interface->system_table->NumberOfTableEntries;
    EFI_CONFIGURATION_TABLE *config_table =
        loader_interface->system_table->ConfigurationTable;

    for (UINTN i = 0; i < config_table_count; i++)
    {
        // check first for ACPI RSDP 2.0
        if (!memcmp(
                    &config_table[i].VendorGuid,
                    &acpi2_guid,
                    sizeof(EFI_GUID)))
        {
            plog(L"found ACPI RSDP 2.0\r\n");
            // if we find the RSDP 2.0 table we can leave it at that
            boot_data.acpi.rsdp = config_table[i].VendorTable;
            boot_data.acpi.version = ACPI_CURRENT;
            break;
        }
        // if we get through the table without finding a v2 RSDP
        // then we must continue to look for a v1 RSDP
        // (unlikely)
        if (!memcmp(
                    &config_table[i].VendorGuid,
                    &acpi_guid,
                    sizeof(EFI_GUID)))
        {
            plog(L"found ACPI RSDP 1.0\r\n");
            boot_data.acpi.rsdp = config_table[i].VendorTable;
            boot_data.acpi.version = ACPI_LEGACY;
            // we must keep looking for the ACPI2.0 table GUID because
            // it might be beyond this entry
            continue;
        }
    }
}

static void collect_memory_map(void)
{
    EFI_STATUS status;

    boot_data.memory.size = 0;

    status = e_bs->GetMemoryMap(&boot_data.memory.size,
            NULL,
            &boot_data.memory.key,
            NULL,
            NULL);

    if (EFI_BUFFER_TOO_SMALL == status)
    {
        plog(L"getting memory map size\r\n");
        boot_data.memory.buffer.size = boot_data.memory.size += 
            EFI_PAGE_SIZE;
        status = loader_interface->page_alloc(SystemMemoryType,
                boot_data.memory.buffer.size,
                (EFI_PHYSICAL_ADDRESS *)&boot_data.memory.buffer.base);
    }

    if (EFI_ERROR(status))
    {
        plog(L"failed allocating pages for memory map\r\n");
    }

    if (!EFI_ERROR(status))
    {
        plog(L"got the memory map size\r\n");
        boot_data.memory.size = boot_data.memory.buffer.size;

        plog(L"getting memory map\r\n");
        status = e_bs->GetMemoryMap(&boot_data.memory.size,
                (EFI_MEMORY_DESCRIPTOR *)boot_data.memory.buffer.base,
                &boot_data.memory.key,
                &boot_data.memory.descsize,
                &boot_data.memory.descver);
    }

    if (EFI_ERROR(status))
    {
        plog(L"failed getting memory map\r\n");
        debug();
    }

}

static void collect_boot_data(void)
{
    plog(L"collecting boot data\r\n");
    collect_video_data();
    collect_acpi_data();
    collect_memory_map();
}

static void convert_acpi_data(void)
{
    k_boot_data->acpi = boot_data.acpi;
}

static void convert_video_data(void)
{
    k_boot_data->video = boot_data.video;
}

static void convert_memory_map(void)
{
    k_boot_data->memory.count = 
        boot_data.memory.size / boot_data.memory.descsize;
    k_boot_data->memory.entries = 
        boot_data_alloc(sizeof(struct memory_range) * 
                k_boot_data->memory.count);

    for (size_t i = 0; i < k_boot_data->memory.count; i++)
    {
        EFI_MEMORY_DESCRIPTOR *desc;
        desc = (EFI_MEMORY_DESCRIPTOR *)(boot_data.memory.buffer.base + 
                i * 
                boot_data.memory.descsize);

        enum memory_range_type type;
        phys_addr_t base;
        size_t size;

        base = desc->PhysicalStart;
        size = desc->NumberOfPages * EFI_PAGE_SIZE;

        switch (desc->Type)
        {
            case EfiConventionalMemory:
            case EfiBootServicesData:
            case EfiBootServicesCode:
            case EfiLoaderCode:
            case EfiLoaderData:
                type = AVAILABLE_MEMORY;
                break;
            case SystemMemoryType:
                type = SYSTEM_MEMORY;
                break;
            case EfiRuntimeServicesData:
            case EfiRuntimeServicesCode:
            case EfiACPIReclaimMemory:
            case EfiACPIMemoryNVS:
            case EfiPalCode:
                type = FIRMWARE_MEMORY;
                break;
            case EfiMemoryMappedIO:
            case EfiMemoryMappedIOPortSpace:
                type = MMIO_MEMORY;
                break;
            case EfiUnusableMemory:
                type = INVALID_MEMORY;
                break;
            default:
                type = RESERVED_MEMORY;
        }

        k_boot_data->memory.entries[i]
            = (struct memory_range){type, base, size};
    }
}

static EFI_STATUS enter_kernel(Elf64_Ehdr *ehdr)
{
    EFI_STATUS status;

    plog(L"exiting boot services\r\n");
    collect_boot_data();

    status = e_bs->ExitBootServices(loader_interface->image_handle,
            boot_data.memory.key);

    if (EFI_ERROR(status))
    {
        plog(L"failed exiting boot services:\r\n");
        debug();
    }

    set_page_map(system_page_map);

    k_boot_data = (struct kc_boot_data *)object_space_base;
    k_boot_data->buffer.base = (unsigned char *)k_boot_data;
    k_boot_data->buffer.max_size = object_space_size;

    k_boot_data->buffer.current = 
        k_boot_data->buffer.base + sizeof(*k_boot_data);
    k_boot_data->buffer.available_size =
        k_boot_data->buffer.max_size - sizeof(*k_boot_data);

    convert_memory_map();
    convert_video_data();
    convert_acpi_data();

    kc_entry_func kernel_entry = (kc_entry_func)
        (KERNEL_IMAGE_BASE + ehdr->e_entry);
    kernel_entry(k_boot_data);

    return status;
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

    plog(L"opening kernel image\r\n");

    if (!EFI_ERROR((status = interface->image_open(&kernel_image))) &&
            !EFI_ERROR((status = interface->image_alloc(&kernel_image))) &&
            !EFI_ERROR((status = interface->image_load(&kernel_image))))
    {
        plog(L"creating page tables\r\n");
        system_page_map = new_page_table();
        // parasitic map of uefi page tables is this ok???????
        uint64_t *efi_map = get_page_map();
        system_page_map[0] = efi_map[0];

        Elf64_Ehdr *ehdr = (Elf64_Ehdr *)kernel_image.buffer_base;

        plog(L"mapping kernel pages\r\n");
        create_kernel_maps(ehdr, (void *)KERNEL_IMAGE_BASE);

        // set up the object space buffer
        // this will be discarded after boot time (maybe idk)
        plog(L"creating object space buffer\r\n");
        EFI_PHYSICAL_ADDRESS object_space_phys_base;
        UINTN object_space_phys_size = OBJECT_SPACE_SIZE;

        status = loader_interface->page_alloc(
                SystemMemoryType,
                object_space_phys_size,
                &object_space_phys_base);

        if (EFI_ERROR(status))
        {
            plog(L"failed allocating physical buffer for object space\r\n");
            debug();
        }

        map_pages(
                system_page_map,
                (uintptr_t)object_space_base,
                object_space_phys_base,
                DATA_PAGE_TYPE,
                object_space_phys_size);
        enter_kernel(ehdr);
    }

    __asm__ volatile (
            "cli\n\t"
            "hlt\n\t"
            );

    return status;
}

