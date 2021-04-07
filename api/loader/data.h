#pragma once

#include <efi.h>

#define EfiUserMemoryType (EFI_MEMORY_TYPE)0x80000000
#define SystemMemoryType EfiUserMemoryType | 0x01

struct efi_memory_map_data
{
    UINTN size;
    UINTN key;
    UINTN descsize;
    UINT32 descver;
    EFI_MEMORY_DESCRIPTOR data[];
};

struct efi_framebuffer_data
{
    EFI_PIXEL_BITMASK bitmask;
    EFI_PHYSICAL_ADDRESS base;
    UINTN size;
    UINT32 width;
    UINT32 height;
    UINT32 pitch;
    EFI_GRAPHICS_PIXEL_FORMAT pixel_format;
    UINT8 pixel_size;
};

struct efi_boot_data
{
    EFI_SYSTEM_TABLE *system_table;
    struct efi_memory_map_data *memory_map;
    struct efi_framebuffer_data *framebuffer;
    void *acpi_rsdp;
};

