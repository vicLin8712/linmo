/* libc: string-to-number conversion functions. */

#include <lib/libc.h>

/* Base-10 string conversion without division or multiplication */
static char *__str_base10(uint32_t value, char *buffer, int *length)
{
    if (value == 0) {
        buffer[0] = '0';
        *length = 1;
        return buffer;
    }
    int pos = 0;

    while (value > 0) {
        uint32_t q, r, t;

        q = (value >> 1) + (value >> 2);
        q += (q >> 4);
        q += (q >> 8);
        q += (q >> 16);
        q >>= 3;
        r = value - (((q << 2) + q) << 1);
        t = ((r + 6) >> 4);
        q += t;
        r -= (((t << 2) + t) << 1);

        buffer[pos++] = '0' + r;
        value = q;
    }
    *length = pos;

    return buffer;
}

/* Handle signed integers */
static char *__str_base10_signed(int32_t value, char *buffer, int *length)
{
    if (value < 0) {
        buffer[0] = '-';
        __str_base10((uint32_t) (-value), buffer + 1, length);
        (*length)++;
        return buffer;
    }
    return __str_base10((uint32_t) value, buffer, length);
}

/* Converts string @s to an integer. */
int32_t strtol(const char *s, char **end, int32_t base)
{
    int32_t i;
    uint32_t ch, value = 0, neg = 0;

    /* Handle optional sign. */
    if (s[0] == '-') {
        neg = 1;
        ++s;
    }

    /* Handle common base prefixes (0x for hex). */
    if (s[0] == '0' && s[1] == 'x') {
        base = 16;
        s += 2;
    }

    /* Convert digits based on the specified base. */
    for (i = 0; i <= 8; ++i) {
        ch = *s++;
        if ('0' <= ch && ch <= '9')
            ch -= '0';
        else if ('A' <= ch && ch <= 'Z')
            ch = ch - 'A' + 10;
        else if ('a' <= ch && ch <= 'z')
            ch = ch - 'a' + 10;
        else
            break;
        value = value * base + ch;
    }

    if (end)
        *end = (char *) s - 1;
    if (neg)
        value = -(int32_t) value;

    return value;
}

/* Converts string @s to an integer. */
int32_t atoi(const char *s)
{
    int32_t n, f;

    n = 0;
    f = 0; /* Flag for sign. */

    /* Skip leading whitespace and handle optional sign. */
    for (;; s++) {
        switch (*s) {
        case ' ':
        case '\t':
        case '\n':
        case '\r':
            continue; /* Skip whitespace. */
        case '-':
            f++; /* Set negative flag. */
            __attribute__((fallthrough));
        case '+':
            s++; /* Skip '+' sign. */
        }
        break;
    }

    /* Convert digits to integer. */
    while (*s >= '0' && *s <= '9')
        n = n * 10 + *s++ - '0';

    return (f ? -n : n);
}

/* Converts integer @i to an ASCII string @s in the given @base. */
void itoa(int32_t i, char *s, int32_t base)
{
    char c;
    char *p = s;
    char *q = s;
    uint32_t h;
    int32_t len;

    if (base == 16) { /* Hexadecimal conversion */
        h = (uint32_t) i;
        do {
            *q++ = '0' + (h % base);
        } while (h /= base); /* Continue until number becomes 0. */

        if ((i >= 0) && (i < 16)) /* Special case for small positive numbers */
            *q++ = '0';

        /* Reverse the string (digits are collected in reverse order). */
        for (*q = 0; p <= --q; p++) {
            /* Convert digit character if needed (e.g., 'a'-'f'). */
            (*p > '9') ? (c = *p + 39) : (c = *p); /* ASCII 'a' is '0'+39 */
            /* Swap characters. */
            (*q > '9') ? (*p = *q + 39) : (*p = *q);
            *q = c;
        }
    } else if (base == 10) { /* Decimal conversion */
        __str_base10_signed(i, s, &len);

        /* Reverse the string. */
        q = s + len;
        for (*q = 0; p <= --q; p++) {
            c = *p;
            *p = *q;
            *q = c;
        }
    } else { /* Other bases */
        if (i >= 0) {
            do {
                *q++ = '0' + (i % base);
            } while (i /= base);
        } else {
            *q++ = '-';
            p++;
            do {
                *q++ = '0' - (i % base);
            } while (i /= base);
        }

        /* Reverse the string. */
        for (*q = 0; p <= --q; p++) {
            c = *p;
            *p = *q;
            *q = c;
        }
    }
}
