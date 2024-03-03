#include <efi/types.h>
#include <efi/error.h>
#include <efi/media.h>
#include <efi/tables.h>

#include "../kjarna.h"
#include "kjarna_efi.h"

struct efi_context const *efi_context;
EFI_STATUS efi_errno = EFI_SUCCESS;

EFI_STATUS _exit(EFI_STATUS status)
{
	return gBS->Exit(efi_context->handle, status, 0, nullptr);
}

EFI_STATUS _start(EFI_HANDLE handle, EFI_SYSTEM_TABLE *system_table)
{
	struct efi_context app_context = { handle, system_table };
	efi_context = &app_context;

	system_table->ConOut->ClearScreen(system_table->ConOut);

	if (main(0, nullptr) != 0)
	{
		efi_errno = EFI_ABORTED;
	}


	return _exit(efi_errno);
}

