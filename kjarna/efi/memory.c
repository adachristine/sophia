#include <libc/string.h>

#include <efi/services.h>
#include <efi/error.h>

#include <stddef.h>

#include "kjarna_efi.h"

void *efi_alloc_pool(size_t size)
{
	void *buffer;

	efi_errno = gBS->AllocatePool(
			EfiLoaderData,
			size,
			&buffer);

	if (EFI_ERROR(efi_errno))
	{
		return nullptr;
	}

	return buffer;
}

void *efi_calloc_pool(size_t count, size_t size)
{
	void *buffer = efi_alloc_pool(count * size);

	if (buffer != nullptr)
	{
		memset(buffer, 0, count * size);
	}

	return buffer;
}

void efi_free_pool(void *buffer)
{
	efi_errno = gBS->FreePool(buffer);
}

#define KjarnaData EFIX_OS_MEMORY_TYPE(1)

void *efi_mmap(void *addr, size_t length, int prot, int flags, int fd, off_t offset)
{
	(void)prot;
	(void)flags;
	(void)fd;
	(void)offset;

	EFI_ALLOCATE_TYPE alloc_type = AllocateAnyPages;
	EFI_MEMORY_TYPE memory_type = KjarnaData;

	void *buffer;

	if (addr != nullptr)
	{
		alloc_type = AllocateAddress;
	}

	efi_errno = gBS->AllocatePages(
			alloc_type, 
			memory_type,
			EFI_SIZE_TO_PAGES(length),
			(uintptr_t *)&buffer);

	if (EFI_ERROR(efi_errno))
	{
		return nullptr;
	}

	return buffer;
}

int efi_munmap(void *addr, size_t length)
{
	efi_errno = gBS->FreePages((uintptr_t)addr, EFI_SIZE_TO_PAGES(length));

	return !EFI_ERROR(efi_errno) ? 0 : -1;
}

