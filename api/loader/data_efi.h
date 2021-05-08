#pragma once

#include <kernel/memory/range.h>

#include <efi.h>

#include <stddef.h>

#define UserMemoryType ((EFI_MEMORY_TYPE)0x80000000)
#define SystemMemoryType (UserMemoryType|0x01)

enum efi_boot_data_type
{
    MEMORY_MAP_DATA,
    FRAMEBUFFER_DATA,
    ACPI_DATA,
};

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
    struct memory_range buffer;
    UINT32 width;
    UINT32 height;
    UINT32 pitch;
    UINT8 pxsize;
    EFI_PIXEL_BITMASK bitmask;
    EFI_GRAPHICS_PIXEL_FORMAT pxformat;
};

struct efi_acpi_data
{
    void *rsdp;
};

struct efi_boot_data
{
    struct efi_memory_map_data *memory_map;
    struct efi_framebuffer_data *framebuffer;
    struct efi_acpi_data *acpi;
};
