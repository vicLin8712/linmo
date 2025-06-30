/* software implementation of 64-bit multiply/divide */

#include <types.h>
#include "private/utils.h"

/* 32-bit multiplication with overflow detection */
uint32_t __mulsi3(uint32_t a, uint32_t b)
{
    /* Early exit for common cases */
    if (unlikely(a == 0 || b == 0))
        return 0;
    if (unlikely(a == 1))
        return b;
    if (unlikely(b == 1))
        return a;

    uint32_t result = 0;

    /* Use the smaller operand as the multiplier for efficiency */
    if (a > b) {
        uint32_t temp = a;
        a = b;
        b = temp;
    }

    while (a) {
        if (a & 1)
            result += b;
        b <<= 1;
        a >>= 1;
    }
    return result;
}

/* 32x32 -> 64-bit multiplication */
uint64_t __muldsi3(uint32_t a, uint32_t b)
{
    /* Early exit optimizations */
    if (unlikely(a == 0 || b == 0))
        return 0;
    if (unlikely(a == 1))
        return b;
    if (unlikely(b == 1))
        return a;

    uint64_t result = 0;
    uint64_t aa = a;

    while (b) {
        if (b & 1)
            result += aa;
        aa <<= 1;
        b >>= 1;
    }

    return result;
}

/* 64x64 -> 64-bit multiplication using Karatsuba-like decomposition */
uint64_t __muldi3(uint64_t a, uint64_t b)
{
    /* Early exit for common cases */
    if (unlikely(a == 0 || b == 0))
        return 0;
    if (unlikely(a == 1))
        return b;
    if (unlikely(b == 1))
        return a;

    /* Split into 32-bit components */
    uint32_t al = (uint32_t) a, ah = (uint32_t) (a >> 32);
    uint32_t bl = (uint32_t) b, bh = (uint32_t) (b >> 32);

    /* Compute partial products */
    uint64_t low = __muldsi3(al, bl);
    uint64_t mid = __muldsi3(al, bh) + __muldsi3(ah, bl);

    /* Combine results (only lower 64 bits matter) */
    return low + (mid << 32);
}

/* Common division helper with comprehensive error handling */
uint32_t __udivmodsi4(uint32_t num, uint32_t den, int mod)
{
    /* Handle division by zero */
    if (unlikely(den == 0)) {
        /* Return maximum value for quotient, 0 for remainder */
        return mod ? 0 : UINT32_MAX;
    }

    /* Handle trivial cases for efficiency */
    if (unlikely(num < den))
        return mod ? num : 0;
    if (unlikely(num == den))
        return mod ? 0 : 1;
    if (unlikely(den == 1))
        return mod ? 0 : num;

    /* Check for power-of-2 divisor optimization */
    if ((den & (den - 1)) == 0) {
        /* den is a power of 2 */
        int shift = 0;
        uint32_t temp = den;
        while (temp > 1) {
            temp >>= 1;
            shift++;
        }
        return mod ? (num & (den - 1)) : (num >> shift);
    }

    uint32_t quot = 0, qbit = 1;

    /* Normalize divisor to avoid overflow */
    while ((int32_t) den >= 0) {
        den <<= 1;
        qbit <<= 1;
    }

    /* Long division algorithm */
    while (qbit) {
        if (num >= den) {
            num -= den;
            quot |= qbit;
        }
        den >>= 1;
        qbit >>= 1;
    }

    return mod ? num : quot;
}

/* Signed division with proper handling of edge cases */
int32_t __divmodsi4(int32_t num, int32_t den, int mod)
{
    /* Handle division by zero */
    if (unlikely(den == 0))
        return mod ? 0 : (num < 0 ? INT32_MIN : INT32_MAX);

    /* Handle overflow case: INT32_MIN / -1 */
    if (unlikely(num == INT32_MIN && den == -1)) {
        return mod ? 0
                   : INT32_MIN; /* Undefined behavior in C, but consistent */
    }

    /* Determine result sign */
    int neg = (num < 0) ^ (den < 0);
    int num_neg = (num < 0);

    /* Convert to unsigned for division */
    uint32_t unum = (num < 0) ? -(uint32_t) num : (uint32_t) num;
    uint32_t uden = (den < 0) ? -(uint32_t) den : (uint32_t) den;
    uint32_t res = __udivmodsi4(unum, uden, mod);

    /* Apply sign correction */
    if (mod) {
        /* Remainder has the same sign as dividend */
        return num_neg ? -(int32_t) res : (int32_t) res;
    } else {
        /* Quotient sign determined by operand signs */
        return neg ? -(int32_t) res : (int32_t) res;
    }
}

/* public division/modulo interfaces */
uint32_t __udivsi3(uint32_t num, uint32_t den)
{
    return __udivmodsi4(num, den, 0);
}

