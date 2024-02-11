#pragma once

#include <posix/sys/types.h>

#include <efi/types.h>
#include <efi/tables.h>

#include <kjarna/interface.h>

struct efi_context
{
	EFI_HANDLE const handle;
	EFI_SYSTEM_TABLE * const system_table;
};

extern struct efi_context const *efi_context;
extern EFI_STATUS efi_errno;

#define gST efi_context->system_table
#define gBS efi_context->system_table->BootServices

void *efi_get_protocol(EFI_HANDLE handle, EFI_GUID *guid);

int SYSV_ABI efi_open(const char *path, int flags, int mode);
int SYSV_ABI efi_close(int fd);
off_t SYSV_ABI efi_lseek(int fd, off_t offset, int whence);
ssize_t SYSV_ABI efi_read(int fd, void *buffer, size_t length);
ssize_t SYSV_ABI efi_write(int fd, const void *buffer, size_t length);

void * SYSV_ABI efi_mmap(void *addr, size_t length, int prot, int flags, int fd, off_t offset);
int SYSV_ABI efi_munmap(void *addr, size_t length);

void *efi_alloc_pool(size_t size);
void *efi_calloc_pool(size_t count, size_t size);
void efi_free_pool(void *block);

int runtime_start(void);

