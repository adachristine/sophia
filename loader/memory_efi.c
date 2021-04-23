#include "loader_efi.h"

#include <loader/data.h>
#include <kernel/memory/paging.h>

struct memory_range system_allocate(size_t size)
{
    EFI_STATUS status;
    struct memory_range buffer;

    buffer.type = SYSTEM_MEMORY;
    buffer.size = size;

    status = e_bs->AllocatePages(AllocateAnyPages,
                                 SystemMemoryType,
                                 page_count(buffer.size, 1),
                                 &buffer.base);

    if (EFI_ERROR(status))
    {
        e_last_error = status;
        buffer.type = UNUSABLE_MEMORY;
    }

    return buffer;
}

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

