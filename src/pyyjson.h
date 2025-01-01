#ifndef PYYJSON_H
#define PYYJSON_H


#include "pyyjson_config.h"

/*==============================================================================
 * Macros
 *============================================================================*/
/** compiler attribute check (since gcc 5.0, clang 2.9, icc 17) */
#ifndef pyyjson_has_attribute
#    ifdef __has_attribute
#        define pyyjson_has_attribute(x) __has_attribute(x)
#    else
#        define pyyjson_has_attribute(x) 0
#    endif
#endif


/** compiler builtin check (since gcc 10.0, clang 2.6, icc 2021) */
#ifndef pyyjson_has_builtin
#    ifdef __has_builtin
#        define pyyjson_has_builtin(x) __has_builtin(x)
#    else
#        define pyyjson_has_builtin(x) 0
#    endif
#endif

/** compiler version (GCC) */
#ifdef __GNUC__
#    define PYYJSON_GCC_VER __GNUC__
#    if defined(__GNUC_PATCHLEVEL__)
#        define pyyjson_gcc_available(major, minor, patch) \
            ((__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__) >= (major * 10000 + minor * 100 + patch))
#    else
#        define pyyjson_gcc_available(major, minor, patch) \
            ((__GNUC__ * 10000 + __GNUC_MINOR__ * 100) >= (major * 10000 + minor * 100 + patch))
#    endif
#else
#    define PYYJSON_GCC_VER 0
#    define pyyjson_gcc_available(major, minor, patch) 0
#endif

/* gcc builtin */
#if pyyjson_has_builtin(__builtin_clz)
#    define GCC_HAS_CLZ 1
#else
#    define GCC_HAS_CLZ 0
#endif

#if pyyjson_has_builtin(__builtin_clzll) || pyyjson_gcc_available(3, 4, 0)
#    define GCC_HAS_CLZLL 1
#else
#    define GCC_HAS_CLZLL 0
#endif

#if pyyjson_has_builtin(__builtin_ctz)
#    define GCC_HAS_CTZ 1
#else
#    define GCC_HAS_CTZ 0
#endif

#if pyyjson_has_builtin(__builtin_ctzll) || pyyjson_gcc_available(3, 4, 0)
#    define GCC_HAS_CTZLL 1
#else
#    define GCC_HAS_CTZLL 0
#endif

/** C version (STDC) */
#if defined(__STDC__) && (__STDC__ >= 1) && defined(__STDC_VERSION__)
#    define PYYJSON_STDC_VER __STDC_VERSION__
#else
#    define PYYJSON_STDC_VER 0
#endif

/** C++ version */
#if defined(__cplusplus)
#    define PYYJSON_CPP_VER __cplusplus
#else
#    define PYYJSON_CPP_VER 0
#endif

/** inline for compiler */
#ifndef pyyjson_inline
#    if defined(_MSC_VER) && _MSC_VER >= 1200
#        define pyyjson_inline __forceinline
#    elif defined(_MSC_VER)
#        define pyyjson_inline __inline
#    elif pyyjson_has_attribute(always_inline) || PYYJSON_GCC_VER >= 4
#        define pyyjson_inline __inline__ __attribute__((always_inline))
#    elif defined(__clang__) || defined(__GNUC__)
#        define pyyjson_inline __inline__
#    elif defined(__cplusplus) || PYYJSON_STDC_VER >= 199901L
#        define pyyjson_inline inline
#    else
#        define pyyjson_inline
#    endif
#endif

/** compiler version (MSVC) */
#ifdef _MSC_VER
#    define PYYJSON_MSC_VER _MSC_VER
#else
#    define PYYJSON_MSC_VER 0
#endif


/* msvc intrinsic */
#if PYYJSON_MSC_VER >= 1400
#    include <intrin.h>
#    if defined(_M_AMD64) || defined(_M_ARM64)
#        define MSC_HAS_BIT_SCAN_64 1
#        pragma intrinsic(_BitScanForward64)
#        pragma intrinsic(_BitScanReverse64)
#    else
#        define MSC_HAS_BIT_SCAN_64 0
#    endif
#    if defined(_M_AMD64) || defined(_M_ARM64) || \
            defined(_M_IX86) || defined(_M_ARM)
