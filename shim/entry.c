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

static struct efi_loader_image *shim_image;
static struct efi_boot_data boot_data;

EFI_STATUS efi_shim_entry(struct efi_loader_image *image,
        struct efi_loader_interface *interface)
{
    /* 1. load kernel and boot modules into memory
     * 2. generate page tables for kernel and boot modules
     * 2. gather boot data, store as type SystemMemoryType
     * 3. exit boot services
     * 5. ???????????
     */
    
    EFI_STATUS status;

    if (!image)
    {
        return EFI_INVALID_PARAMETER;
    }
    else if (!shim_image)
    {
        shim_image = image;
        kernel_image.image_handle = shim_image->image_handle;
    }
    else
    {
        return EFI_ALREADY_STARTED;
    }

    if (!EFI_ERROR((status = interface->image_open(&kernel_image))) &&
            !EFI_ERROR((status = interface->image_alloc(&kernel_image))) &&
            !EFI_ERROR((status = interface->image_load(&kernel_image))))
    {
        (void)boot_data;
    }

    __asm__ volatile (
            "cli\n\t"
            "hlt\n\t"
            );

    return status;
}

