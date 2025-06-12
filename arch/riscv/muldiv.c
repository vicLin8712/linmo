/* software implementation of 64-bit multiply/divide */

#include <types.h>

uint32_t __mulsi3(uint32_t a, uint32_t b)
{
    uint32_t result = 0;
    while (b) {
        if (b & 1)
            result += a;
        a <<= 1;
        b >>= 1;
    }
    return result;
}

uint64_t __muldsi3(uint32_t a, uint32_t b)
{
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

uint64_t __muldi3(uint64_t a, uint64_t b)
{
    uint32_t al = (uint32_t) a, ah = (uint32_t) (a >> 32);
    uint32_t bl = (uint32_t) b, bh = (uint32_t) (b >> 32);

    uint64_t low = __muldsi3(al, bl);
    uint64_t mid = __muldsi3(al, bh) + __muldsi3(ah, bl);

    return low + (mid << 32);
}

uint32_t __udivmodsi4(uint32_t num, uint32_t den, int mod)
{
    if (den == 0)
        return 0;

    uint32_t quot = 0, qbit = 1;

    while ((int32_t) den >= 0) {
        den <<= 1;
        qbit <<= 1;
    }

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

int32_t __divmodsi4(int32_t num, int32_t den, int mod)
{
    int neg = (num < 0) ^ (den < 0);

    uint32_t unum = (num < 0) ? -num : num;
    uint32_t uden = (den < 0) ? -den : den;
    uint32_t res = __udivmodsi4(unum, uden, mod);

    return neg ? -res : res;
}

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

uint64_t __ashldi3(uint64_t val, int cnt)
{
    if (cnt >= 64)
        return 0;
    if (cnt == 0)
        return val;
    if (cnt < 32)
        return (val << cnt);
    return ((uint64_t) (uint32_t) val << (cnt - 32)) << 32;
}

uint64_t __ashrdi3(uint64_t val, int cnt)
{
    if (cnt >= 64)
        cnt = 63;
    if (cnt == 0)
        return val;
    if (cnt < 32)
        return ((int64_t) val) >> cnt;
    return ((int64_t) (val >> 32)) >> (cnt - 32);
}

uint64_t __lshrdi3(uint64_t val, int cnt)
{
    if (cnt >= 64)
        return 0;
    if (cnt == 0)
        return val;
    if (cnt < 32)
        return val >> cnt;
    return (val >> 32) >> (cnt - 32);
}

uint64_t __udivmoddi4(uint64_t num, uint64_t den, uint64_t *rem)
{
    if (den == 0)
        return 0;

    uint64_t quot = 0, qbit = 1;

    while ((int64_t) den >= 0) {
        den <<= 1;
        qbit <<= 1;
    }

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

int64_t __divmoddi4(int64_t num, int64_t den, int64_t *rem)
{
    int neg = (num < 0) ^ (den < 0);

    uint64_t unum = (num < 0) ? -num : num;
    uint64_t uden = (den < 0) ? -den : den;
    uint64_t res = __udivmoddi4(unum, uden, (uint64_t *) rem);

    if (rem && num < 0)
        *rem = -(*rem);

    return neg ? -res : res;
}

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