#        define MSC_HAS_BIT_SCAN 1
#        pragma intrinsic(_BitScanForward)
#        pragma intrinsic(_BitScanReverse)
#    else
#        define MSC_HAS_BIT_SCAN 0
#    endif
#    if defined(_M_AMD64)
#        define MSC_HAS_UMUL128 1
#        pragma intrinsic(_umul128)
#    else
#        define MSC_HAS_UMUL128 0
#    endif
#else
#    define MSC_HAS_BIT_SCAN_64 0
#    define MSC_HAS_BIT_SCAN 0
#    define MSC_HAS_UMUL128 0
#endif

/** noinline for compiler */
#ifndef pyyjson_noinline
#    if PYYJSON_MSC_VER >= 1400
#        define pyyjson_noinline __declspec(noinline)
#    elif pyyjson_has_attribute(noinline) || PYYJSON_GCC_VER >= 4
#        define pyyjson_noinline __attribute__((noinline))
#    else
#        define pyyjson_noinline
#    endif
#endif

/** real gcc check */
#if !defined(__clang__) && !defined(__INTEL_COMPILER) && !defined(__ICC) && \
        defined(__GNUC__)
#    define PYYJSON_IS_REAL_GCC 1
#else
#    define PYYJSON_IS_REAL_GCC 0
#endif

/** likely for compiler */
#ifndef pyyjson_likely
#    if pyyjson_has_builtin(__builtin_expect) || \
            (PYYJSON_GCC_VER >= 4 && PYYJSON_GCC_VER != 5)
#        define pyyjson_likely(expr) __builtin_expect(!!(expr), 1)
#    else
#        define pyyjson_likely(expr) (expr)
#    endif
#endif

/** unlikely for compiler */
#ifndef pyyjson_unlikely
#    if pyyjson_has_builtin(__builtin_expect) || \
            (PYYJSON_GCC_VER >= 4 && PYYJSON_GCC_VER != 5)
#        define pyyjson_unlikely(expr) __builtin_expect(!!(expr), 0)
#    else
#        define pyyjson_unlikely(expr) (expr)
#    endif
#endif

#define force_inline static pyyjson_inline
// #define force_inline force_noinline
#define force_noinline pyyjson_noinline
#define likely pyyjson_likely
#define unlikely pyyjson_unlikely


/** repeat utils */
#define REPEAT_2(x) x, x,
#define REPEAT_4(x) REPEAT_2(x) REPEAT_2(x)
#define REPEAT_8(x) REPEAT_4(x) REPEAT_4(x)
#define REPEAT_16(x) REPEAT_8(x) REPEAT_8(x)
#define REPEAT_32(x) REPEAT_16(x) REPEAT_16(x)
#define REPEAT_64(x) REPEAT_32(x) REPEAT_32(x)


/*==============================================================================
 * Macros
 *============================================================================*/

/* Macros used for loop unrolling and other purpose. */
// #define repeat2(x)  { x x }
// #define repeat3(x)  { x x x }
#define REPEAT_CALL_4(x) {x x x x}
// #define repeat8(x)  { x x x x x x x x }
#define REPEAT_CALL_16(x) {x x x x x x x x x x x x x x x x}

// #define repeat2_incr(x)   { x(0)  x(1) }
// #define repeat4_incr(x)   { x(0)  x(1)  x(2)  x(3) }
// #define repeat8_incr(x)   { x(0)  x(1)  x(2)  x(3)  x(4)  x(5)  x(6)  x(7)  }
#define REPEAT_INCR_16(x) {x(0) x(1) x(2) x(3) x(4) x(5) x(6) x(7) \
                                   x(8) x(9) x(10) x(11) x(12) x(13) x(14) x(15)}

#define REPEAT_INCR_IN_1_18(x) {x(1) x(2) x(3) x(4) x(5) x(6) x(7) x(8)                \
                                        x(9) x(10) x(11) x(12) x(13) x(14) x(15) x(16) \
                                                x(17) x(18)}


/** align for compiler */
#ifndef pyyjson_align
#    if defined(_MSC_VER) && _MSC_VER >= 1300
#        define pyyjson_align(x) __declspec(align(x))
#    elif pyyjson_has_attribute(aligned) || defined(__GNUC__)
#        define pyyjson_align(x) __attribute__((aligned(x)))
#    elif PYYJSON_CPP_VER >= 201103L
#        define pyyjson_align(x) alignas(x)
#    else
#        define pyyjson_align(x)
#    endif
#endif


/* Concat macros */
#define PYYJSON_CONCAT2_EX(a, b) a##_##b
#define PYYJSON_CONCAT2(a, b) PYYJSON_CONCAT2_EX(a, b)

#define PYYJSON_CONCAT3_EX(a, b, c) a##_##b##_##c
#define PYYJSON_CONCAT3(a, b, c) PYYJSON_CONCAT3_EX(a, b, c)

