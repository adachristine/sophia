#pragma once

#include <lib.h>

#include <stdarg.h>

typedef struct FILE FILE;

extern FILE *kstdout;
extern FILE *kstderr;

extern int kfputc(int c, FILE *f);

int kvfprintf(FILE *f, const char *restrict format, va_list arguments);
int kfprintf(FILE *f, const char *restrict format, ...);
int kvprintf(const char *restrict format, va_list arguments);
int kprintf(const char *restrict format, ...);

