#include "kprint.h"
#include "serial.h"

#include <kc/lib.h>

#include <stdint.h>
#include <stdbool.h>

    __attribute__((optimize("O3")))
int kputchar(int c)
{
    return serial_putchar(c);
}

    __attribute__((optimize("O3")))
int kputs(const char *s)
{
    int count = 0;
    while (*s)
    {
        count++;
        kputchar(*s++);
    }

    return count;
}

enum specifier_type
{
    INVALID_PRINT,
    CHARACTER_PRINT,
    STRING_PRINT,
    INTEGER_PRINT,
    COUNT_PRINT
};

enum specifier_flags
{
    INVALID_FLAGS,
    LEFT_JUSTIFY_FLAG = 0x1,
    EXPLICIT_SIGN_FLAG = 0x2,
    PAD_SIGN_FLAG = 0x4,
    SIGN_FLAG_MASK = 0x6,
    ALTERNATE_FORM_FLAG = 0x8,
    ZERO_PAD_FLAG = 0x10,
};

enum specifier_integer_format
{
    INVALID_FORMAT,
    SIGNED_DEC_FORMAT,
    UNSIGNED_DEC_FORMAT,
    BIN_FORMAT,
    OCT_FORMAT,
    HEX_FORMAT
};

enum specifier_integer_width
{
    INVALID_WIDTH,
    BYTE_WIDTH = 8,
    SHORT_WIDTH = 16,
    INT_WIDTH = 32,
    LONG_WIDTH = 64
};

struct specifier
{
    enum specifier_type type;
    enum specifier_flags flags;
    enum specifier_integer_width integer_width;
    enum specifier_integer_format integer_format;
    int field_width;
    int field_precision;
    size_t length;
};

static struct specifier parse_specifier(const char *format, va_list arguments)
{
    (void)arguments;
    struct specifier result = 
    {
        INVALID_PRINT,
        INVALID_FLAGS,
        INVALID_WIDTH,
        INVALID_FORMAT,
        0,
        0,
        0
    };

    // keep a pointer to the beginning of the specifier
    const char *begin = format;

    // step 1: scan for flags
    bool flags_parsed = false;

    while (!flags_parsed)
    {
        switch (*begin)
        {
            case 0:
                result.type = INVALID_PRINT;
                return result;
            case '-':
                result.flags |= LEFT_JUSTIFY_FLAG;
                begin++;
                break;
            case '+':
                result.flags |= EXPLICIT_SIGN_FLAG;
                begin++;
                break;
            case ' ':
                result.flags |= PAD_SIGN_FLAG;
                begin++;
                break;
            case '#':
                result.flags |= ALTERNATE_FORM_FLAG;
                begin++;
                break;
            case '0':
                result.flags |= ZERO_PAD_FLAG;
                begin++;
                break;
            default:
                flags_parsed = true;
        }
    }

    // step 2: parse field width and precision
    // TODO: parse field width and precision
    /* procedure: 
     * 1. check if next character is * or .
     *   a. if *, field width is the value pointed to by the next item in
     *      arguments list
     *   b. if ., field width will be set to 0, proceed to 3.
     * 2. if above check is false, check for numeric character
     *   a. if is numeric character, parse field width via strtoul, add length
     *      of numeric string to begin.
     *   b. if is not a numeric character, skip check for width and precision 
     *      entirely
     * 3. check if next character is * or numeric
     *   a. if *, field precision is the value pointed to by the next item
     *      in arguments list.
     *   b. if is numeric character, parse field precision via strtoul, add
     *      length of numeric string to begin.
     */

    // step 3: check for type width arguments
    switch (*begin)
    {
        case 'h':
            {
                if (begin[0] == begin[1])
                {
                    result.integer_width = BYTE_WIDTH;
                    begin += 2;
                }
                else
                {
                    result.integer_width = SHORT_WIDTH;
                    begin++;
                }
                break;
            }
        case 'l':
            {
                // TODO: deal with LLP64? idk.
                if (begin[0] == begin[1])
                {
                    begin += 2;
                }
                else
                {
                    begin++;
                }
                result.integer_width = LONG_WIDTH;
                break;
            }
        case 'j':
            // TODO: use INTMAX_T_WIDTH
            result.integer_width = 64;
            begin++;
            break;
        case 'z':
            // TODO: use SIZE_T_WIDTH?
            result.integer_width = 64;
            begin++;
            break;
        case 't':
            // TODO: use PTRDIFF_T_WIDTH?
            result.integer_width = 32;
            begin++;
            break;
        default:
            // there is no width argument to be found.
            result.integer_width = INT_WIDTH;
            break;
    }

