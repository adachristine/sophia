#include "loader_efi.h"

#include <stdbool.h>
#include <stddef.h>

EFI_SYSTEM_TABLE *e_st;
EFI_BOOT_SERVICES *e_bs;
EFI_RUNTIME_SERVICES *e_rt;

EFI_HANDLE e_image_handle;
EFI_LOADED_IMAGE_PROTOCOL *e_loaded_image;
EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *e_system_partition;
EFI_GRAPHICS_OUTPUT_PROTOCOL *e_graphics_output;
EFI_STATUS e_last_error;

extern int _text;
extern int _data;

static EFI_LOADED_IMAGE_PROTOCOL *open_loaded_image(void);
static EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *open_system_partition(void);
static EFI_GRAPHICS_OUTPUT_PROTOCOL *open_graphics_output(void);

EFI_STATUS efi_main(EFI_HANDLE image_handle, EFI_SYSTEM_TABLE *system_table)
{
    e_image_handle = image_handle;
    e_st = system_table;
    e_bs = e_st->BootServices;
    e_rt = e_st->RuntimeServices;
    
    InitializeLib(e_image_handle, e_st);
    Print(L"loader starting\r\n");
    
    e_loaded_image = open_loaded_image();
    e_system_partition = open_system_partition();
    e_graphics_output = open_graphics_output();

    loader_main();

    efi_exit(EFI_ABORTED);
}

noreturn void efi_exit(EFI_STATUS status)
{
    if (EFI_ERROR(status))
    {
        Print(L"loader exited: %r\r\n", status);
    }
    
    e_bs->Exit(e_image_handle, status, 0, NULL);

    while (true);
}

static EFI_LOADED_IMAGE_PROTOCOL *open_loaded_image(void)
{
    Print(L"opening loaded image\r\n");
    
    EFI_STATUS status;
    EFI_LOADED_IMAGE_PROTOCOL *image;
    
    status = e_bs->OpenProtocol(e_image_handle,
                                &LoadedImageProtocol,
                                (void **)&image,
                                e_image_handle,
                                NULL,
                                EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL);
    
    if (EFI_ERROR(status))
    {
        Print(L"failed opening loaded image\r\n");
        efi_exit(status);
    }

    Print(L"image base: 0x%16.0x\r\n", image->ImageBase);
    Print(L"image .text segment: 0x%16.0x\r\n", &_text);
    Print(L"image .data segment: 0x%16.0x\r\n", &_data);
    
    return image;
}

static EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *open_system_partition(void)
{
    Print(L"opening system partition\r\n");
    
    EFI_STATUS status;
    EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *partition;
    
    status = e_bs->OpenProtocol(e_loaded_image->DeviceHandle,
                               &FileSystemProtocol,
                               (void **)&partition,
                               e_image_handle,
                               NULL,
                               EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL);

    if (EFI_ERROR(status))
    {
        Print(L"failed opening system partition\r\n");
        efi_exit(status);
    }
    
    Print(L"opened system partition\r\n");
    
    return partition;
}

static EFI_GRAPHICS_OUTPUT_PROTOCOL *open_graphics_output(void)
{
    EFI_STATUS status;
    EFI_GRAPHICS_OUTPUT_PROTOCOL *graphics;

    Print(L"opening graphics output\r\n");  
 
    status = e_bs->LocateProtocol(&GraphicsOutputProtocol,
                                  NULL,
                                  (VOID **)&graphics);

    if (EFI_ERROR(status))
    {
        Print(L"failed opening graphics output: %r\r\n", status);
        return NULL;
    }

    Print(L"graphics output opened\r\n");

    return graphics;
}

