#include "serial.h"

#include <stdint.h>
#include <stdbool.h>

#include <lib/kstdio.h>
#include <lib/kstring.h>

FILE *kstdout;

int kfputc(int c, FILE *f)
{
    (void)f;
    return serial_putchar(c);
}

