#include "kprint.h"
#include "serial.h"

#include <sophialib.h>

#include <stdint.h>
#include <stdbool.h>

FILE *kstdout;

int kfputc(int c, FILE *f)
{
    (void)f;
    return serial_putchar(c);
}

