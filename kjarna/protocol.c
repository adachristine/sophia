#include <efi/error.h>

#include "kjarna_efi.h"

void *efi_get_protocol(EFI_HANDLE handle, EFI_GUID *guid)
{
	void *protocol;

	efi_errno = gBS->OpenProtocol(
			handle,
			guid,
			&protocol,
			efi_context->handle,
			nullptr,
			EFI_OPEN_PROTOCOL_GET_PROTOCOL);

	if (EFI_ERROR(efi_errno))
	{
		return nullptr;
	}

	return protocol;
}

