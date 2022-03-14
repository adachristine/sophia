#include "kprint.h"
#include "serial.h"

#include <lib.h>

#include <stdint.h>
#include <stdbool.h>

int kputchar(int c)
{
    return serial_putchar(c);
}

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
    SIGNED_TYPE_FLAG = 0x8,
    SIGN_FLAG_MASK = 0x6,
    ALTERNATE_FORM_FLAG = 0x10,
    ZERO_PAD_FLAG = 0x20,
};

enum specifier_integer_base
{
    INVALID_BASE,
    BIN_BASE = 2,
    OCT_BASE = 8,
    DEC_BASE = 10,
    HEX_BASE = 16
};

enum specifier_integer_width
{
    INVALID_WIDTH,
    BYTE_WIDTH = 8,
    SHORT_WIDTH = 16,
    INT_WIDTH = 32,
    LONG_WIDTH = 64
};

struct method
{
    int (*write_character)(struct method *m, char c);
    int (*write_string)(struct method *m, const char *s);
    int count;
};

struct specifier
{
    enum specifier_type type;
    enum specifier_flags flags;
    enum specifier_integer_width integer_width;
    enum specifier_integer_base integer_base;
    int field_width;
    int field_precision;
    size_t length;
};

static struct specifier parse_specifier(const char *format, va_list *arguments)
{
    struct specifier result = 
    {
        INVALID_PRINT,
        INVALID_FLAGS,
        INVALID_WIDTH,
        INVALID_BASE,
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
    bool field_width_parsed = false;
    bool field_precision_parsed = false;

    while (!field_width_parsed || !field_precision_parsed)
    {
        switch (*begin)
        {
            case '.':
                if (!field_width_parsed)
                {
                    result.field_width = 0;
                    field_width_parsed = true;
                    begin++;
                }
                else
                {
                    // the specifier is invalid!
                    result.type = INVALID_PRINT;
                }
                break;
            case '*':
                {
                    int w = *va_arg(*arguments, int *);
                    if (!field_width_parsed)
                    {
                        result.field_width = w;
                        field_width_parsed = true;
                    }
                    else
                    {
                        result.field_precision = w;
                        field_precision_parsed = true;
                    }
                    begin++;
                }
                break;
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case '9':
            case '0':
                {
                    char *next;
                    unsigned long long w = strtoull(begin, &next, 10);
                    if (!field_width_parsed)
                    {
                        result.field_width = (int)w;
                        field_width_parsed = true;
                    }
                    else if (!field_precision_parsed)
                    {
                        result.field_precision = (int)w;
                        field_precision_parsed = true;
                    }
                    if (next > begin)
                    {
                        begin = next;
                    }
                }
                break;
            default:
                field_width_parsed = true;
                field_precision_parsed = true;
        }
    }

