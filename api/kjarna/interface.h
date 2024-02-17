#pragma once

#include <stddef.h>
#include <posix/sys/types.h>

#define SYSV_ABI __attribute__((sysv_abi))

struct kjarna_interface
{
	int (SYSV_ABI *open)(const char *, int, int);
	int (SYSV_ABI *close)(int);
	off_t (SYSV_ABI *lseek)(int, off_t, int);
	ssize_t (SYSV_ABI *read)(int, void *, size_t);
	ssize_t (SYSV_ABI *write)(int, const void *, size_t);

	void *(SYSV_ABI *mmap)(void *, size_t, int, int, int, off_t);
	int (SYSV_ABI *munmap)(void *, size_t);
};

struct kjarna_entry_params
{
	struct kjarna_interface *interface;
	int argc;
	char **argv;
	char **envp;
};

typedef int (SYSV_ABI kjarna_image_entry_func)(struct kjarna_entry_params *);

