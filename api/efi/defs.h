#pragma once

#define EFI_IMAGE_SUBSYSTEM_EFI_APPLICATION 10

#define EFI_IMAGE_MACHINE_x64 0x8664

#define IN
#define OUT
#define OPTIONAL
#define CONST const

#if defined(__ELF__) && defined(__GNUC__)
#define EFIAPI __attribute__((ms_abi))
#else
#define EFIAPI
#endif

#define EFI_PAGE_SIZE 4096
#define EFI_SIZE_TO_PAGES(size) (((size + EFI_PAGE_SIZE - 1) & ~(EFI_PAGE_SIZE - 1)) / EFI_PAGE_SIZE)