    // step 3: check for type width arguments
    switch (*begin)
    {
        case 0: // unexpected eos
            result.type = INVALID_PRINT;
            return result;
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
            result.flags |= SIGNED_TYPE_FLAG;
            result.integer_base = DEC_BASE;
            break;
        case 'u':
            result.type = INTEGER_PRINT;
            result.integer_base = DEC_BASE;
            break;
        case 'b':
            result.type = INTEGER_PRINT;
            result.integer_base = BIN_BASE;
            break;
        case 'o':
            result.type = INTEGER_PRINT;
            result.integer_base = OCT_BASE;
            break;
        case 'x':
            result.type = INTEGER_PRINT;
            result.integer_base = HEX_BASE;
            break;
        case 'p':
            // TODO: use UINTPTR_T_WIDTH here?
            // pointer type overrides all flags
            // i can do what i want it says "implementation-defined" in the spec
            result.field_precision = 16;
            result.type = INTEGER_PRINT;
            result.integer_base = HEX_BASE;
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
        int zpadding,
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

    // result >= buffer condition ensures we don't underflow 

    do
    {
        *--result = stringdigits[value % base];
        zpadding--;
        value /= base;
    } 
    while (value > 0 && result >= buffer);

    while (zpadding-- > 0 && result >= buffer)
    {
        *--result = '0';
    }

    return result;
}

static int print_character(
        struct method *m,
        struct specifier *spec,
        va_list *arguments)
{
    (void)spec;
    // all specifier flags and etc. are ignored.
    return m->write_character(m, va_arg(*arguments, int));
}

static int print_string(
        struct method *m,
        struct specifier *spec,
        va_list *arguments)
{
    (void)spec;
    // TODO: respect field width
    return m->write_string(m, va_arg(*arguments, const char *));
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

static int print_integer(
        struct method *m, 
        struct specifier *spec, 
        va_list *arguments)
{
    // 65 bytes is the maximum length that convert_integer will need
    // i.e. conversion of uintmax_t to binary plus NUL terminator
    // TODO: use UINTMAX_T_WIDTH + 1?
    char buffer[65] = {0};
    char *s;
    bool negative = false;
    uint64_t value = 0;
    int r = 0;
    unsigned zpad = 0;

    switch (spec->integer_width)
    {
        case BYTE_WIDTH:
        case SHORT_WIDTH:
        case INT_WIDTH:
            value = va_arg(*arguments, unsigned int);
            break;
        case LONG_WIDTH:
            value = va_arg(*arguments, uint64_t);
            break;
        default:
            return -1;
    }

    // check if we need to bother with signs
    if (spec->flags & SIGNED_TYPE_FLAG)
    {
        if ((negative = is_negative(value, spec->integer_width)))
        {
            value = ~value + 1;
        }

        if (negative)
        {
            r = m->write_character(m, '-');
        }

        else if (!negative && (spec->flags & SIGN_FLAG_MASK))
        {
            if (spec->flags & EXPLICIT_SIGN_FLAG)
            {
                r = m->write_character(m, '+');
            }
            else
            {
                r = m->write_character(m, ' ');
            }
        }
    }

    // zero all the unnecessary bits
    value &= (2ULL << (spec->integer_width - 1)) - 1;

    switch (spec->integer_base)
    {
        case BIN_BASE:
            if (spec->flags & ALTERNATE_FORM_FLAG)
            {
                r = m->write_string(m, "0b");
            }
            break;
        case OCT_BASE:
            if (spec->flags & ALTERNATE_FORM_FLAG)
            {
                r = m->write_character(m, '0');
            }
            break;
        case HEX_BASE:
            if (spec->flags & ALTERNATE_FORM_FLAG)
            {
                r = m->write_string(m, "0x");
            }
            break;
        default:
            break;
    }

    if (spec->flags & ZERO_PAD_FLAG)
    {
        zpad = spec->field_precision;
    }
    s = convert_integer(value, spec->integer_base, zpad, buffer, sizeof(buffer));

    if (s)
    {
        m->write_string(m, s);
    }
    else
    {
        m->write_string(m, "(INVALID)");
        r = -1;
    }

    return r;
}

static int printf_internal(
        struct method *m,
        const char *restrict format,
        va_list *arguments)
{
    int r = 0;

    while (*format && !r)
    {
        // case 1: not a format specification
        if (*format != '%')
        {
            r = m->write_character(m, *format++);
            continue;
        }

        // case 2: looks like a format specification, but isn't
        else if (*format == '%' && format[0] == format[1])
        {
            r = m->write_character(m, '%');
            format += 2;
            continue;
        }

        // case 3: is a format specification. parse it
        struct specifier spec = parse_specifier(++format, arguments);

        switch (spec.type)
        {
            case CHARACTER_PRINT:
                r = print_character(m, &spec, arguments);
                break;
            case STRING_PRINT:
                r = print_string(m, &spec, arguments);
                break;
            case INTEGER_PRINT:
                r = print_integer(m, &spec, arguments);
                break;
            default:
                r = m->write_string(m, "(INVALID)");
                return -1;
        }
        format += spec.length;
    }

    return r;
}

static int kpm_write_character(struct method *m, char c)
{
    kputchar(c);
    m->count++;
    return 0;
}

static int kpm_write_string(struct method *m, const char *c)
{
    m->count += kputs(c);
    return 0;
}

int kvprintf(const char *restrict format, va_list arguments)
{
    struct method m = {
        kpm_write_character,
        kpm_write_string,
        0};
    va_list acopy;
    va_copy(acopy, arguments);
    int r = printf_internal(&m, format, &acopy);
    if (!r)
    {
        return m.count;
    }
    else
    {
        return -1;
    }
    va_end(acopy);
}

int kprintf(const char *restrict format, ...)
{
    va_list arguments;
    va_start (arguments, format);

    int count = kvprintf(format, arguments);

    va_end(arguments);

    return count;
}

