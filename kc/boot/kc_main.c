#include <loader/efi/shim.h>
#include <kernel/memory/range.h>

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

static void collect_boot_data(void)
{
    EFI_BOOT_SERVICES *ebs = loader_interface->system_table->BootServices;
    EFI_STATUS status;
    
    boot_data.memory_map.size = 0;

    status = ebs->GetMemoryMap(&boot_data.memory_map.size,
            NULL,
            NULL,
            NULL,
            NULL);

    if (EFI_BUFFER_TOO_SMALL == status)
    {
        boot_data.memory_map.size += EFI_PAGE_SIZE;
        status = loader_interface->page_alloc(SystemMemoryType,
                boot_data.memory_map.size,
                (EFI_PHYSICAL_ADDRESS *)&boot_data.memory_map.buffer.base);
    }

    if (!EFI_ERROR(status))
    {
        boot_data.memory_map.size = boot_data.memory_map.buffer.size;

        status = ebs->GetMemoryMap(&boot_data.memory_map.size,
                (EFI_MEMORY_DESCRIPTOR *)&boot_data.memory_map.buffer.base,
                &boot_data.memory_map.key,
                &boot_data.memory_map.descsize,
                &boot_data.memory_map.descver);
    }

    if (EFI_ERROR(status))
    {
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

    if (!EFI_ERROR((status = interface->image_open(&kernel_image))) &&
            !EFI_ERROR((status = interface->image_alloc(&kernel_image))) &&
            !EFI_ERROR((status = interface->image_load(&kernel_image))))
    {
        collect_boot_data();
    }

    __asm__ volatile (
            "cli\n\t"
            "hlt\n\t"
            );

    return status;
}

