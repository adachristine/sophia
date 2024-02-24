#include <efi/types.h>
#include <efi/error.h>
#include <efi/media.h>
#include <efi/tables.h>

#include <posix/unistd.h>
#include <libc/stdlib.h>
#include <libc/string.h>

#include "kjarna_efi.h"

#define FD_COUNT 16
#define FD_MIN 3
#define FD_MAX (FD_MIN + FD_COUNT - 1)

static EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *fd_to_outconsole(int fd)
{
	switch (fd)
	{
		case STDOUT_FILENO:
			return efi_context->system_table->ConOut;
		case STDERR_FILENO:
			return efi_context->system_table->StdErr;
		default:
			return nullptr;
	}
}

static EFI_SIMPLE_TEXT_INPUT_PROTOCOL *fd_to_inconsole(int fd)
{
	return fd == STDIN_FILENO ? efi_context->system_table->ConIn : nullptr;
}

static EFI_FILE_PROTOCOL *open_files[FD_COUNT];

static EFI_FILE_PROTOCOL *fd_to_file(int fd)
{
	if (fd < FD_MIN || fd > FD_MAX)
	{
		return nullptr;
	}

	return open_files[fd];
}

static int alloc_fd(void)
{
	for (int i = 0; i < FD_COUNT; i++)
	{
		if (open_files[i] == nullptr)
		{
			return FD_MIN + i;
		}
	}

	return -1;
}

static void free_fd(int fd)
{
	if (fd < FD_MIN || fd > FD_MAX)
	{
		open_files[fd] = nullptr;
	}
}

int efi_open(const char *path, int flags, int mode)
{
	static EFI_GUID lip_guid = EFI_LOADED_IMAGE_PROTOCOL_GUID;
	static EFI_GUID sfsp_guid = EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_GUID;
		
	EFI_LOADED_IMAGE_PROTOCOL *lip = efi_get_protocol(efi_context->handle, &lip_guid);

	if (lip == nullptr)
	{
		return -1;
	}

	EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *sfsp = efi_get_protocol(lip->DeviceHandle, &sfsp_guid);

	if (sfsp == nullptr)
	{
		return -1;
	}

	EFI_FILE_PROTOCOL *root = nullptr;

	efi_errno = sfsp->OpenVolume(sfsp, &root);

	if (root == nullptr)
	{
		return -1;
	}

	int fd = alloc_fd();

	if (fd < FD_MIN)
	{
		return -1;
	}

	size_t path_len = strlen(path) + 1;
	CHAR16 *wcpath = efi_calloc_pool(path_len, sizeof(*wcpath));

	if (mbstowcs(wcpath, path, path_len) == (size_t)-1)
	{
		efi_errno = EFI_INVALID_PARAMETER;
	}
	else
	{
		efi_errno = root->Open(
				root, 
				&open_files[fd], 
				wcpath, 
				EFI_FILE_MODE_READ, 
				0);
	}

	efi_close_protocol(lip->DeviceHandle, &lip_guid);
	efi_close_protocol(efi_context->handle, &sfsp_guid);
	efi_free_pool(wcpath);

	(void)flags;
	(void)mode;

	return !EFI_ERROR(efi_errno) ? fd : -1;
}

int efi_close(int fd)
{
	EFI_FILE_PROTOCOL *file = fd_to_file(fd);

	if (file == nullptr)
	{
		efi_errno = EFI_INVALID_PARAMETER;
		return -1;
	}

	free_fd(fd);

	efi_errno = file->Close(file);

	return !EFI_ERROR(efi_errno) ? 0 : -1;
}

off_t efi_lseek(int fd, off_t offset, int whence)
{
	EFI_FILE_PROTOCOL *file = fd_to_file(fd);

	if (file == nullptr)
	{
		efi_errno = EFI_INVALID_PARAMETER;
		return -1;
	}

	off_t current = 0;

	if (whence == SEEK_END)
	{
		efi_errno = file->SetPosition(file, (UINTN)-1);
	}

	if (EFI_ERROR(efi_errno))
	{
		return -1;
	}

	if (whence == SEEK_CUR || whence == SEEK_END)
	{
		efi_errno = file->GetPosition(file, (UINTN *)&current);
	}

	if (EFI_ERROR(efi_errno))
	{
		return -1;
	}

	efi_errno = file->SetPosition(file, (UINTN)(offset + current));

	return !EFI_ERROR(efi_errno) ? offset : -1;
}

static ssize_t efi_console_read(EFI_SIMPLE_TEXT_INPUT_PROTOCOL *console, CHAR16 *buffer, size_t length)
{
	//TODO: implement user input
	(void)console;
	(void)buffer;
	efi_errno = EFI_INVALID_PARAMETER;
	return !EFI_ERROR(efi_errno) ? length : -1;
}

ssize_t efi_read(int fd, void *buffer, size_t length)
{
	if (buffer == nullptr)
	{
		efi_errno = EFI_INVALID_PARAMETER;
		return -1;
	}

	EFI_SIMPLE_TEXT_INPUT_PROTOCOL *console = fd_to_inconsole(fd);

	if (console != nullptr)
	{
		return efi_console_read(console, buffer, length);
	}

	EFI_FILE_PROTOCOL *file = fd_to_file(fd);

	if (file == nullptr)
	{
		efi_errno = EFI_INVALID_PARAMETER;
		return -1;
	}

	size_t bytes = length;
	efi_errno = file->Read(file, &bytes, buffer);

	return !EFI_ERROR(efi_errno) ? bytes : -1;
}

static ssize_t efi_console_write(EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *console, const void *buffer, size_t length)
{
	if (length == (size_t)-1)
	{
		length = strlen(buffer);
	}

	wchar_t *ws = efi_calloc_pool(length + 2, sizeof(*ws));

	if(mbstowcs(ws, buffer, length + 2) == (size_t)-1)
	{
		efi_errno = EFI_INVALID_PARAMETER;
	}
	else
	{
		ws[length] = L'\r';
		efi_errno = console->OutputString(console, ws);
	}

	efi_free_pool(ws);

	return !EFI_ERROR(efi_errno) ? length : -1;
}

ssize_t efi_write(int fd, const void *buffer, size_t length)
{
	if (buffer == nullptr)
	{
		efi_errno = EFI_INVALID_PARAMETER;
		return -1;
	}

	EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *console = fd_to_outconsole(fd);

	if (console != nullptr)
	{
		return efi_console_write(console, buffer, length);
	}

	EFI_FILE_PROTOCOL *file = fd_to_file(fd);

	if (file == nullptr)
	{
		efi_errno = EFI_INVALID_PARAMETER;
		return -1;
	}

	size_t bytes = length;
	efi_errno = file->Write(file, &bytes, (void *)buffer);

	return !EFI_ERROR(efi_errno) ? bytes : -1;
}

