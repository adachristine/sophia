#pragma once

#include <efi.h>
#include <efilib.h>

#include <stddef.h>
#include <stdnoreturn.h>

extern EFI_SYSTEM_TABLE *e_st;
extern EFI_BOOT_SERVICES *e_bs;
extern EFI_RUNTIME_SERVICES *e_rt;

extern EFI_HANDLE e_image_handle;
extern EFI_LOADED_IMAGE_PROTOCOL *e_loaded_image;
extern EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *e_system_partition;
extern EFI_GRAPHICS_OUTPUT_PROTOCOL *e_graphics_output;
extern EFI_STATUS e_last_error;

noreturn void efi_exit(EFI_STATUS status);

void *efi_allocate(size_t size);
void efi_free(void *buffer);

void loader_main(void);