#define PYYJSON_CONCAT4_EX(a, b, c, d) a##_##b##_##c##_##d
#define PYYJSON_CONCAT4(a, b, c, d) PYYJSON_CONCAT4_EX(a, b, c, d)

#define PYYJSON_CONCAT5_EX(a, b, c, d, e) a##_##b##_##c##_##d##_##e
#define PYYJSON_CONCAT5(a, b, c, d, e) PYYJSON_CONCAT5_EX(a, b, c, d, e)

#define PYYJSON_SIMPLE_CONCAT2_EX(a, b) a##b
#define PYYJSON_SIMPLE_CONCAT2(a, b) PYYJSON_SIMPLE_CONCAT2_EX(a, b)

#define PYYJSON_SIMPLE_CONCAT3_EX(a, b, c) a##b##c
#define PYYJSON_SIMPLE_CONCAT3(a, b, c) PYYJSON_SIMPLE_CONCAT3_EX(a, b, c)

#define PYYJSON_MAX(x, y) ((x) > (y) ? (x) : (y))

/* String type macros */
#define PYYJSON_STRING_TYPE_ASCII 0
#define PYYJSON_STRING_TYPE_LATIN1 1
#define PYYJSON_STRING_TYPE_UCS2 2
#define PYYJSON_STRING_TYPE_UCS4 4


/* IEEE 754 floating-point binary representation */
#if defined(DOUBLE_IS_LITTLE_ENDIAN_IEEE754) || defined(DOUBLE_IS_BIG_ENDIAN_IEEE754) || defined(DOUBLE_IS_ARM_MIXED_ENDIAN_IEEE754)
#    define PYYJSON_HAS_IEEE_754 1
#elif (FLT_RADIX == 2) && (DBL_MANT_DIG == 53) && (DBL_DIG == 15) && \
        (DBL_MIN_EXP == -1021) && (DBL_MAX_EXP == 1024) &&           \
        (DBL_MIN_10_EXP == -307) && (DBL_MAX_10_EXP == 308)
#    define PYYJSON_HAS_IEEE_754 1
#else
#    define PYYJSON_HAS_IEEE_754 0
static_assert(false, "false");
#endif

/* Helper for quickly write an err handle. */
#define RETURN_ON_UNLIKELY_ERR(x) \
    do {                          \
        if (unlikely((x))) {      \
            return false;         \
        }                         \
    } while (0)

/* Some constants. */
#define _Quote (34)
#define _Slash (92)
#define _MinusOne (-1)
#define ControlMax (32)


/*==============================================================================
 * 128-bit Integer Utils
 * These functions are used by the floating-point number reader and writer.
 *============================================================================*/

/** Multiplies two 64-bit unsigned integers (a * b),
    returns the 128-bit result as 'hi' and 'lo'. */
force_inline void u128_mul(u64 a, u64 b, u64 *hi, u64 *lo) {
#if PYYJSON_HAS_INT128
    u128 m = (u128)a * b;
    *hi = (u64)(m >> 64);
    *lo = (u64)(m);
#elif MSC_HAS_UMUL128
    *lo = _umul128(a, b, hi);
#else
    u32 a0 = (u32)(a), a1 = (u32)(a >> 32);
    u32 b0 = (u32)(b), b1 = (u32)(b >> 32);
    u64 p00 = (u64)a0 * b0, p01 = (u64)a0 * b1;
    u64 p10 = (u64)a1 * b0, p11 = (u64)a1 * b1;
    u64 m0 = p01 + (p00 >> 32);
    u32 m00 = (u32)(m0), m01 = (u32)(m0 >> 32);
    u64 m1 = p10 + m00;
    u32 m10 = (u32)(m1), m11 = (u32)(m1 >> 32);
    *hi = p11 + m01 + m11;
    *lo = ((u64)m10 << 32) | (u32)p00;
#endif
}

/** Multiplies two 64-bit unsigned integers and add a value (a * b + c),
    returns the 128-bit result as 'hi' and 'lo'. */
force_inline void u128_mul_add(u64 a, u64 b, u64 c, u64 *hi, u64 *lo) {
#if PYYJSON_HAS_INT128
    u128 m = (u128)a * b + c;
    *hi = (u64)(m >> 64);
    *lo = (u64)(m);
#else
    u64 h, l, t;
    u128_mul(a, b, &h, &l);
    t = l + c;
    h += (u64)(((t < l) | (t < c)));
    *hi = h;
    *lo = t;
#endif
}


