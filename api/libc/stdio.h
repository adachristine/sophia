#pragma once

#include <libc/string.h>

#include <stdarg.h>

typedef struct FILE FILE;

extern FILE *stdin;
extern FILE *stdout;
extern FILE *stderr;

int fputc(int c, FILE *f);
int fputs(const char *str, FILE *f);

int vfprintf(FILE *f, const char *restrict format, va_list arguments);
int fprintf(FILE *f, const char *restrict format, ...);
int vprintf(const char *restrict format, va_list arguments);
int printf(const char *restrict format, ...);

