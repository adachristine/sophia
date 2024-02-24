#pragma once

#include <stddef.h>
#include "sys/types.h"

#define STDIN_FILENO 0
#define STDOUT_FILENO 1
#define STDERR_FILENO 2

enum seek_whence
{
	SEEK_SET,
	SEEK_CUR,
	SEEK_END
};

ssize_t read(int fd, void *buf, size_t length);
ssize_t write(int fd, const void *buf, size_t length);
off_t lseek(int fd, off_t position, int whence);
int close(int fd);

