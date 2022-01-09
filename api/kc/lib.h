#pragma once

#include <stddef.h>

void *memcpy(void *dest, const void *src, size_t size);
void *memmove(void *dest, const void *src, size_t size);
void *memset(void *dest, int val, size_t size);
int memcmp(const void *str1, const void *str2, size_t count);

