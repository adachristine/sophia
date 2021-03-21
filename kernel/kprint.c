#include "kprint.h"
#include "serial.h"

int kputchar(int c)
{
    return serial_putchar(c);
}

int kputs(const char *s)
{
    while (*s)
    {
        kputchar(*s++);
    }

    return 0;
}

