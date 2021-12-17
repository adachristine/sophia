#pragma once

#include <loader/shim.h>

#include <efi.h>

#ifdef __ELF__
    #define EFIEXPORT __attribute__((visibility("default"))) 
#else
    #define EFIEXPORT __declspec(dllexport)
#endif

#define OsVendorMemoryType(t) ((EFI_MEMORY_TYPE)(0x80000000|t))
#define SystemMemoryType OsVendorMemoryType(1)

struct efi_loader_image
{
    CHAR16 *path;
    EFI_HANDLE image_handle;
    EFI_SYSTEM_TABLE *system_table;
    EFI_FILE_PROTOCOL *root;
    EFI_FILE_PROTOCOL *file;
    EFI_PHYSICAL_ADDRESS buffer_base;
    UINTN buffer_size;
};

typedef EFI_STATUS (*efi_image_func)(struct efi_loader_image *image);
typedef EFI_STATUS (*efi_page_alloc_func)(EFI_MEMORY_TYPE type,
        UINTN size,
        EFI_PHYSICAL_ADDRESS *base);
typedef EFI_STATUS (*efi_page_free_func)(EFI_PHYSICAL_ADDRESS base, UINTN size);

struct efi_loader_interface
{
    efi_page_alloc_func page_alloc;
    efi_page_free_func page_free;

    efi_image_func image_open;
    efi_image_func image_alloc;
    efi_image_func image_load;
};

typedef EFI_STATUS (*efi_shim_entry_func)(struct efi_loader_image *image,
    struct efi_loader_interface *interface);