    // step 4: parse field type
    switch (*begin)
    {
        // unexpected EOS
        case 0:
            result.type = INVALID_PRINT;
            return result;
        case 'c':
            result.type = CHARACTER_PRINT;
            break;
        case 's':
            result.type = STRING_PRINT;
            break;
        case 'd':
        case 'i':
            result.type = INTEGER_PRINT;
            result.integer_format = SIGNED_DEC_FORMAT;
            break;
        case 'u':
            result.type = INTEGER_PRINT;
            result.integer_format = UNSIGNED_DEC_FORMAT;
            break;
        case 'b':
            result.type = INTEGER_PRINT;
            result.integer_format = BIN_FORMAT;
            break;
        case 'o':
            result.type = INTEGER_PRINT;
            result.integer_format = OCT_FORMAT;
            break;
        case 'x':
            result.type = INTEGER_PRINT;
            result.integer_format = HEX_FORMAT;
            break;
        case 'p':
            // TODO: use UINTPTR_T_WIDTH here?
            // pointer type overrides all flags
            // i can do what i want it says "implementation-defined" in the spec
            result.field_precision = 8;
            result.type = INTEGER_PRINT;
            result.integer_format = HEX_FORMAT;
            result.integer_width = LONG_WIDTH;
            result.flags = ALTERNATE_FORM_FLAG|ZERO_PAD_FLAG;
            break;
        case 'n':
            // all flags are invalid/ignored and the current count will be
            // stored in the value pointed to by the argument
            result.type = COUNT_PRINT;
            result.flags = INVALID_FLAGS;
            result.integer_width = INVALID_WIDTH;
            result.field_width = 0;
            result.field_precision = 0;
            break;
        default:
            // this byte of the specifier _must_ be valid. if not,
            // the procedure to print should not proceed as it might
            // output garbage.
            // TODO: specify somehow in the output that the format is bad?
            result.type = INVALID_PRINT;
            return result;
    }

    begin++;

    result.length = begin - format;
    return result;
}

static char *convert_integer(
        uint64_t value,
        unsigned base, 
        char *buffer, 
        size_t bufsz)
{
    static const char *stringdigits = "0123456789abcdef";

    // the string will be built from the lower-to-higher value, and the
    // result pointer will point to the first character of the string in
    // the supplied buffer
    
    // cannot currently work with a base > 16
    if (base > 16)
    {
        return NULL;
    }

    // make absolutely sure there are no excess bits

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

static int print_character(struct specifier *spec, va_list arguments)
{
    (void)spec;
    // all specifier flags and etc. are ignored.
    kputchar(va_arg(arguments, int));
    return 1;
}

static int print_string(struct specifier *spec, va_list arguments)
{
    (void)spec;
    // TODO: respect field width
    const char *s = va_arg(arguments, const char *);
    return kputs(s);
}

static inline bool is_negative(uint64_t value, unsigned width)
{
    switch (width)
    {
        case BYTE_WIDTH:
            return ((int8_t)value) < 0;
        case SHORT_WIDTH:
            return ((int16_t)value) < 0;
        case INT_WIDTH:
            return ((int32_t)value) < 0;
        case LONG_WIDTH:
            return ((int64_t)value) < 0;
        default:
            return false;
    }
}

static int print_integer(struct specifier *spec, va_list arguments)
{
    // 65 bytes is the maximum length that convert_integer will need
    // i.e. conversion of uintmax_t to binary plus NUL terminator
    // TODO: use UINTMAX_T_WIDTH + 1?
    char buffer[65] = {0};
    char *s;
    int count = 0;
    bool negative = false;
    uint64_t value = va_arg(arguments, uint64_t);
    unsigned base = 10;

    // check if we need to bother with signs
    if (spec->integer_format == SIGNED_DEC_FORMAT)
    {
        if ((negative = is_negative(value, spec->integer_width)))
        {
            value = ~value + 1;
        }

        if (negative)
        {
            kputchar('-');
            count++;
        }

        else if (!negative && (spec->flags & SIGN_FLAG_MASK))
        {
            if (spec->flags & EXPLICIT_SIGN_FLAG)
            {
                kputchar('+');
            }
            else
            {
                kputchar(' ');
            }
            count++;
        }
    }

    // zero all the unnecessary bits
    value &= (2ULL << (spec->integer_width - 1)) - 1;

    switch (spec->integer_format)
    {
        case BIN_FORMAT:
            if (spec->flags & ALTERNATE_FORM_FLAG)
            {
                kputs("0b");
                count += 2;
            }
            base = 2;
            break;
        case OCT_FORMAT:
            if (spec->flags & ALTERNATE_FORM_FLAG)
            {
                kputchar('0');
                count++;
            }
            base = 8;
            break;
        case HEX_FORMAT:
            if (spec->flags & ALTERNATE_FORM_FLAG)
            {
                kputs("0x");
                count += 2;
            }
            base = 16;
            break;
        default:
            base = 10;
    }

    s = convert_integer(value, base, buffer, sizeof(buffer));

    if (s)
    {
        count += kputs(s);
    }
    else
    {
        kputs("(INVALID)");
    }

    return count;
}

int kvprintf(const char *restrict format, va_list arguments)
{
    int count = 0;

    while (*format)
    {
        // case 1: not a format specification
        if (*format != '%')
        {
            kputchar(*format++);
            count++;
            continue;
        }

        // case 2: looks like a format specification, but isn't
        else if (*format == '%' && format[0] == format[1])
        {
            kputchar('%');
            format += 2;
            count++;
            continue;
        }

        // case 3: is a format specification. parse it
        struct specifier spec = parse_specifier(++format, arguments);

        switch (spec.type)
        {
            case CHARACTER_PRINT:
                count += print_character(&spec, arguments);
                break;
            case STRING_PRINT:
                count += print_string(&spec, arguments);
                break;
            case INTEGER_PRINT:
                count += print_integer(&spec, arguments);
                break;
            default:
                kputs("(INVALID)");
                return -1;
        }
        format += spec.length;
    }

    return count;
}

int kprintf(const char *restrict format, ...)
{
    va_list arguments;
    va_start (arguments, format);

    int count = kvprintf(format, arguments);

    va_end(arguments);

    return count;
}
