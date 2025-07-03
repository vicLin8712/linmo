/* libc: String manipulation functions. */

#include <lib/libc.h>

#include "private/utils.h"

/* Checks for any zero byte in a 32-bit word. */
static inline int byte_is_zero(uint32_t v)
{
    /* bitwise check for zero bytes. */
    return ((v - 0x01010101u) & ~v & 0x80808080u) != 0;
}

/* Checks if a 32-bit word @w matches the pattern @pat in all bytes. */
static inline int byte_is_match(uint32_t w, uint32_t pat)
{
    uint32_t t = w ^ pat; /* t will be zero for matching bytes. */
    /* Similar logic to byte_is_zero, but applied to the XORed result. */
    return ((t - 0x01010101u) & ~t & 0x80808080u) != 0;
}

/* strlen that scans by words whenever possible for efficiency. */
size_t strlen(const char *s)
{
    const char *p = s;

    /* Align pointer to word boundary (4 bytes) */
    while ((uint32_t) p & 3) {
        if (!*p) /* If null terminator is found byte-by-byte */
            return (size_t) (p - s);
        p++;
    }

    /* Word scan: Iterate through 32-bit words as long as no byte is zero. */
    const uint32_t *w = (const uint32_t *) p;
    while (!byte_is_zero(*w))
        w++;

    /* Final byte scan: Within the word that contained the zero byte, find the
     * exact position.
     */
    p = (const char *) w;
    while (*p) /* Scan byte-by-byte until the null terminator. */
        p++;
    return (size_t) (p - s); /* Return total length. */
}

char *strcpy(char *dst, const char *src)
{
    char *ret = dst;
    while ((*dst++ = *src++) != 0)
        ;
    return ret;
}

char *strncpy(char *dst, const char *src, int32_t n)
{
    char *ret = dst;
    /* Copy up to @n characters or until the null terminator. */
    while (n && (*dst++ = *src++) != 0)
        n--;

    /* Pad with null bytes if @n is not exhausted and source was shorter. */
    while (n--)
        *dst++ = 0;

    return ret;
}

char *strcat(char *dst, const char *src)
{
    char *ret = dst;
    /* Advance to the end of the destination string. */
    while (*dst)
        dst++;

    /* Append the source string. */
    while ((*dst++ = *src++) != 0)
        ;

    return ret;
}

char *strncat(char *dst, const char *src, int32_t n)
{
    char *ret = dst;

    /* Advance to the end of the destination string. */
    while (*dst)
        dst++;

    /* Copy up to @n bytes from src, or until a null terminator is found. */
    while (n-- && (*dst = *src++) != 0)
        dst++;

    /* Ensure the result is null-terminated. */
    *dst = 0;
    return ret;
}

/* Check if two words are equal
 * Return true if XOR is zero (all bytes match)
 */
static inline int equal_word(uint32_t a, uint32_t b)
{
    return (a ^ b) == 0;
}

/* Word-oriented string comparison. */
int32_t strcmp(const char *s1, const char *s2)
{
    /* Align pointers to word boundary */
    while (((uint32_t) s1 & 3) && *s1 && *s1 == *s2) {
        s1++;
        s2++;
    }

    /* If alignment causes one to hit null or inequality first */
    if (((uint32_t) s1 & 3) == ((uint32_t) s2 & 3)) {
        const uint32_t *w1 = (const uint32_t *) s1;
        const uint32_t *w2 = (const uint32_t *) s2;

        /* Word comparison loop */
        for (;; ++w1, ++w2) {
            uint32_t v1 = *w1;
            uint32_t v2 = *w2;

            /* Exit if words differ or if a zero byte is found in either word */
            if (!equal_word(v1, v2) || byte_is_zero(v1)) {
                s1 = (const char *) w1;
                s2 = (const char *) w2;
                break;
            }
        }
    }

    /* Final byte comparison until null terminator or difference */
    while (*s1 && *s1 == *s2) {
        s1++;
        s2++;
    }

    /* Return the difference between the first differing bytes */
    return (int32_t) ((unsigned char) *s1 - (unsigned char) *s2);
}

/* Word-oriented string comparison, up to 'n' characters. */
int32_t strncmp(const char *s1, const char *s2, int32_t n)
{
    if (n == 0) /* If n is 0, strings are considered equal. */
        return 0;

    /* Align pointers to word boundary */
    while (((uint32_t) s1 & 3) && *s1 && *s1 == *s2) {
        s1++;
        s2++;
        n--;
    }

    /* If alignment causes one to hit null or inequality, or n becomes 0 */
    if (n == 0 || *s1 != *s2)
        goto tail;

    /* Word comparison loop */
    if (((uint32_t) s1 & 3) == ((uint32_t) s2 & 3)) {
        const uint32_t *w1 = (const uint32_t *) s1;
        const uint32_t *w2 = (const uint32_t *) s2;

        /* Compare words as long as n >= 4 and bytes within words match */
        while (n >= 4) {
            uint32_t v1 = *w1;
            uint32_t v2 = *w2;

            /* Exit if words differ or if a zero byte is found */
            if (!equal_word(v1, v2) || byte_is_zero(v1))
                break;

            w1++;
            w2++;
            n -= 4;
        }
        s1 = (const char *) w1;
        s2 = (const char *) w2;
    }

tail: /* Fallback for byte comparison or if word comparison was skipped */
    while (n && *s1 && *s1 == *s2) {
        s1++;
        s2++;
        n--;
    }

    /* Return difference, or 0 if n reached 0 */
    return n ? (int32_t) ((unsigned char) *s1 - (unsigned char) *s2) : 0;
}

