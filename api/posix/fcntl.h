#pragma once

#include "sys/types.h"

#define O_RDONLY 0

int open(const char *path, int flags, mode_t mode);

