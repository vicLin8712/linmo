#pragma once

/* All types are sized for 32-bit RISC-V architectures (RV32I). */

/* Fixed-width integer types */
typedef unsigned char uint8_t;
typedef char int8_t;

typedef unsigned short int uint16_t;
typedef short int int16_t;

typedef unsigned int uint32_t;
typedef int int32_t;

typedef unsigned long long uint64_t;
typedef long long int64_t;

/* Integer limits */
#ifndef INT8_MAX
#define INT8_MAX 127
#define INT8_MIN (-128)
#define UINT8_MAX 255U
#endif

#ifndef INT16_MAX
#define INT16_MAX 32767
#define INT16_MIN (-32768)
#define UINT16_MAX 65535U
#endif

#ifndef INT32_MAX
#define INT32_MAX 2147483647
#define INT32_MIN (-2147483648)
#define UINT32_MAX 4294967295U
#endif

#ifndef INT64_MAX
#define INT64_MAX 9223372036854775807LL
#define INT64_MIN (-9223372036854775808LL)
#define UINT64_MAX 18446744073709551615ULL
#endif

/* pointer-sized types */
typedef unsigned int uintptr_t; /* address as unsigned value */
typedef int intptr_t;           /* address as signed value   */

/* convenient aliases */
typedef int ptrdiff_t;       /* result of pointer subtractions */
typedef unsigned int size_t; /* object sizes */
typedef signed int ssize_t;  /* signed size */
typedef signed int ssize_t;  /* Signed size for error returns */

/* Compile-time assertions */
#ifdef __GNUC__
#define STATIC_ASSERT(cond, msg) _Static_assert(cond, msg)
#else
#define STATIC_ASSERT(cond, msg) \
    typedef char static_assert_##__LINE__[(cond) ? 1 : -1]
#endif

/* Endianness definitions for RV32I (little-endian) */
#define BYTE_ORDER_LITTLE_ENDIAN 1234
#define BYTE_ORDER_BIG_ENDIAN 4321
#define BYTE_ORDER BYTE_ORDER_LITTLE_ENDIAN

/* Compile-time type validation for this architecture */
STATIC_ASSERT(sizeof(uint8_t) == 1, "uint8_t must be 1 byte");
STATIC_ASSERT(sizeof(uint16_t) == 2, "uint16_t must be 2 bytes");
STATIC_ASSERT(sizeof(uint32_t) == 4, "uint32_t must be 4 bytes");
STATIC_ASSERT(sizeof(uint64_t) == 8, "uint64_t must be 8 bytes");
STATIC_ASSERT(sizeof(uintptr_t) == 4, "uintptr_t must be 4 bytes on RV32I");
STATIC_ASSERT(sizeof(size_t) == 4, "size_t must be 4 bytes on RV32I");

/* Additional safety checks */
STATIC_ASSERT(sizeof(void *) == sizeof(uintptr_t), "Pointer size mismatch");
STATIC_ASSERT(UINT8_MAX == 255U, "UINT8_MAX incorrect");
STATIC_ASSERT(UINT16_MAX == 65535U, "UINT16_MAX incorrect");
STATIC_ASSERT(UINT32_MAX == 4294967295U, "UINT32_MAX incorrect");