uint32_t __umodsi3(uint32_t num, uint32_t den)
{
    return __udivmodsi4(num, den, 1);
}

int32_t __divsi3(int32_t num, int32_t den)
{
    return __divmodsi4(num, den, 0);
}

int32_t __modsi3(int32_t num, int32_t den)
{
    return __divmodsi4(num, den, 1);
}

/* 64-bit left shift with bounds checking */
uint64_t __ashldi3(uint64_t val, int cnt)
{
    /* Handle edge cases */
    if (unlikely(cnt <= 0))
        return val;
    if (unlikely(cnt >= 64))
        return 0;

    if (cnt < 32)
        return val << cnt;
    /* Shift by 32 or more - high word becomes shifted low word */
    return ((uint64_t) (uint32_t) val) << (cnt - 32) << 32;
}

/* 64-bit arithmetic right shift with sign extension */
uint64_t __ashrdi3(uint64_t val, int cnt)
{
    /* Handle edge cases */
    if (unlikely(cnt <= 0))
        return val;
    if (unlikely(cnt >= 64)) {
        /* Fill with sign bit */
        return ((int64_t) val < 0) ? UINT64_MAX : 0;
    }

    /* Perform arithmetic shift */
    if (cnt < 32)
        return ((int64_t) val) >> cnt;
    /* Shift by 32 or more */
    int32_t high = (int32_t) (val >> 32);
    return ((int64_t) high) >> (cnt - 32);
}

/* 64-bit logical right shift */
uint64_t __lshrdi3(uint64_t val, int cnt)
{
    /* Handle edge cases */
    if (unlikely(cnt <= 0))
        return val;
    if (unlikely(cnt >= 64))
        return 0;

    if (cnt < 32)
        return val >> cnt;
    /* Shift by 32 or more */
    return (val >> 32) >> (cnt - 32);
}

/* 64-bit unsigned division with remainder - enhanced version */
uint64_t __udivmoddi4(uint64_t num, uint64_t den, uint64_t *rem)
{
    /* Handle division by zero */
    if (unlikely(den == 0)) {
        if (rem)
            *rem = 0;
        return UINT64_MAX;
    }

    /* Handle trivial cases */
    if (unlikely(num < den)) {
        if (rem)
            *rem = num;
        return 0;
    }
    if (unlikely(num == den)) {
        if (rem)
            *rem = 0;
        return 1;
    }
    if (unlikely(den == 1)) {
        if (rem)
            *rem = 0;
        return num;
    }

    /* Check for 32-bit divisors for optimization */
    if (den <= UINT32_MAX && num <= UINT32_MAX) {
        uint32_t q = __udivmodsi4((uint32_t) num, (uint32_t) den, 0);
        if (rem)
            *rem = (uint32_t) num - q * (uint32_t) den;
        return q;
    }

    uint64_t quot = 0, qbit = 1;

    /* Normalize divisor */
    while ((int64_t) den >= 0) {
        den <<= 1;
        qbit <<= 1;
    }

    /* Long division */
    while (qbit) {
        if (num >= den) {
            num -= den;
            quot |= qbit;
        }
        den >>= 1;
        qbit >>= 1;
    }

    if (rem)
        *rem = num;

    return quot;
}

/* 64-bit signed division with remainder */
int64_t __divmoddi4(int64_t num, int64_t den, int64_t *rem)
{
    /* Handle division by zero */
    if (unlikely(den == 0)) {
        if (rem)
            *rem = 0;
        return (num < 0) ? INT64_MIN : INT64_MAX;
    }

    /* Handle overflow case */
    if (unlikely(num == INT64_MIN && den == -1)) {
        if (rem)
            *rem = 0;
        return INT64_MIN;
    }

    /* Determine signs */
    int neg = (num < 0) ^ (den < 0);
    int num_neg = (num < 0);

    /* Convert to unsigned */
    uint64_t unum = (num < 0) ? -(uint64_t) num : (uint64_t) num;
    uint64_t uden = (den < 0) ? -(uint64_t) den : (uint64_t) den;
    uint64_t ures = __udivmoddi4(unum, uden, (uint64_t *) rem);

    /* Apply sign corrections */
    if (rem && num_neg)
        *rem = -(*rem);

    return neg ? -(int64_t) ures : (int64_t) ures;
}

/* Public 64-bit division/modulo interfaces */
uint64_t __umoddi3(uint64_t num, uint64_t den)
{
    uint64_t rem = 0;
    __udivmoddi4(num, den, &rem);
    return rem;
}

uint64_t __udivdi3(uint64_t num, uint64_t den)
{
    return __udivmoddi4(num, den, 0);
}

int64_t __moddi3(int64_t num, int64_t den)
{
    int64_t rem;
    __divmoddi4(num, den, &rem);
    return rem;
}

int64_t __divdi3(int64_t num, int64_t den)
{
    return __divmoddi4(num, den, 0);
}
