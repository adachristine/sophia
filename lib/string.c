#include <lib/kstring.h>
#include <stdbool.h>

size_t strlen(const char *s)
{
    size_t l = 0;

    while (*s++) l++;

    return l;
}

static long long valueof(int c, int base)
{
    long long result = 0;

    if (base < 2 || base > 36)
    {
        // bases less than 2 or greater than 36 are invalid.
        return -1;
    }

    // check if value is an ascii numeric character
    if (c >= '0' && c <= '9')
    {
        // simple as
        result = (long long)(c ^ 0x30);
    }

    // check if value is an uppercase ASCII alphabetical character
    else if (
            (c >= 'A' && c <= 'Z') ||
            (c >= 'a' && c <= 'z'))
    {
        // mask in uppercase bit
        c |= 0x20;
        // value is 10 plus the alphabetical order of the character
        result = (long long)(10 + c - 'a');
    }
    else
    {
        // character has no valid interpretation under base 36
        return -1LL;
    }

    if (result > (base - 1))
    {
        // character is not valid under given base
        return -1LL;
    }

    return result;
}

unsigned long long strtoull(
        const char *restrict begin,
        char **restrict end,
        int base)
{
    long result = 0;
    const char *current = begin;

    // check for empty string
    if (*current == 0)
    {
        if (end)
        {
            *end = (char *)begin;
            return result;
        }
    }

    // skip initial whitespace
    bool whitespace = true;
    while (whitespace)
    {
        switch (*current)
        {
            case 0x9:
            case 0xa:
            case 0xb:
            case 0xc:
            case 0xd:
            case 0x20:
                current++;
                break;
            default:
                whitespace = false;
        }
    }

    bool negative = false;
    // detect if negative sign is used
    if (*current == '-')
    {
        negative = true;
        current++;
    }

    // detect base from input
    if (base == 0)
    {
        // base is non-decimal
        if (current[0] == 0)
        {
            if (current[1] == 'b')
            {
                base = 2;
                current += 2;
            }
            else if (current[1] == 'x')
            {
                base = 16;
                current += 2;
            }
            else
            {
                base = 8;
                current++;
            }
        }
        // base is decimal otherwise
        else
        {
            base = 10;
        }
    }
    // do the actual conversion now
    while (*current)
    {
        long long value = valueof(*current, base);

        // we've reached the end of the conversion
        if (value == -1)
        {
            break;
        }

        result *= base;
        result += value;
        current++;
    }

    // store the end pointer if needed
    if (end)
    {
        *end = (char *)current;
    }

    // flip the result if we're meant to
    if (negative)
    {
        result = -result;
    }

    return result;
}

