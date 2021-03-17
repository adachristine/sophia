#include "loader_efi.h"

void *efi_allocate(size_t size)
{
    EFI_STATUS status;
    void *buffer;

    status = e_bs->AllocatePool(EfiLoaderData, size, &buffer);

    if (EFI_ERROR(status))
    {
        e_last_error = status;
        buffer = NULL;
    }

    return buffer;
}

void efi_free(void *buffer)
{
    EFI_STATUS status;
    
    status = e_bs->FreePool(buffer);
    
    if (EFI_ERROR(status))
    {
        e_last_error = status;
    }
}