/*==============================================================================
 * Digit Character Matcher
 *============================================================================*/

/** Digit type */
typedef u8 digi_type;

/** Digit: '0'. */
static const digi_type DIGI_TYPE_ZERO = 1 << 0;

/** Digit: [1-9]. */
static const digi_type DIGI_TYPE_NONZERO = 1 << 1;

/** Plus sign (positive): '+'. */
static const digi_type DIGI_TYPE_POS = 1 << 2;

/** Minus sign (negative): '-'. */
static const digi_type DIGI_TYPE_NEG = 1 << 3;

/** Decimal point: '.' */
static const digi_type DIGI_TYPE_DOT = 1 << 4;

/** Exponent sign: 'e, 'E'. */
static const digi_type DIGI_TYPE_EXP = 1 << 5;


/** Whitespace character: ' ', '\\t', '\\n', '\\r'. */
static const u8 CHAR_TYPE_SPACE = 1 << 0;

/** Number character: '-', [0-9]. */
static const u8 CHAR_TYPE_NUMBER = 1 << 1;

/** JSON Escaped character: '"', '\', [0x00-0x1F]. */
static const u8 CHAR_TYPE_ESC_ASCII = 1 << 2;

/** Non-ASCII character: [0x80-0xFF]. */
static const u8 CHAR_TYPE_NON_ASCII = 1 << 3;

/** JSON container character: '{', '['. */
static const u8 CHAR_TYPE_CONTAINER = 1 << 4;

/** Comment character: '/'. */
static const u8 CHAR_TYPE_COMMENT = 1 << 5;

/** Line end character: '\\n', '\\r', '\0'. */
static const u8 CHAR_TYPE_LINE_END = 1 << 6;

/** Hexadecimal numeric character: [0-9a-fA-F]. */
static const u8 CHAR_TYPE_HEX = 1 << 7;


