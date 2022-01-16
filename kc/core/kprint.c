#include "kprint.h"
#include "serial.h"

#include <stdarg.h>

__attribute__((optimize("O3")))
int kputchar(int c)
{
    return serial_putchar(c);
}

__attribute__((optimize("O3")))
int kputs(const char *s)
{
    while (*s)
    {
        kputchar(*s++);
    }

    return 0;
}

