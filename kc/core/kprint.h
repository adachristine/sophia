#pragma once

#include <stdarg.h>

int kputchar(int c);
int kputs(const char *s);
int kprintf(const char *restrict format, ...);
int kvprintf(const char *restrict format, va_list arguments);