/** Digit type table (generate with misc/make_tables.c) */
static const u8 digi_table[256] = {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x04, 0x00, 0x08, 0x10, 0x00,
        0x01, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02,
        0x02, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

/** Match a character with specified type. */
force_inline bool digi_is_type(u8 d, u8 type) {
    return (digi_table[d] & type) != 0;
}

/** Match a sign: '+', '-' */
force_inline bool digi_is_sign(u8 d) {
    return digi_is_type(d, (u8)(DIGI_TYPE_POS | DIGI_TYPE_NEG));
}

/** Match a none zero digit: [1-9] */
force_inline bool digi_is_nonzero(u8 d) {
    return digi_is_type(d, (u8)DIGI_TYPE_NONZERO);
}

/** Match a digit: [0-9] */
force_inline bool digi_is_digit(u8 d) {
    return digi_is_type(d, (u8)(DIGI_TYPE_ZERO | DIGI_TYPE_NONZERO));
}

/** Match an exponent sign: 'e', 'E'. */
force_inline bool digi_is_exp(u8 d) {
    return digi_is_type(d, (u8)DIGI_TYPE_EXP);
}

/** Match a floating point indicator: '.', 'e', 'E'. */
force_inline bool digi_is_fp(u8 d) {
    return digi_is_type(d, (u8)(DIGI_TYPE_DOT | DIGI_TYPE_EXP));
}

/** Match a digit or floating point indicator: [0-9], '.', 'e', 'E'. */
force_inline bool digi_is_digit_or_fp(u8 d) {
    return digi_is_type(d, (u8)(DIGI_TYPE_ZERO | DIGI_TYPE_NONZERO |
                                DIGI_TYPE_DOT | DIGI_TYPE_EXP));
}

/** Returns the number of leading 0-bits in value (input should not be 0). */
force_inline u32 u32_lz_bits(u32 v) {
    assert(v);
#if GCC_HAS_CLZ
    return (u32)__builtin_clz(v);
#elif MSC_HAS_BIT_SCAN
    unsigned long r;
    _BitScanReverse(&r, v);
    return (u32)31 - (u32)r;
#else
    /*
     branchless, use de Bruijn sequences
     see: https://www.chessprogramming.org/BitScan
     */
    const u8 table[64] = {
            63, 16, 62, 7, 15, 36, 61, 3, 6, 14, 22, 26, 35, 47, 60, 2,
            9, 5, 28, 11, 13, 21, 42, 19, 25, 31, 34, 40, 46, 52, 59, 1,
            17, 8, 37, 4, 23, 27, 48, 10, 29, 12, 43, 20, 32, 41, 53, 18,
            38, 24, 49, 30, 44, 33, 54, 39, 50, 45, 55, 51, 56, 57, 58, 0};
    v |= v >> 1;
    v |= v >> 2;
    v |= v >> 4;
    v |= v >> 8;
    v |= v >> 16;
    return table[(v * U64(0x03F79D71, 0xB4CB0A89)) >> 58];
#endif
}

/** Returns the number of leading 0-bits in value (input should not be 0). */
force_inline u32 u64_lz_bits(u64 v) {
    assert(v);
#if GCC_HAS_CLZLL
    return (u32)__builtin_clzll(v);
#elif MSC_HAS_BIT_SCAN_64
    unsigned long r;
    _BitScanReverse64(&r, v);
    return (u32)63 - (u32)r;
#elif MSC_HAS_BIT_SCAN
    unsigned long hi, lo;
    bool hi_set = _BitScanReverse(&hi, (u32)(v >> 32)) != 0;
    _BitScanReverse(&lo, (u32)v);
    hi |= 32;
    return (u32)63 - (u32)(hi_set ? hi : lo);
#else
    /*
     branchless, use de Bruijn sequences
     see: https://www.chessprogramming.org/BitScan
     */
    const u8 table[64] = {
            63, 16, 62, 7, 15, 36, 61, 3, 6, 14, 22, 26, 35, 47, 60, 2,
            9, 5, 28, 11, 13, 21, 42, 19, 25, 31, 34, 40, 46, 52, 59, 1,
            17, 8, 37, 4, 23, 27, 48, 10, 29, 12, 43, 20, 32, 41, 53, 18,
            38, 24, 49, 30, 44, 33, 54, 39, 50, 45, 55, 51, 56, 57, 58, 0};
    v |= v >> 1;
    v |= v >> 2;
    v |= v >> 4;
    v |= v >> 8;
    v |= v >> 16;
    v |= v >> 32;
    return table[(v * U64(0x03F79D71, 0xB4CB0A89)) >> 58];
#endif
}

/** Returns the number of trailing 0-bits in value (input should not be 0). */
force_inline u32 u32_tz_bits(u32 v) {
    assert(v);
#if GCC_HAS_CTZ
    return (u32)__builtin_ctz(v);
#elif MSC_HAS_BIT_SCAN
    unsigned long r;
    _BitScanForward(&r, v);
    return (u32)r;
#else
    /*
     branchless, use de Bruijn sequences
     see: https://www.chessprogramming.org/BitScan
     */
    const u8 table[64] = {
            0, 1, 2, 53, 3, 7, 54, 27, 4, 38, 41, 8, 34, 55, 48, 28,
            62, 5, 39, 46, 44, 42, 22, 9, 24, 35, 59, 56, 49, 18, 29, 11,
            63, 52, 6, 26, 37, 40, 33, 47, 61, 45, 43, 21, 23, 58, 17, 10,
            51, 25, 36, 32, 60, 20, 57, 16, 50, 31, 19, 15, 30, 14, 13, 12};
    return table[(((u64)v & (~(u64)v + 1)) * U64(0x022FDD63, 0xCC95386D)) >> 58];
#endif
}

/** Returns the number of trailing 0-bits in value (input should not be 0). */
force_inline u32 u64_tz_bits(u64 v) {
    assert(v);
#if GCC_HAS_CTZLL
    return (u32)__builtin_ctzll(v);
#elif MSC_HAS_BIT_SCAN_64
    unsigned long r;
    _BitScanForward64(&r, v);
    return (u32)r;
#elif MSC_HAS_BIT_SCAN
    unsigned long lo, hi;
    bool lo_set = _BitScanForward(&lo, (u32)(v)) != 0;
    _BitScanForward(&hi, (u32)(v >> 32));
    hi += 32;
    return lo_set ? lo : hi;
#else
    /*
     branchless, use de Bruijn sequences
     see: https://www.chessprogramming.org/BitScan
     */
    const u8 table[64] = {
            0, 1, 2, 53, 3, 7, 54, 27, 4, 38, 41, 8, 34, 55, 48, 28,
            62, 5, 39, 46, 44, 42, 22, 9, 24, 35, 59, 56, 49, 18, 29, 11,
            63, 52, 6, 26, 37, 40, 33, 47, 61, 45, 43, 21, 23, 58, 17, 10,
            51, 25, 36, 32, 60, 20, 57, 16, 50, 31, 19, 15, 30, 14, 13, 12};
    return table[((v & (~v + 1)) * U64(0x022FDD63, 0xCC95386D)) >> 58];
#endif
}

#endif // PYYJSON_H
