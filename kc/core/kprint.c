#include "kprint.h"
#include "serial.h"

#include <kc/lib.h>

#include <stdint.h>

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

static char *convert_scalar(
        uint64_t value,
        unsigned base, 
        char *buffer, 
        size_t bufsz)
{
    static const char *stringdigits = "0123456789abcdef";

    // the string will be built from the lower-to-higher value, and the
    // result pointer will point to the first character of the string in
    // the supplied buffer

    // gotta zero the buffer
    memset(buffer, 0, bufsz);

    // the string is being built backwards, so the pointer needs to be
    // at the last byte of the string
    char *result = buffer + bufsz - 1;

    // result >= buffer condition ensures we don't overflow
    
    while (value > 0 && result >= buffer)
    {
        *--result = stringdigits[value % base];
        value /= base;
    }

    return result;
}

int kvprintf(const char *restrict format, va_list arguments)
{
    int count = 0;

    while (*format)
    {
        if (*format != '%')
        {
            kputchar(*format++);
            count++;
            continue;
        }

        switch (*++format)
        {
            case 0:
                return count;
            case '%':
                count++;
                kputchar('%');
                break;
            case 's':
                {
                    const char *s = va_arg(arguments, const char *);
                    count += strlen(s);
                    kputs(s);
                    break;
                }

            case 'x':
                {
                    char buffer[65];
                    char *s = convert_scalar(
                            va_arg(arguments, uint64_t),
                            16,
                            buffer,
                            sizeof(buffer));
                    count += strlen(s);
                    kputs(s);
                    break;
                }
            case 'd':
                {
                    char buffer[65];
                    char *s = convert_scalar(
                            va_arg(arguments, uint64_t),
                            10,
                            buffer,
                            sizeof(buffer));
                    count += strlen(s);
                    kputs(s);
                    break;
                }
            case 'o':
                {
                    char buffer[65];
                    char *s = convert_scalar(
                            va_arg(arguments, uint64_t),
                            8,
                            buffer,
                            sizeof(buffer));
                    count += strlen(s);
                    kputs(s);
                    break;
                }
            case 'b':
                {
                    char buffer[65];
                    char *s = convert_scalar(
                            va_arg(arguments, uint64_t),
                            2,
                            buffer,
                            sizeof(buffer));
                    count += strlen(s);
                    kputs(s);
                    break;
                }

            default:
                break;
        }

        format++;
    }

    va_end(arguments);
    return count;
}

int kprintf(const char *restrict format, ...)
{
    va_list arguments;
    va_start (arguments, format);

    return kvprintf(format, arguments);
}