/* Locates the first occurrence of a character 'c' in string 's'. */
char *strchr(const char *s, int32_t c)
{
    uint8_t ch = (uint8_t) c;
    /* Create a 32-bit pattern for byte matching */
    uint32_t pat = 0x01010101u * ch;

    /* Byte-by-byte scan until word-aligned */
    while (((uint32_t) s & 3)) {
        if (*s == ch || *s == 0) /* Found char or end of string */
            return (*s == ch) ? (char *) s : 0;
        s++;
    }

    /* Word scan: Iterate through words, checking for character match or zero
     * byte.
     */
    const uint32_t *w = (const uint32_t *) s;
    for (;; ++w) {
        uint32_t v = *w;
        /* Exit if word contains zero or matches the pattern */
        if (byte_is_zero(v) || byte_is_match(v, pat)) {
            s = (const char *) w;  /* Reset to start of word */
            while (*s && *s != ch) /* Byte scan within the word */
                s++;

            return (*s == ch) ? (char *) s : 0; /* Return if found */
        }
    }
}

/* Locates the first occurrence of any character from @set in string @s. */
char *strpbrk(const char *s, const char *set)
{
    /* Build a 256-bit bitmap (eight 32-bit words) for characters in 'set'. */
    uint32_t map[8] = {0}; /* Initialize bitmap to all zeros. */
    while (*set) {
        uint8_t ch = (uint8_t) *set++;
        map[ch >> 5] |=
            1u << (ch & 31); /* Set the bit corresponding to the character. */
    }

    /* Scan the string @s */
    while (*s) {
        uint8_t ch = (uint8_t) *s;
        /* Check if the character's bit is set in the bitmap. */
        if (map[ch >> 5] & (1u << (ch & 31)))
            return (char *) s; /* Found a character from @set. */
        s++;
    }

    return 0;
}

/* Splits string 's' into tokens by delimiters in @delim. */
char *strsep(char **pp, const char *delim)
{
    char *p = *pp;
    if (!p)
        return 0;

    /* Find the first delimiter character in the current string segment. */
    char *q = strpbrk(p, delim);
    if (q) {
        *q = 0;
        *pp = q + 1;
    } else
        *pp = 0;
    return p;
}

/* Classic non-re-entrant tokenizer. Uses a static buffer for state. */
char *strtok(char *s, const char *delim)
{
    static char *last;
    if (s == 0)
        s = last;
    if (!s)
        return 0;

    /* Skip leading delimiters. */
    while (*s && strpbrk(&*s, delim) == &*s)
        *s++ = 0;
    if (*s == 0) {
        last = 0;
        return 0;
    }

    char *tok = s;
    /* Advance to next delimiter or end of string. */
    while (*s && !strpbrk(&*s, delim))
        s++;
    if (*s) {
        *s++ = 0;
        last = s;
    } else
        last = 0;
    return tok;
}

/* Re-entrant version of strtok. */
char *strtok_r(char *s, const char *delim, char **save)
{
    if (s == 0)
        s = *save;
    if (!s)
        return 0;

    /* Skip leading delimiters. */
    while (*s && strpbrk(&*s, delim) == &*s)
        *s++ = 0;
    if (*s == 0) {
        *save = 0;
        return 0;
    }

    char *tok = s;
    /* Advance to the next delimiter or end of string. */
    while (*s && !strpbrk(&*s, delim))
        s++;
    if (*s) {
        *s++ = 0;
        *save = s;
    } else
        *save = 0;
    return tok;
}

/* Locates the first occurrence of substring @needle in string @haystack. */
char *strstr(const char *haystack, const char *needle)
{
    if (!*needle) /* Empty needle matches any string */
        return (char *) haystack;

    const char *h, *n;
    char first = *needle;

    /* Search for the first character of needle */
    while (*haystack) {
        if (*haystack == first) {
            /* Found potential match, check the rest */
            h = haystack;
            n = needle;

            while (*h && *n && *h == *n) {
                h++;
                n++;
            }

            if (!*n) /* Reached end of needle - full match found */
                return (char *) haystack;
        }
        haystack++;
    }

    return NULL; /* No match found */
}
