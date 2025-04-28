#ifndef PYYJSON_SIMD_IMPL_H
#define PYYJSON_SIMD_IMPL_H

#include "Python.h"
#include "pyyjson.h"
#include "simd/simd_detect.h"
#include <string.h>

/* Common SIMD vector types. */
#if defined(_MSC_VER) && !defined(__clang__)
typedef u32 VECTOR_U8_32_A;

typedef __declspec(align(1)) struct {
    u8 v[4];
} VECTOR_U8_32_U;

typedef u32 VECTOR_U16_32_A;

typedef __declspec(align(2)) struct {
    u16 v[2];
} VECTOR_U16_32_U;

typedef u32 VECTOR_U32_32_A;

typedef __declspec(align(4)) struct {
    u32 v[1];
} VECTOR_U32_32_U;

typedef u64 VECTOR_U8_64_A;

typedef __declspec(align(1)) struct {
    u8 v[8];
} VECTOR_U8_64_U;

typedef u64 VECTOR_U16_64_A;

typedef __declspec(align(2)) struct {
    u16 v[4];
} VECTOR_U16_64_U;

typedef u64 VECTOR_U32_64_A;

typedef __declspec(align(4)) struct {
    u32 v[2];
} VECTOR_U32_64_U;

typedef __m128i VECTOR_U8_128_A;

typedef __declspec(align(1)) struct {
    u8 v[16];
} VECTOR_U8_128_U;

typedef __m128i VECTOR_U16_128_A;

typedef __declspec(align(2)) struct {
    u16 v[8];
} VECTOR_U16_128_U;

typedef __m128i VECTOR_U32_128_A;

typedef __declspec(align(4)) struct {
    u32 v[4];
} VECTOR_U32_128_U;

typedef __m256i VECTOR_U8_256_A;

typedef __declspec(align(1)) struct {
    u8 v[32];
} VECTOR_U8_256_U;

typedef __m256i VECTOR_U16_256_A;

typedef __declspec(align(2)) struct {
    u16 v[16];
} VECTOR_U16_256_U;

typedef __m256i VECTOR_U32_256_A;

typedef __declspec(align(4)) struct {
    u32 v[8];
} VECTOR_U32_256_U;

typedef __m512i VECTOR_U8_512_A;

typedef __declspec(align(1)) struct {
    u8 v[64];
} VECTOR_U8_512_U;

typedef __m512i VECTOR_U16_512_A;

typedef __declspec(align(2)) struct {
    u16 v[32];
} VECTOR_U16_512_U;

typedef __m512i VECTOR_U32_512_A;

typedef __declspec(align(4)) struct {
    u32 v[16];
} VECTOR_U32_512_U;

#elif PYYJSON_AARCH
// smaller than 128
typedef u8 VECTOR_U8_32_A __attribute__((__vector_size__(4), __aligned__(4)));
typedef u8 VECTOR_U8_32_U __attribute__((__vector_size__(4), __aligned__(1)));
typedef u16 VECTOR_U16_32_A __attribute__((__vector_size__(4), __aligned__(4)));
typedef u16 VECTOR_U16_32_U __attribute__((__vector_size__(4), __aligned__(2)));
typedef u32 VECTOR_U32_32_A __attribute__((__vector_size__(4), __aligned__(4)));
typedef u32 VECTOR_U32_32_U __attribute__((__vector_size__(4), __aligned__(4)));
// 64~128
typedef uint8x8_t VECTOR_U8_64_A;
typedef u8 VECTOR_U8_64_U __attribute__((__vector_size__(8), __aligned__(1)));
typedef uint16x4_t VECTOR_U16_64_A;
typedef u16 VECTOR_U16_64_U __attribute__((__vector_size__(8), __aligned__(2)));
typedef uint32x2_t VECTOR_U32_64_A;
typedef u32 VECTOR_U32_64_U __attribute__((__vector_size__(8), __aligned__(4)));
typedef uint8x16_t VECTOR_U8_128_A;
typedef u8 VECTOR_U8_128_U __attribute__((__vector_size__(16), __aligned__(1)));
typedef uint16x8_t VECTOR_U16_128_A;
typedef u16 VECTOR_U16_128_U __attribute__((__vector_size__(16), __aligned__(2)));
typedef uint32x4_t VECTOR_U32_128_A;
typedef u32 VECTOR_U32_128_U __attribute__((__vector_size__(16), __aligned__(4)));
// larger than 128
typedef u8 VECTOR_U8_256_A __attribute__((__vector_size__(32), __aligned__(32)));
typedef u8 VECTOR_U8_256_U __attribute__((__vector_size__(32), __aligned__(1)));
typedef u16 VECTOR_U16_256_A __attribute__((__vector_size__(32), __aligned__(32)));
typedef u16 VECTOR_U16_256_U __attribute__((__vector_size__(32), __aligned__(2)));
typedef u32 VECTOR_U32_256_A __attribute__((__vector_size__(32), __aligned__(32)));
typedef u32 VECTOR_U32_256_U __attribute__((__vector_size__(32), __aligned__(4)));
typedef u8 VECTOR_U8_512_A __attribute__((__vector_size__(64), __aligned__(64)));
typedef u8 VECTOR_U8_512_U __attribute__((__vector_size__(64), __aligned__(1)));
typedef u16 VECTOR_U16_512_A __attribute__((__vector_size__(64), __aligned__(64)));
typedef u16 VECTOR_U16_512_U __attribute__((__vector_size__(64), __aligned__(2)));
typedef u32 VECTOR_U32_512_A __attribute__((__vector_size__(64), __aligned__(64)));
typedef u32 VECTOR_U32_512_U __attribute__((__vector_size__(64), __aligned__(4)));

typedef u8 VECTOR_U8_1024_A __attribute__((__vector_size__(128), __aligned__(128)));
typedef u8 VECTOR_U8_1024_U __attribute__((__vector_size__(128), __aligned__(1)));
typedef u16 VECTOR_U16_1024_A __attribute__((__vector_size__(128), __aligned__(128)));
typedef u16 VECTOR_U16_1024_U __attribute__((__vector_size__(128), __aligned__(2)));
typedef u32 VECTOR_U32_1024_A __attribute__((__vector_size__(128), __aligned__(128)));
typedef u32 VECTOR_U32_1024_U __attribute__((__vector_size__(128), __aligned__(4)));

typedef u8 VECTOR_U8_2048_A __attribute__((__vector_size__(256), __aligned__(256)));
typedef u8 VECTOR_U8_2048_U __attribute__((__vector_size__(256), __aligned__(1)));
typedef u16 VECTOR_U16_2048_A __attribute__((__vector_size__(256), __aligned__(256)));
typedef u16 VECTOR_U16_2048_U __attribute__((__vector_size__(256), __aligned__(2)));
typedef u32 VECTOR_U32_2048_A __attribute__((__vector_size__(256), __aligned__(256)));
typedef u32 VECTOR_U32_2048_U __attribute__((__vector_size__(256), __aligned__(4)));
#else
typedef u8 VECTOR_U8_32_A __attribute__((__vector_size__(4), __aligned__(4)));
typedef u8 VECTOR_U8_32_U __attribute__((__vector_size__(4), __aligned__(1)));
typedef u16 VECTOR_U16_32_A __attribute__((__vector_size__(4), __aligned__(4)));
typedef u16 VECTOR_U16_32_U __attribute__((__vector_size__(4), __aligned__(2)));
typedef u32 VECTOR_U32_32_A __attribute__((__vector_size__(4), __aligned__(4)));
typedef u32 VECTOR_U32_32_U __attribute__((__vector_size__(4), __aligned__(4)));
typedef u8 VECTOR_U8_64_A __attribute__((__vector_size__(8), __aligned__(8)));
typedef u8 VECTOR_U8_64_U __attribute__((__vector_size__(8), __aligned__(1)));
typedef u16 VECTOR_U16_64_A __attribute__((__vector_size__(8), __aligned__(8)));
typedef u16 VECTOR_U16_64_U __attribute__((__vector_size__(8), __aligned__(2)));
typedef u32 VECTOR_U32_64_A __attribute__((__vector_size__(8), __aligned__(8)));
typedef u32 VECTOR_U32_64_U __attribute__((__vector_size__(8), __aligned__(4)));
typedef u8 VECTOR_U8_128_A __attribute__((__vector_size__(16), __aligned__(16)));
typedef u8 VECTOR_U8_128_U __attribute__((__vector_size__(16), __aligned__(1)));
typedef u16 VECTOR_U16_128_A __attribute__((__vector_size__(16), __aligned__(16)));
typedef u16 VECTOR_U16_128_U __attribute__((__vector_size__(16), __aligned__(2)));
typedef u32 VECTOR_U32_128_A __attribute__((__vector_size__(16), __aligned__(16)));
typedef u32 VECTOR_U32_128_U __attribute__((__vector_size__(16), __aligned__(4)));
typedef u8 VECTOR_U8_256_A __attribute__((__vector_size__(32), __aligned__(32)));
typedef u8 VECTOR_U8_256_U __attribute__((__vector_size__(32), __aligned__(1)));
typedef u16 VECTOR_U16_256_A __attribute__((__vector_size__(32), __aligned__(32)));
typedef u16 VECTOR_U16_256_U __attribute__((__vector_size__(32), __aligned__(2)));
typedef u32 VECTOR_U32_256_A __attribute__((__vector_size__(32), __aligned__(32)));
typedef u32 VECTOR_U32_256_U __attribute__((__vector_size__(32), __aligned__(4)));
typedef u8 VECTOR_U8_512_A __attribute__((__vector_size__(64), __aligned__(64)));
typedef u8 VECTOR_U8_512_U __attribute__((__vector_size__(64), __aligned__(1)));
typedef u16 VECTOR_U16_512_A __attribute__((__vector_size__(64), __aligned__(64)));
typedef u16 VECTOR_U16_512_U __attribute__((__vector_size__(64), __aligned__(2)));
typedef u32 VECTOR_U32_512_A __attribute__((__vector_size__(64), __aligned__(64)));
typedef u32 VECTOR_U32_512_U __attribute__((__vector_size__(64), __aligned__(4)));

typedef u8 VECTOR_U8_1024_A __attribute__((__vector_size__(128), __aligned__(128)));
typedef u8 VECTOR_U8_1024_U __attribute__((__vector_size__(128), __aligned__(1)));
typedef u16 VECTOR_U16_1024_A __attribute__((__vector_size__(128), __aligned__(128)));
typedef u16 VECTOR_U16_1024_U __attribute__((__vector_size__(128), __aligned__(2)));
typedef u32 VECTOR_U32_1024_A __attribute__((__vector_size__(128), __aligned__(128)));
typedef u32 VECTOR_U32_1024_U __attribute__((__vector_size__(128), __aligned__(4)));

typedef u8 VECTOR_U8_2048_A __attribute__((__vector_size__(256), __aligned__(256)));
typedef u8 VECTOR_U8_2048_U __attribute__((__vector_size__(256), __aligned__(1)));
typedef u16 VECTOR_U16_2048_A __attribute__((__vector_size__(256), __aligned__(256)));
typedef u16 VECTOR_U16_2048_U __attribute__((__vector_size__(256), __aligned__(2)));
typedef u32 VECTOR_U32_2048_A __attribute__((__vector_size__(256), __aligned__(256)));
typedef u32 VECTOR_U32_2048_U __attribute__((__vector_size__(256), __aligned__(4)));
#endif


#if PYYJSON_X86
#    if SIMD_BIT_SIZE == 512
#        define SIMD_VAR z
#        define SIMD_TYPE __m512i
#        define SIMD_MASK_TYPE u64
#        define SIMD_SMALL_MASK_TYPE u16
#        define SIMD_BIT_MASK_TYPE u64
#        define SIMD_HALF_TYPE __m256i
#        define SIMD_EXTRACT_QUARTER _mm512_extracti32x4_epi32
#        define SIMD_EXTRACT_HALF _mm512_extracti64x4_epi64
#        define SIMD_REAL_HALF_TYPE __m256i
#        define SIMD_REAL_QUARTER_TYPE __m128i
#    elif SIMD_BIT_SIZE == 256
#        define SIMD_VAR y
#        define SIMD_TYPE __m256i
#        define SIMD_MASK_TYPE SIMD_TYPE
#        define SIMD_SMALL_MASK_TYPE __m128i
#        define SIMD_BIT_MASK_TYPE u32
#        define SIMD_HALF_TYPE __m128i
#        define SIMD_EXTRACT_HALF _mm256_extracti128_si256
#        define SIMD_REAL_HALF_TYPE __m128i
#        define SIMD_REAL_QUARTER_TYPE u64
#    else
#        define SIMD_VAR x
#        define SIMD_TYPE __m128i
#        define SIMD_MASK_TYPE SIMD_TYPE
#        define SIMD_SMALL_MASK_TYPE SIMD_TYPE
#        define SIMD_BIT_MASK_TYPE u16
#        define SIMD_HALF_TYPE SIMD_TYPE
#        define SIMD_REAL_HALF_TYPE u64
#        define SIMD_REAL_QUARTER_TYPE u32
#    endif

#    define SIMD_STORER PYYJSON_CONCAT2(write, SIMD_BIT_SIZE)
#    define SIMD_AND PYYJSON_CONCAT2(simd_and, SIMD_BIT_SIZE)

/*==============================================================================
 * common SIMD code
 *============================================================================*/

/*
 * Right shift 128 bits. Shifted bits should be multiple of 8.
 */
#    define RIGHT_SHIFT_128BITS(x, bits, out_ptr)                  \
        do {                                                       \
            static_assert(((bits) % 8) == 0, "((bits) % 8) == 0"); \
            *(out_ptr) = _mm_bsrli_si128((x), (bits) / 8);         \
        } while (0)

/*
 * Load memory to a simd variable.
 * This is unaligned.
 */
force_inline SIMD_TYPE load_simd(const void *src) {
#    if SIMD_BIT_SIZE == 512
    return _mm512_loadu_si512(src);
#    elif SIMD_BIT_SIZE == 256
    return _mm256_lddqu_si256((const SIMD_256_IU *)src);
#    elif __SSE3__
    return _mm_lddqu_si128((const SIMD_128_IU *)src);
#    else
    return _mm_loadu_si128((const SIMD_128_IU *)src);
#    endif
}

/*
 * Load memory to a simd variable.
 * This is aligned.
 */
force_inline SIMD_TYPE load_simd_aligned(const void *src) {
#    if SIMD_BIT_SIZE == 512
    return _mm512_load_si512(src);
#    elif SIMD_BIT_SIZE == 256
    return _mm256_load_si256((const __m256i *)src);
#    else
    return _mm_load_si128((const __m128i *)src);
#    endif
}

/*
 * Write memory with length sizeof(SIMD_TYPE) to `dst`.
 * This is unaligned.
 */
force_inline void write_simd(void *dst, SIMD_TYPE SIMD_VAR) {
#    if SIMD_BIT_SIZE == 512
    _mm512_storeu_si512(dst, SIMD_VAR);
#    elif SIMD_BIT_SIZE == 256
    _mm256_storeu_si256((SIMD_256_IU *)dst, SIMD_VAR);
#    else
    _mm_storeu_si128((SIMD_128_IU *)dst, SIMD_VAR);
#    endif
}

/*
 * write memory with length sizeof(SIMD_TYPE) to `dst`.
 * The `dst` must be aligned.
 */
force_inline void write_aligned(void *dst, SIMD_TYPE SIMD_VAR) {
#    if SIMD_BIT_SIZE == 128
    _mm_store_si128((__m128i *)dst, SIMD_VAR);
#    elif SIMD_BIT_SIZE == 256
    _mm256_store_si256((__m256i *)dst, SIMD_VAR);
#    else
    _mm512_store_si512(dst, SIMD_VAR);
#    endif
}

/*
 * write memory with length sizeof(SIMD_TYPE) to dst.
 */
force_inline void write_128(void *dst, SIMD_128 x) {
    _mm_storeu_si128((SIMD_128_IU *)dst, x);
}

force_inline SIMD_128 load_128(const void *src) {
#    if __SSE3__
    return _mm_lddqu_si128((const SIMD_128_IU *)src);
#    else
    return _mm_loadu_si128((const SIMD_128_IU *)src);
#    endif
}

force_inline SIMD_128 load_128_aligned(const void *src) {
    return _mm_load_si128((const __m128i *)src);
}

force_inline SIMD_128 simd_and_128(SIMD_128 a, SIMD_128 b) {
    return _mm_and_si128(a, b);
}

force_inline SIMD_128 simd_or_128(SIMD_128 a, SIMD_128 b) {
    return _mm_or_si128(a, b);
}

force_inline SIMD_128 cmpgt_i32_128(SIMD_128 a, SIMD_128 b) {
    return _mm_cmpgt_epi32(a, b);
}

force_inline SIMD_128 broadcast_8_128(i8 v) {
    return _mm_set1_epi8(v);
}

force_inline SIMD_128 broadcast_16_128(i16 v) {
    return _mm_set1_epi16(v);
}

force_inline SIMD_128 broadcast_32_128(i32 v) {
    return _mm_set1_epi32(v);
}

force_inline SIMD_128 broadcast_64_128(i64 v) {
#    if defined(_MSC_VER) && !defined(_M_IX86)
    return _mm_set1_epi64x(v);
#    else
    return _mm_set1_epi64((__m64)v);
#    endif
}

force_inline const void *read_rshift_mask_table(int row);

/*
 * Right shift 128 bits for the case imm8 cannot be determined at compile time.
 Shifted bits should be multiple of 8; imm8 is the number of "bytes" to shift.
 */
force_inline SIMD_128 runtime_right_shift_128bits(SIMD_128 x, int imm8) {
#    if __SSSE3__
    return _mm_shuffle_epi8(x, load_128_aligned(read_rshift_mask_table(imm8)));
#    else
    switch (imm8) {
        case 1: {
            return _mm_bsrli_si128(x, 1);
            break;
        }
        case 2: {
            return _mm_bsrli_si128(x, 2);
            break;
        }
        case 3: {
            return _mm_bsrli_si128(x, 3);
            break;
        }
        case 4: {
            return _mm_bsrli_si128(x, 4);
            break;
        }
        case 5: {
            return _mm_bsrli_si128(x, 5);
            break;
        }
        case 6: {
            return _mm_bsrli_si128(x, 6);
            break;
        }
        case 7: {
            return _mm_bsrli_si128(x, 7);
            break;
        }
        case 8: {
            return _mm_bsrli_si128(x, 8);
            break;
        }
        case 9: {
            return _mm_bsrli_si128(x, 9);
            break;
        }
        case 10: {
            return _mm_bsrli_si128(x, 10);
            break;
        }
        case 11: {
            return _mm_bsrli_si128(x, 11);
            break;
        }
        case 12: {
            return _mm_bsrli_si128(x, 12);
            break;
        }
        case 13: {
            return _mm_bsrli_si128(x, 13);
            break;
        }
        case 14: {
            return _mm_bsrli_si128(x, 14);
            break;
        }
        case 15: {
            return _mm_bsrli_si128(x, 15);
            break;
        }
        default: {
            Py_UNREACHABLE();
            assert(false);
        }
    }
    Py_UNREACHABLE();
    return x;
#    endif
}

force_inline SIMD_128 set_32_128(i32 d, i32 c, i32 b, i32 a) {
    return _mm_set_epi32(a, b, c, d);
}

force_inline SIMD_128 unpack_hi_64_128(SIMD_128 a, SIMD_128 b) {
    return _mm_unpackhi_epi64(a, b);
}

force_inline SIMD_128 cmpeq0_8_128(SIMD_128 a) {
    return _mm_cmpeq_epi8(a, _mm_setzero_si128());
}

force_inline u16 to_bitmask_128(SIMD_128 a) {
    return (u16)_mm_movemask_epi8(a);
}

force_inline SIMD_128 cmpeq_8_128(SIMD_128 a, SIMD_128 b) {
    return _mm_cmpeq_epi8(a, b);
}

force_inline SIMD_128 cmpeq_16_128(SIMD_128 a, SIMD_128 b) {
    return _mm_cmpeq_epi16(a, b);
}

force_inline SIMD_128 cmpeq_32_128(SIMD_128 a, SIMD_128 b) {
    return _mm_cmpeq_epi32(a, b);
}

force_inline SIMD_128 cmpneq_8_128(SIMD_128 a, SIMD_128 b) {
    return _mm_cmpeq_epi8(_mm_cmpeq_epi8(a, b), _mm_setzero_si128());
}

force_inline SIMD_128 cmpneq_16_128(SIMD_128 a, SIMD_128 b) {
    return _mm_cmpeq_epi16(_mm_cmpeq_epi16(a, b), _mm_setzero_si128());
}

force_inline SIMD_128 cmpneq_32_128(SIMD_128 a, SIMD_128 b) {
    return _mm_cmpeq_epi32(_mm_cmpeq_epi32(a, b), _mm_setzero_si128());
}

force_inline SIMD_128 satureate_minus_128(SIMD_128 a, SIMD_128 b) {
    return _mm_subs_epu8(a, b);
}

/* Elevate utilities.
 * See https://github.com/samyvilar/dyn_perf/blob/master/sse2.h
 */
force_inline VECTOR_U16_128_A elevate_1_2_to_128(VECTOR_U8_128_A a) {
#    if __SSE4_1__
    return _mm_cvtepu8_epi16(a);
#    elif __SSSE3__
    return _mm_shuffle_epi8(a, _mm_set_epi8(0x80, 7, 0x80, 6, 0x80, 5, 0x80, 4, 0x80, 3, 0x80, 2, 0x80, 1, 0x80, 0));
#    else
    return _mm_unpacklo_epi8(a, _mm_setzero_si128()); // ~2 cycles ..
#    endif
}

force_inline VECTOR_U32_128_A elevate_1_4_to_128(VECTOR_U8_128_A a) {
#    if __SSE4_1__
    return _mm_cvtepu8_epi32(a);
#    elif __SSSE3__
    return _mm_shuffle_epi8(
            a,
            _mm_set_epi8(
                    0x80, 0x80, 0x80, 3,
                    0x80, 0x80, 0x80, 2,
                    0x80, 0x80, 0x80, 1,
                    0x80, 0x80, 0x80, 0));
#    else
    a = _mm_unpacklo_epi8(a, a);                         // a0, a0, a1, a1, a2, a2, a3, a3, ....
    return _mm_srli_epi32(_mm_unpacklo_epi16(a, a), 24); // ~ 3 cycles ...
#    endif
}

force_inline VECTOR_U32_128_A elevate_2_4_to_128(VECTOR_U16_128_A a) {

#    if defined(__SSE4_1__)
    return _mm_cvtepu16_epi32(a);
#    elif defined(__SSSE3__)
    return _mm_shuffle_epi8(
            a,
            _mm_set_epi8(
                    0x80, 0x80, 7, 6,
                    0x80, 0x80, 5, 4,
                    0x80, 0x80, 3, 2,
                    0x80, 0x80, 1, 0));
#    else
    return _mm_unpacklo_epi16(a, _mm_setzero_si128());
#    endif
}

force_inline void extract_128_two_parts(SIMD_128 x, SIMD_128 *restrict x1, SIMD_128 *restrict x2) {
    *x1 = x;
    *x2 = unpack_hi_64_128(x, x);
}

force_inline void extract_128_four_parts(SIMD_128 x, SIMD_128 *restrict x1, SIMD_128 *restrict x2, SIMD_128 *restrict x3, SIMD_128 *restrict x4) {
#    if __SSE3__
#        define MOVEHDUP(_x) (SIMD_128) _mm_movehdup_ps((__m128)(_x))
#    else
#        define MOVEHDUP(_x) _mm_bsrli_si128((_x), 4)
#    endif
    *x1 = x;
    *x2 = MOVEHDUP(x);
    *x3 = unpack_hi_64_128(x, x);
    *x4 = MOVEHDUP(*x3);
#    undef MOVEHDUP
}

force_inline u64 real_extract_first_64_from_128(SIMD_128 x) {
#    if defined(_MSC_VER) && !defined(_M_IX86) && !defined(__clang__)
    return (u64)_mm_cvtsi128_si64x(x);
#    else
    return (u64)_mm_cvtsi128_si64(x);
#    endif
}

force_inline u32 real_extract_first_32_from_128(SIMD_128 x) {
    return (u32)_mm_cvtsi128_si32(x);
}

/* (a & b) == 0 */
force_inline bool testz_128(SIMD_128 a, SIMD_128 b) {
#    if defined(__SSE4_1__)
    return (bool)_mm_testz_si128(a, b);
#    else
    return _mm_movemask_epi8(_mm_cmpeq_epi8(simd_and_128(a, b), _mm_setzero_si128())) == 0xFFFF;
#    endif
}

/*
 * Mask utilities.
 */
force_inline bool check_mask_zero(SIMD_MASK_TYPE mask) {
#    if SIMD_BIT_SIZE == 512
    return mask == 0;
#    elif SIMD_BIT_SIZE == 256
    return (bool)_mm256_testz_si256(mask, mask);
#    else
#        if defined(__SSE4_1__)
    return (bool)_mm_testz_si128(mask, mask);
#        else
    return _mm_movemask_epi8(_mm_cmpeq_epi8(mask, _mm_setzero_si128())) == 0xFFFF;
#        endif
#    endif
}

/*
 * UTF-8.
 */

force_inline void ucs2_encode_2bytes_utf8_sse2(VECTOR_U16_128_A x, u8 *writer) {
    /* abcdefgh|12300000 -> gh123[mmm]|abcdef[mm] */
    /* x1 = gh123000|00000000 */
    VECTOR_U16_128_A x1 = _mm_srli_epi16(x, 6);
    /* x2 = ????????|abcdefgh */
    VECTOR_U16_128_A x2 = _mm_bslli_si128(x, 1);
    /* x2 = 00000000|abcdef00 */
    x2 = x2 & broadcast_16_128(0x3f00);
    /* y = gh123000|abcdef00 */
    x = x1 | x2;
    /* y = gh123[mmm]|abcdef[mm] */
    x = x | broadcast_16_128(0x80c0);
    *(VECTOR_U16_128_U *)writer = x;
}

// force_inline void ucs2_encode_3bytes_utf8_ssse3(VECTOR_U16_128_A x, u8 *writer) {
//     /* abcdefgh|12345678 -> 5678[mmmm]|gh1234[mm]|abcdef[mm] */
// #    if __SSSE3__
//     // we need __SSSE3__ for _mm_shuffle_epi8
//     static const VECTOR_U8_128_A t1 = {
//             0x80, 0x80, 0,
//             0x80, 0x80, 2,
//             0x80, 0x80, 4,
//             0x80, 0x80, 6,
//             0x80, 0x80, 8,
//             0x80};
//     static const VECTOR_U8_128_A t2 = {
//             0x80, 0, 0x80,
//             0x80, 2, 0x80,
//             0x80, 4, 0x80,
//             0x80, 6, 0x80,
//             0x80, 8, 0x80,
//             0x80};
//     static const VECTOR_U8_128_A t3 = {
//             0, 0x80, 0x80,
//             2, 0x80, 0x80,
//             4, 0x80, 0x80,
//             6, 0x80, 0x80,
//             8, 0x80, 0x80,
//             10};
//     static const VECTOR_U8_128_A m1 = {
//             0xff, 0x3f, 0x3f,
//             0xff, 0x3f, 0x3f,
//             0xff, 0x3f, 0x3f,
//             0xff, 0x3f, 0x3f,
//             0xff, 0x3f, 0x3f,
//             0xff};
//     static const VECTOR_U8_128_A m2 = {
//             0xe0, 0x80, 0x80,
//             0xe0, 0x80, 0x80,
//             0xe0, 0x80, 0x80,
//             0xe0, 0x80, 0x80,
//             0xe0, 0x80, 0x80,
//             0xe0};
//     /* x1 = gh123456|78000000 */
//     VECTOR_U16_128_A x1 = _mm_srli_epi16(x, 6);
//     /* x2 = 56780000|00000000 */
//     VECTOR_U16_128_A x2 = _mm_srli_epi16(x, 12);
//     /* x3 = 00000000|00000000|abcdefgh */
//     VECTOR_U8_128_A x3 = _mm_shuffle_epi8(x, t1);
//     /* x4 = 00000000|gh123456|00000000 */
//     VECTOR_U8_128_A x4 = _mm_shuffle_epi8(x1, t2);
//     /* x5 = 56780000|00000000|00000000 */
//     VECTOR_U8_128_A x5 = _mm_shuffle_epi8(x2, t3);
//     /* x6 = 56780000|gh123456|abcdefgh */
//     VECTOR_U8_128_A x6 = x3 | x4 | x5;
//     /* x6 = 5678[mmmm]|gh1234[mm]|abcdef[mm] */
//     x6 = (x6 & m1) | m2;
//     /* want: gh1234[mm]|abcdef[mm]|5678[mmmm]|gh1234[mm]|... */
//     /* x = abcdefgh|12345678 */
//     x = _mm_bsrli_si128(x, 10);
//     /* x1 = gh123456|78000000 */
//     x1 = _mm_bsrli_si128(x1, 10);
//     /* x2 = 56780000|00000000 */
//     x2 = _mm_bsrli_si128(x2, 10);
//     /* x3 = gh123456|00000000|00000000|gh123456 */
//     x3 = _mm_shuffle_epi8(x1, t3);
//     /* x4 = 00000000|00000000|56780000|00000000 */
//     x4 = _mm_shuffle_epi8(x2, t1);
//     /* x5 = 00000000|abcdefgh|00000000|00000000 */
//     x5 = _mm_shuffle_epi8(x, t2);
//     /* x7 = gh123456|abcdefgh|56780000|gh123456 */
//     VECTOR_U8_128_A x7 = x3 | x4 | x5;
//     VECTOR_U8_128_A m3 = _mm_bsrli_si128(m1, 1);
//     VECTOR_U8_128_A m4 = _mm_bsrli_si128(m2, 1);
//     /* x7 = gh1234[mm]|abcdef[mm]|5678[mmmm]|gh1234[mm] */
//     x7 = (x7 & m3) | m4;
//     *(VECTOR_U8_128_U *)writer = x6;
//     *(VECTOR_U8_128_U *)(writer + 16) = x7;
// #    else
//     assert(false);
//     Py_UNREACHABLE();
// #    endif
// }

force_inline void ucs2_encode_3bytes_utf8_ssse3(VECTOR_U16_128_A x, u8 *writer) {
#    if __SSSE3__
    static const VECTOR_U8_128_A t1 = {
            0x80, 0x80, 0,
            0x80, 0x80, 4,
            0x80, 0x80, 8,
            0x80, 0x80, 12,
            0x80, 0x80, 0x80, 0x80};
    static const VECTOR_U8_128_A t2 = {
            0x80, 0, 0x80,
            0x80, 4, 0x80,
            0x80, 8, 0x80,
            0x80, 12, 0x80,
            0x80, 0x80, 0x80, 0x80};
    static const VECTOR_U8_128_A t3 = {
            0, 0x80, 0x80,
            4, 0x80, 0x80,
            8, 0x80, 0x80,
            12, 0x80, 0x80,
            0x80, 0x80, 0x80, 0x80};
    static const VECTOR_U8_128_A m1 = {
            0xff, 0x3f, 0x3f,
            0xff, 0x3f, 0x3f,
            0xff, 0x3f, 0x3f,
            0xff, 0x3f, 0x3f,
            0xff, 0xff, 0xff, 0xff};
    static const VECTOR_U8_128_A m2 = {
            0xe0, 0x80, 0x80,
            0xe0, 0x80, 0x80,
            0xe0, 0x80, 0x80,
            0xe0, 0x80, 0x80,
            0, 0, 0, 0};
    VECTOR_U32_128_A x1 = elevate_2_4_to_128(x);
    VECTOR_U32_128_A x2 = elevate_2_4_to_128(unpack_hi_64_128(x, x));
    VECTOR_U32_128_A x3 = _mm_srli_epi32(x1, 6);
    VECTOR_U32_128_A x4 = _mm_srli_epi32(x1, 12);
    VECTOR_U32_128_A x5 = _mm_srli_epi32(x2, 6);
    VECTOR_U32_128_A x6 = _mm_srli_epi32(x2, 12);
    VECTOR_U8_128_A x7 = _mm_shuffle_epi8(x1, t1);
    VECTOR_U8_128_A x8 = _mm_shuffle_epi8(x3, t2);
    VECTOR_U8_128_A x9 = _mm_shuffle_epi8(x4, t3);
    VECTOR_U8_128_A x10 = _mm_shuffle_epi8(x2, t1);
    VECTOR_U8_128_A x11 = _mm_shuffle_epi8(x5, t2);
    VECTOR_U8_128_A x12 = _mm_shuffle_epi8(x6, t3);
    VECTOR_U8_128_A x13 = ((x7 | x8 | x9) & m1) | m2;
    VECTOR_U8_128_A x14 = ((x10 | x11 | x12) & m1) | m2;
    VECTOR_U8_128_A x15 = _mm_alignr_epi8(x14, _mm_bslli_si128(x13, 4), 4);
    VECTOR_U8_128_A x16 = _mm_bsrli_si128(x14, 4);
    *(VECTOR_U8_128_U *)(writer + 0) = x15;
    *(VECTOR_U8_128_U *)(writer + 16) = x16;
#    else
    assert(false);
    Py_UNREACHABLE();
#    endif
}

force_inline void ucs4_encode_3bytes_utf8_ssse3(VECTOR_U32_128_A x, u8 *writer) {
#    if __SSSE3__
    static const VECTOR_U8_128_A t1 = {
            0x80, 0x80, 0,
            0x80, 0x80, 4,
            0x80, 0x80, 8,
            0x80, 0x80, 12,
            0x80, 0x80, 0x80, 0x80};
    static const VECTOR_U8_128_A t2 = {
            0x80, 0, 0x80,
            0x80, 4, 0x80,
            0x80, 8, 0x80,
            0x80, 12, 0x80,
            0x80, 0x80, 0x80, 0x80};
    static const VECTOR_U8_128_A t3 = {
            0, 0x80, 0x80,
            4, 0x80, 0x80,
            8, 0x80, 0x80,
            12, 0x80, 0x80,
            0x80, 0x80, 0x80, 0x80};
    static const VECTOR_U8_128_A m1 = {
            0xff, 0x3f, 0x3f,
            0xff, 0x3f, 0x3f,
            0xff, 0x3f, 0x3f,
            0xff, 0x3f, 0x3f,
            0xff, 0xff, 0xff, 0xff};
    static const VECTOR_U8_128_A m2 = {
            0xe0, 0x80, 0x80,
            0xe0, 0x80, 0x80,
            0xe0, 0x80, 0x80,
            0xe0, 0x80, 0x80,
            0, 0, 0, 0};
    VECTOR_U32_128_A x1 = _mm_srli_epi32(x, 6);
    VECTOR_U32_128_A x2 = _mm_srli_epi32(x, 12);
    VECTOR_U8_128_A x3 = _mm_shuffle_epi8(x, t1);
    VECTOR_U8_128_A x4 = _mm_shuffle_epi8(x1, t2);
    VECTOR_U8_128_A x5 = _mm_shuffle_epi8(x2, t3);
    VECTOR_U8_128_A x6 = ((x3 | x4 | x5) & m1) | m2;
    *(VECTOR_U8_128_U *)writer = x6;
#    else
    assert(false);
    Py_UNREACHABLE();
#    endif
}

/*==============================================================================
 * SSE4.1 only SIMD code
 *============================================================================*/
#    if __SSE4_1__
force_inline SIMD_128 blendv_128(SIMD_128 blend, SIMD_128 x, SIMD_128 mask) {
    return _mm_blendv_epi8(blend, x, mask);
}

/*
 * Write a tail to `addr` using blendv, keeping its head content.
 * [addr, addr + head_bytes) is left unchanged, while
 * [addr + head_bytes, addr + 128 / 8) is written, and
 * tailmask = load_128_aligned(read_tail_mask_table_8(head_bytes)).
 */
force_inline void blendv_writetail_128(SIMD_128 to_write, void *addr, SIMD_128 tailmask) {
    SIMD_128 blend_A = load_128(addr);
    SIMD_128 blended = blendv_128(blend_A, to_write, tailmask);
    write_128(addr, blended);
}
#    endif

/*==============================================================================
 * SSE4.2 only SIMD code
 *============================================================================*/
#    if __SSE4_2__
#    endif

/*==============================================================================
 * AVX only SIMD code
 *============================================================================*/
#    if __AVX__
force_inline SIMD_256 load_256(const void *src) {
    return _mm256_lddqu_si256((const SIMD_256_IU *)src);
}

force_inline SIMD_256 load_256_aligned(const void *src) {
    return _mm256_load_si256((const __m256i *)src);
}

force_inline void write_256(void *dst, SIMD_256 y) {
    _mm256_storeu_si256((SIMD_256_IU *)dst, y);
}

force_inline void write_256_aligned(void *dst, SIMD_256 y) {
    _mm256_store_si256((__m256i *)dst, y);
}

force_inline VECTOR_U8_256_A broadcast_8_256(i8 v) {
    return _mm256_set1_epi8(v);
}

force_inline VECTOR_U16_256_A broadcast_16_256(i16 v) {
    return _mm256_set1_epi16(v);
}

force_inline VECTOR_U32_256_A broadcast_32_256(i32 v) {
    return _mm256_set1_epi32(v);
}

force_inline bool testz_256(SIMD_256 y) {
    return (bool)_mm256_testz_si256(y, y);
}
#    endif

/*==============================================================================
 * AVX2 only SIMD code
 *============================================================================*/
#    if __AVX2__
force_inline VECTOR_U8_256_A cmpneq_8_256(VECTOR_U8_256_A a, VECTOR_U8_256_A b) {
    return _mm256_cmpeq_epi8(_mm256_cmpeq_epi8(a, b), _mm256_setzero_si256());
}

force_inline VECTOR_U16_256_A cmpneq_16_256(VECTOR_U16_256_A a, VECTOR_U16_256_A b) {
    return _mm256_cmpeq_epi16(_mm256_cmpeq_epi16(a, b), _mm256_setzero_si256());
}

force_inline VECTOR_U32_256_A cmpneq_32_256(VECTOR_U32_256_A a, VECTOR_U32_256_A b) {
    return _mm256_cmpeq_epi32(_mm256_cmpeq_epi32(a, b), _mm256_setzero_si256());
}

force_inline VECTOR_U16_256_A elevate_1_2_to_256(VECTOR_U8_128_A x) {
    return _mm256_cvtepu8_epi16(x);
}

force_inline VECTOR_U32_256_A elevate_1_4_to_256(VECTOR_U8_128_A x) {
    return _mm256_cvtepu8_epi32(x);
}

force_inline VECTOR_U32_256_A elevate_2_4_to_256(VECTOR_U16_128_A x) {
    return _mm256_cvtepu16_epi32(x);
}

force_inline SIMD_256 simd_and_256(SIMD_256 a, SIMD_256 b) {
    return _mm256_and_si256(a, b);
}

force_inline SIMD_256 simd_or_256(SIMD_256 a, SIMD_256 b) {
    return _mm256_or_si256(a, b);
}

force_inline u32 to_bitmask_256(SIMD_256 a) {
    int t = _mm256_movemask_epi8(a);
    return (u32)t;
}

force_inline VECTOR_U8_256_A cmpeq0_8_256(VECTOR_U8_256_A a) {
    return _mm256_cmpeq_epi8(a, _mm256_setzero_si256());
}

force_inline SIMD_256 blendv_256(SIMD_256 blend, SIMD_256 SIMD_VAR, SIMD_256 mask) {
    return _mm256_blendv_epi8(blend, SIMD_VAR, mask);
}

/*
 * Write a tail to `addr` using blendv, keeping its head content.
 * [addr, addr + head_bytes) is left unchanged, while
 * [addr + head_bytes, addr + 256 / 8) is written, and
 * tailmask = load_256_aligned(read_tail_mask_table_8(head_bytes)).
 */
force_inline void blendv_writetail_256(SIMD_256 to_write, void *addr, SIMD_256 tailmask) {
    SIMD_256 blend_A = load_256((const void *)addr);
    SIMD_256 blended = blendv_256(blend_A, to_write, tailmask);
    write_256(addr, blended);
}

force_inline void extract_256_two_parts(SIMD_256 y, SIMD_128 *restrict x1, SIMD_128 *restrict x2) {
    *x1 = _mm256_extracti128_si256(y, 0);
    *x2 = _mm256_extracti128_si256(y, 1);
}

force_inline void extract_256_four_parts(SIMD_256 y, SIMD_128 *restrict x1, SIMD_128 *restrict x2, SIMD_128 *restrict x3, SIMD_128 *restrict x4) {
    extract_256_two_parts(y, x1, x3);
    *x2 = unpack_hi_64_128(*x1, *x1);
    *x4 = unpack_hi_64_128(*x3, *x3);
}

force_inline VECTOR_U32_256_A cmpgt_i32_256(VECTOR_U32_256_A a, VECTOR_U32_256_A b) {
    return _mm256_cmpgt_epi32(a, b);
}

force_inline VECTOR_U8_128_A zip_256_16_to_8(VECTOR_U16_256_A y) {
    __m128i x_low = _mm256_extracti128_si256(y, 0);
    __m128i x_high = _mm256_extracti128_si256(y, 1);
    return _mm_packus_epi16(x_low, x_high);
}

force_inline u64 zip_256_32_to_8(VECTOR_U32_256_A y) {
    /*y = axxxbxxxcxxxdxxx|exxxfxxxgxxxhxxx */
    pyyjson_align(64) static const u8 t1[32] = {0, 4, 8, 12,
                                                0x80, 0x80,
                                                0x80, 0x80,
                                                0x80, 0x80,
                                                0x80, 0x80,
                                                0x80, 0x80,
                                                0x80, 0x80,
                                                // seperate
                                                0x80, 0x80,
                                                0x80, 0x80,
                                                0, 4, 8, 12,
                                                0x80, 0x80,
                                                0x80, 0x80,
                                                0x80, 0x80,
                                                0x80, 0x80};
    /* y2 = abcd000000000000|0000efgh00000000 */
    __m256i y2 = _mm256_shuffle_epi8(y, load_256_aligned(t1));
    /* x_high = 0000efgh00000000 */
    i64 i1 = _mm256_extract_epi64(y2, 0);
    i64 i2 = _mm256_extract_epi64(y2, 2);
    return (u64)(i1 | i2);
}

force_inline VECTOR_U16_128_A zip_256_32_to_16(VECTOR_U32_256_A y) {
    __m128i x_low = _mm256_extracti128_si256(y, 0);
    __m128i x_high = _mm256_extracti128_si256(y, 1);
    return _mm_packus_epi32(x_low, x_high);
}

force_inline void ucs2_encode_3bytes_utf8_avx2(VECTOR_U16_256_A y, u8 *writer) {
    /* abcdefgh|12345678 -> 5678[mmmm]|gh1234[mm]|abcdef[mm] */
    VECTOR_U8_256_A t1 = {
            0x80, 0x80, 0,
            0x80, 0x80, 2,
            0x80, 0x80, 4,
            0x80, 0x80, 6,
            0x80, 0x80, 8,
            0x80,
            //
            4, 0x80, 0x80,
            6, 0x80, 0x80,
            8, 0x80, 0x80,
            10, 0x80, 0x80,
            12, 0x80, 0x80,
            14};
    VECTOR_U8_256_A t2 = {
            0x80, 0, 0x80,
            0x80, 2, 0x80,
            0x80, 4, 0x80,
            0x80, 6, 0x80,
            0x80, 8, 0x80,
            0x80,
            //
            0x80, 0x80, 6,
            0x80, 0x80, 8,
            0x80, 0x80, 10,
            0x80, 0x80, 12,
            0x80, 0x80, 14,
            0x80};
    VECTOR_U8_256_A t3 = {
            0, 0x80, 0x80,
            2, 0x80, 0x80,
            4, 0x80, 0x80,
            6, 0x80, 0x80,
            8, 0x80, 0x80,
            10,
            //
            0x80, 6, 0x80,
            0x80, 8, 0x80,
            0x80, 10, 0x80,
            0x80, 12, 0x80,
            0x80, 14, 0x80,
            0x80};
    VECTOR_U8_256_A m1 = {
            0xff, 0x3f, 0x3f,
            0xff, 0x3f, 0x3f,
            0xff, 0x3f, 0x3f,
            0xff, 0x3f, 0x3f,
            0xff, 0x3f, 0x3f,
            0xff,
            //
            0x3f, 0xff, 0x3f,
            0x3f, 0xff, 0x3f,
            0x3f, 0xff, 0x3f,
            0x3f, 0xff, 0x3f,
            0x3f, 0xff, 0x3f,
            0x3f};
    VECTOR_U8_256_A m2 = {
            0xe0, 0x80, 0x80,
            0xe0, 0x80, 0x80,
            0xe0, 0x80, 0x80,
            0xe0, 0x80, 0x80,
            0xe0, 0x80, 0x80,
            0xe0,
            //
            0x80, 0xe0, 0x80,
            0x80, 0xe0, 0x80,
            0x80, 0xe0, 0x80,
            0x80, 0xe0, 0x80,
            0x80, 0xe0, 0x80,
            0x80};
    /* y1 = gh123456|78000000 */
    VECTOR_U16_256_A y1 = _mm256_srli_epi16(y, 6);
    /* y2 = 56780000|00000000 */
    VECTOR_U16_256_A y2 = _mm256_srli_epi16(y, 12);
    /* y3 = 00000000|00000000|abcdefgh */
    VECTOR_U8_256_A y3 = _mm256_shuffle_epi8(y, t1);
    /* y4 = 00000000|gh123456|00000000 */
    VECTOR_U8_256_A y4 = _mm256_shuffle_epi8(y1, t2);
    /* y5 = 56780000|00000000|00000000 */
    VECTOR_U8_256_A y5 = _mm256_shuffle_epi8(y2, t3);
    /* y6 = 56780000|gh123456|abcdefgh */
    VECTOR_U8_256_A y6 = y3 | y4 | y5;
    /* y6 = 5678[mmmm]|gh1234[mm]|abcdef[mm] */
    y6 = (y6 & m1) | m2;
    VECTOR_U16_128_A x, x1, x2;
    {
        VECTOR_U8_128_A _x1, _x2;
        _x1 = _mm256_extracti128_si256(y, 0);
        _x2 = _mm256_extracti128_si256(y, 1);
        x = _mm_alignr_epi8(_x2, _x1, 10);
    }
    {
        VECTOR_U8_128_A _x1, _x2;
        _x1 = _mm256_extracti128_si256(y1, 0);
        _x2 = _mm256_extracti128_si256(y1, 1);
        x1 = _mm_alignr_epi8(_x2, _x1, 10);
    }
    {
        VECTOR_U8_128_A _x1, _x2;
        _x1 = _mm256_extracti128_si256(y2, 0);
        _x2 = _mm256_extracti128_si256(y2, 1);
        x2 = _mm_alignr_epi8(_x2, _x1, 12);
    }
    /* x3 = gh123456|00000000|00000000 */
    VECTOR_U8_128_A x3 = _mm_shuffle_epi8(x1, _mm256_extracti128_si256(t3, 0));
    /* x4 = 00000000|abcdefgh|00000000 */
    VECTOR_U8_128_A x4 = _mm_shuffle_epi8(x, _mm256_extracti128_si256(t2, 0));
    /* x5 = 00000000|00000000|56780000 */
    VECTOR_U8_128_A x5 = _mm_shuffle_epi8(x2, _mm256_extracti128_si256(t1, 0));
    /* x6 = gh123456|abcdefgh|56780000 */
    VECTOR_U8_128_A x6 = x3 | x4 | x5;
    VECTOR_U8_128_A mx1, mx2;
    {
        VECTOR_U8_128_A _m1, _m2;
        _m1 = _mm256_extracti128_si256(m1, 0);
        _m2 = _mm256_extracti128_si256(m1, 1);
        mx1 = _mm_alignr_epi8(_m2, _m1, 1);
    }
    {
        VECTOR_U8_128_A _m1, _m2;
        _m1 = _mm256_extracti128_si256(m2, 0);
        _m2 = _mm256_extracti128_si256(m2, 1);
        mx2 = _mm_alignr_epi8(_m2, _m1, 1);
    }
    VECTOR_U8_128_A x7 = (x6 & mx1) | mx2;
    *(VECTOR_U8_128_U *)(writer + 0) = _mm256_extracti128_si256(y6, 0);
    *(VECTOR_U8_128_U *)(writer + 16) = x7;
    *(VECTOR_U8_128_U *)(writer + 32) = _mm256_extracti128_si256(y6, 1);
}

// /* Read: 32 bytes (16 u16). Write: 56 bytes. Valid in result: 48 bytes. */
// force_inline void ucs2_encode_3bytes_utf8_avx2(u16 *read_in, u8 *writer) {
//     /* abcdefgh|12345678 */
//     VECTOR_U16_256_A _y;
//     memcpy(&_y, read_in, sizeof(_y));
//     VECTOR_U16_256_A y[2];
//     VECTOR_U16_128_A x_low = _mm256_extracti128_si256(_y, 0);
//     VECTOR_U16_128_A x_high = _mm256_extracti128_si256(_y, 1);
//     y[0] = _mm256_set_m128i(x_low, x_low);
//     y[1] = _mm256_set_m128i(x_high, x_high);
//     pyyjson_align(32) static const u8 t1[32] = {
//             0x80, 0x80, 0,
//             0x80, 0x80, 2,
//             0x80, 0x80, 4,
//             0x80, 0x80, 6,
//             0x80, 0x80, 8,
//             0x80, 0x80, 10,
//             0x80, 0x80, 12,
//             0x80, 0x80, 14,
//             0x80, 0x80, 0x80,
//             0x80, 0x80, 0x80,
//             0x80, 0x80};
//     pyyjson_align(32) static const u8 t2[32] = {
//             0x80, 0, 0x80,
//             0x80, 2, 0x80,
//             0x80, 4, 0x80,
//             0x80, 6, 0x80,
//             0x80, 8, 0x80,
//             0x80, 10, 0x80,
//             0x80, 12, 0x80,
//             0x80, 14, 0x80,
//             0x80, 0x80, 0x80,
//             0x80, 0x80, 0x80,
//             0x80, 0x80};
//     pyyjson_align(32) static const u8 t3[32] = {
//             0, 0x80, 0x80,
//             2, 0x80, 0x80,
//             4, 0x80, 0x80,
//             6, 0x80, 0x80,
//             8, 0x80, 0x80,
//             10, 0x80, 0x80,
//             12, 0x80, 0x80,
//             14, 0x80, 0x80,
//             0x80, 0x80, 0x80,
//             0x80, 0x80, 0x80,
//             0x80, 0x80};
//     pyyjson_align(32) static const u8 m1[32] = {
//             0xff, 0x3f, 0x3f,
//             0xff, 0x3f, 0x3f,
//             0xff, 0x3f, 0x3f,
//             0xff, 0x3f, 0x3f,
//             0xff, 0x3f, 0x3f,
//             0xff, 0x3f, 0x3f,
//             0xff, 0x3f, 0x3f,
//             0xff, 0x3f, 0x3f,
//             0, 0, 0, 0,
//             0, 0, 0, 0};
//     pyyjson_align(32) static const u8 m2[32] = {
//             0xe0, 0x80, 0x80,
//             0xe0, 0x80, 0x80,
//             0xe0, 0x80, 0x80,
//             0xe0, 0x80, 0x80,
//             0xe0, 0x80, 0x80,
//             0xe0, 0x80, 0x80,
//             0xe0, 0x80, 0x80,
//             0xe0, 0x80, 0x80,
//             0, 0, 0, 0,
//             0, 0, 0, 0};
//     for (int i = 0; i < 2; ++i) {
//         SIMD_256 z = y[i];
//         SIMD_256 z1, z2, z3;
//         /*z1 = 00000000|00000000|abcdefgh */
//         z1 = _mm256_shuffle_epi8(z, *(const SIMD_256 *)t1);
//         /*z2 = gh123456|78000000 */
//         z2 = _mm256_srli_epi16(z, 6);
//         /*z2 = 00000000|gh123456|00000000 */
//         z2 = _mm256_shuffle_epi8(z2, *(const SIMD_256 *)t2);
//         /*z3 = 56780000|00000000 */
//         z3 = _mm256_srli_epi16(z, 12);
//         /*z3 = 56780000|00000000|00000000 */
//         z3 = _mm256_shuffle_epi8(z3, *(const SIMD_256 *)t3);
//         /*z = 56780000|gh123456|abcdefgh */
//         z = _mm256_or_si256(z1, _mm256_or_si256(z2, z3));
//         /*z = 56780000|gh123400|abcdef00 */
//         z = _mm256_and_si256(z, *(const SIMD_256 *)m1);
//         // 5678[mmmm]|gh1234[mm]|abcdef[mm]
//         z = _mm256_or_si256(z, *(const SIMD_256 *)m2);
//         _mm256_storeu_si256((void *)writer, z);
//         writer += 24;
//     }
// }

/* Read: 20 bytes (10 u16). Write: 32 bytes. Valid in result: 30 bytes. */
// force_inline void ucs2_encode_3bytes_utf8_avx2_v2(u16 *read_in, u8 *writer) {
//     /* abcdefgh|12345678 */
//     VECTOR_U16_256_A y;
//     memcpy(((VECTOR_U16_128_A *)&y) + 1, read_in + 4, 12); // max read 20 bytes
//     memcpy(((VECTOR_U16_128_A *)&y) + 0, read_in + 0, sizeof(VECTOR_U16_128_A));
//     pyyjson_align(32) static const u8 t1[32] = {
//             0x80, 0x80, 0,
//             0x80, 0x80, 2,
//             0x80, 0x80, 4,
//             0x80, 0x80, 6,
//             0x80, 0x80, 8,
//             0x80, 0x80, 2,
//             0x80, 0x80, 4,
//             0x80, 0x80, 6,
//             0x80, 0x80, 8,
//             0x80, 0x80, 10,
//             0x80, 0x80};
//     pyyjson_align(32) static const u8 t2[32] = {
//             0x80, 0, 0x80,
//             0x80, 2, 0x80,
//             0x80, 4, 0x80,
//             0x80, 6, 0x80,
//             0x80, 8, 0x80,
//             0x80, 2, 0x80,
//             0x80, 4, 0x80,
//             0x80, 6, 0x80,
//             0x80, 8, 0x80,
//             0x80, 10, 0x80,
//             0x80, 0x80};
//     pyyjson_align(32) static const u8 t3[32] = {
//             0, 0x80, 0x80,
//             2, 0x80, 0x80,
//             4, 0x80, 0x80,
//             6, 0x80, 0x80,
//             8, 0x80, 0x80,
//             10, 0x80, 0x80,
//             4, 0x80, 0x80,
//             6, 0x80, 0x80,
//             8, 0x80, 0x80,
//             10, 0x80, 0x80,
//             0x80, 0x80};
//     pyyjson_align(32) static const u8 m1[32] = {
//             0xff, 0x3f, 0x3f,
//             0xff, 0x3f, 0x3f,
//             0xff, 0x3f, 0x3f,
//             0xff, 0x3f, 0x3f,
//             0xff, 0x3f, 0x3f,
//             0xff, 0x3f, 0x3f,
//             0xff, 0x3f, 0x3f,
//             0xff, 0x3f, 0x3f,
//             0xff, 0x3f, 0x3f,
//             0xff, 0x3f, 0x3f,
//             0, 0};
//     pyyjson_align(32) static const u8 m2[32] = {
//             0xe0, 0x80, 0x80,
//             0xe0, 0x80, 0x80,
//             0xe0, 0x80, 0x80,
//             0xe0, 0x80, 0x80,
//             0xe0, 0x80, 0x80,
//             0xe0, 0x80, 0x80,
//             0xe0, 0x80, 0x80,
//             0xe0, 0x80, 0x80,
//             0xe0, 0x80, 0x80,
//             0xe0, 0x80, 0x80,
//             0, 0};
//     SIMD_256 y1, y2, y3;
//     /*y1 = 00000000|00000000|abcdefgh */
//     y1 = _mm256_shuffle_epi8(y, *(const SIMD_256 *)t1);
//     /*y2 = gh123456|78000000 */
//     y2 = _mm256_srli_epi16(y, 6);
//     /*y2 = 00000000|gh123456|00000000 */
//     y2 = _mm256_shuffle_epi8(y2, *(const SIMD_256 *)t2);
//     /*y3 = 56780000|00000000 */
//     y3 = _mm256_srli_epi16(y, 12);
//     /*y3 = 56780000|00000000|00000000 */
//     y3 = _mm256_shuffle_epi8(y3, *(const SIMD_256 *)t3);
//     /*y = 56780000|gh123456|abcdefgh */
//     y = _mm256_or_si256(y1, _mm256_or_si256(y2, y3));
//     /*y = 56780000|gh123400|abcdef00 */
//     y = _mm256_and_si256(y, *(const SIMD_256 *)m1);
//     // 5678[mmmm]|gh1234[mm]|abcdef[mm]
//     y = _mm256_or_si256(y, *(const SIMD_256 *)m2);
//     _mm256_storeu_si256((void *)writer, y);
// }

force_inline void ucs2_encode_2bytes_utf8_avx2(VECTOR_U16_256_A y, u8 *writer) {
    /* abcdefgh|12300000 -> gh123[mmm]|abcdef[mm] */
    pyyjson_align(16) static const u8 t1[16] = {
            0x80, 0,
            0x80, 2,
            0x80, 4,
            0x80, 6,
            0x80, 8,
            0x80, 10,
            0x80, 12,
            0x80, 14};
    /*y1 = gh123000|00000000 */
    VECTOR_U16_256_A y1 = _mm256_srli_epi16(y, 6);
    /*y2 = 00000000|abcdefgh */
    VECTOR_U16_256_A y2 = _mm256_shuffle_epi8(y, _mm256_broadcastsi128_si256(*(const SIMD_128 *)t1));
    /*y = gh123000|abcdefgh */
    y = y1 | y2;
    /*y = gh123000|abcdef00 */
    y = y & broadcast_16_256(0x3fff);
    /*y = gh123[mmm]|abcdef[mm] */
    y = y | broadcast_16_256(0x80c0);
    *(VECTOR_U16_256_U *)writer = y;
}

force_inline void ucs4_encode_3bytes_utf8_avx2(VECTOR_U32_256_A y, u8 *writer) {
    /* abcdefgh|12345678|00000000|00000000 -> 5678[mmmm]|gh1234[mm]|abcdef[mm] */
    VECTOR_U8_256_A t1 = {
            0x80, 0x80, 0x80, 0x80,
            0x80, 0x80, 0,
            0x80, 0x80, 4,
            0x80, 0x80, 8,
            0x80, 0x80, 12,
            //
            0x80, 0x80, 0,
            0x80, 0x80, 4,
            0x80, 0x80, 8,
            0x80, 0x80, 12,
            0x80, 0x80, 0x80, 0x80};
    VECTOR_U8_256_A t2 = {
            0x80, 0x80, 0x80, 0x80,
            0x80, 0, 0x80,
            0x80, 4, 0x80,
            0x80, 8, 0x80,
            0x80, 12, 0x80,
            //
            0x80, 0, 0x80,
            0x80, 4, 0x80,
            0x80, 8, 0x80,
            0x80, 12, 0x80,
            0x80, 0x80, 0x80, 0x80};
    VECTOR_U8_256_A t3 = {
            0x80, 0x80, 0x80, 0x80,
            0, 0x80, 0x80,
            4, 0x80, 0x80,
            8, 0x80, 0x80,
            12, 0x80, 0x80,
            //
            0, 0x80, 0x80,
            4, 0x80, 0x80,
            8, 0x80, 0x80,
            12, 0x80, 0x80,
            0x80, 0x80, 0x80, 0x80};
    VECTOR_U8_256_A m1 = {
            0xff, 0xff, 0xff, 0xff,
            0xff, 0x3f, 0x3f,
            0xff, 0x3f, 0x3f,
            0xff, 0x3f, 0x3f,
            0xff, 0x3f, 0x3f,
            //
            0xff, 0x3f, 0x3f,
            0xff, 0x3f, 0x3f,
            0xff, 0x3f, 0x3f,
            0xff, 0x3f, 0x3f,
            0xff, 0xff, 0xff, 0xff};
    VECTOR_U8_256_A m2 = {
            0, 0, 0, 0,
            0xe0, 0x80, 0x80,
            0xe0, 0x80, 0x80,
            0xe0, 0x80, 0x80,
            0xe0, 0x80, 0x80,
            //
            0xe0, 0x80, 0x80,
            0xe0, 0x80, 0x80,
            0xe0, 0x80, 0x80,
            0xe0, 0x80, 0x80,
            0, 0, 0, 0};
    VECTOR_U32_256_A mw = {0, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0};
    /* y1 = gh123456|78000000|00000000|00000000 */
    VECTOR_U32_256_A y1 = _mm256_srli_epi32(y, 6);
    /* y2 = 56780000|00000000|00000000|00000000 */
    VECTOR_U32_256_A y2 = _mm256_srli_epi32(y, 12);

    /* y3 = 00000000|00000000|abcdefgh */
    VECTOR_U8_256_A y3 = _mm256_shuffle_epi8(y, t1);
    /* y4 = 00000000|gh123456|00000000 */
    VECTOR_U8_256_A y4 = _mm256_shuffle_epi8(y1, t2);
    /* y5 = 56780000|00000000|00000000 */
    VECTOR_U8_256_A y5 = _mm256_shuffle_epi8(y2, t3);
    VECTOR_U8_256_A y6 = ((y3 | y4 | y5) & m1) | m2;
    _mm256_maskstore_epi32(PYYJSON_CAST(int *, writer - 4), mw, y6);
}

#    endif

/*==============================================================================
 * AVX512F only SIMD code
 *============================================================================*/
#    if __AVX512F__
force_inline SIMD_512 load_512(const void *src) {
    return _mm512_loadu_si512(src);
}

force_inline SIMD_512 load_512_aligned(const void *src) {
    return _mm512_load_si512(src);
}

force_inline void write_512(void *dst, SIMD_512 z) {
    _mm512_storeu_si512(dst, z);
}

force_inline void write_512_aligned(void *dst, SIMD_512 z) {
    _mm512_store_si512(dst, z);
}

force_inline SIMD_512 simd_and_512(SIMD_512 a, SIMD_512 b) {
    return _mm512_and_si512(a, b);
}

force_inline u16 cmpneq_32_512(VECTOR_U32_512_A a, VECTOR_U32_512_A b) {
    return (u16)_mm512_cmpneq_epi32_mask(a, b);
}

force_inline VECTOR_U8_512_A broadcast_8_512(i8 v) {
    return _mm512_set1_epi8(v);
}

force_inline VECTOR_U16_512_A broadcast_16_512(i16 v) {
    return _mm512_set1_epi16(v);
}

force_inline VECTOR_U32_512_A broadcast_32_512(i32 v) {
    return _mm512_set1_epi32(v);
}

force_inline VECTOR_U32_512_A elevate_2_4_to_512(VECTOR_U16_256_A y) {
    return _mm512_cvtepu16_epi32(y);
}

force_inline VECTOR_U32_512_A elevate_1_4_to_512(VECTOR_U8_128_A x) {
    return _mm512_cvtepu8_epi32(x);
}

force_inline void extract_512_two_parts(SIMD_512 z, SIMD_256 *restrict y1, SIMD_256 *restrict y2) {
    *y1 = _mm512_extracti64x4_epi64(z, 0);
    *y2 = _mm512_extracti64x4_epi64(z, 1);
}

force_inline void extract_512_four_parts(SIMD_512 z, SIMD_128 *restrict x1, SIMD_128 *restrict x2, SIMD_128 *restrict x3, SIMD_128 *restrict x4) {
    *x1 = _mm512_extracti32x4_epi32(z, 0);
    *x2 = _mm512_extracti32x4_epi32(z, 1);
    *x3 = _mm512_extracti32x4_epi32(z, 2);
    *x4 = _mm512_extracti32x4_epi32(z, 3);
}
#    endif

/*==============================================================================
 * AVX512BW only SIMD code
 *============================================================================*/
#    if __AVX512BW__
force_inline VECTOR_U16_512_A elevate_1_2_to_512(VECTOR_U8_256_A y) {
    return _mm512_cvtepu8_epi16(y);
}

force_inline u64 cmpneq_8_512(VECTOR_U8_512_A a, VECTOR_U8_512_A b) {
    return (u64)_mm512_cmpneq_epi8_mask(a, b);
}

force_inline u32 cmpneq_16_512(VECTOR_U16_512_A a, VECTOR_U16_512_A b) {
    return (u32)_mm512_cmpneq_epi16_mask(a, b);
}
#    endif

/*==============================================================================
 * AVX512F && AVX512BW only SIMD code
 *============================================================================*/
#    if __AVX512F__ && __AVX512BW__
force_inline void ucs2_encode_3bytes_utf8_avx512(VECTOR_U16_512_A z, u8 *writer) {
    VECTOR_U8_512_A t1 = {
            0x80, 0x80, 0,
            0x80, 0x80, 2,
            0x80, 0x80, 4,
            0x80, 0x80, 6,
            0x80, 0x80, 8,
            0x80,
            //
            4, 0x80, 0x80,
            6, 0x80, 0x80,
            8, 0x80, 0x80,
            10, 0x80, 0x80,
            12, 0x80, 0x80,
            14,
            //
            0x80, 0x80, 0,
            0x80, 0x80, 2,
            0x80, 0x80, 4,
            0x80, 0x80, 6,
            0x80, 0x80, 8,
            0x80,
            //
            4, 0x80, 0x80,
            6, 0x80, 0x80,
            8, 0x80, 0x80,
            10, 0x80, 0x80,
            12, 0x80, 0x80,
            14};
    VECTOR_U8_512_A t2 = {
            0x80, 0, 0x80,
            0x80, 2, 0x80,
            0x80, 4, 0x80,
            0x80, 6, 0x80,
            0x80, 8, 0x80,
            0x80,
            //
            0x80, 0x80, 6,
            0x80, 0x80, 8,
            0x80, 0x80, 10,
            0x80, 0x80, 12,
            0x80, 0x80, 14,
            0x80,
            //
            0x80, 0, 0x80,
            0x80, 2, 0x80,
            0x80, 4, 0x80,
            0x80, 6, 0x80,
            0x80, 8, 0x80,
            0x80,
            //
            0x80, 0x80, 6,
            0x80, 0x80, 8,
            0x80, 0x80, 10,
            0x80, 0x80, 12,
            0x80, 0x80, 14,
            0x80};
    VECTOR_U8_512_A t3 = {
            0, 0x80, 0x80,
            2, 0x80, 0x80,
            4, 0x80, 0x80,
            6, 0x80, 0x80,
            8, 0x80, 0x80,
            10,
            //
            0x80, 6, 0x80,
            0x80, 8, 0x80,
            0x80, 10, 0x80,
            0x80, 12, 0x80,
            0x80, 14, 0x80,
            0x80,
            //
            0, 0x80, 0x80,
            2, 0x80, 0x80,
            4, 0x80, 0x80,
            6, 0x80, 0x80,
            8, 0x80, 0x80,
            10,
            //
            0x80, 6, 0x80,
            0x80, 8, 0x80,
            0x80, 10, 0x80,
            0x80, 12, 0x80,
            0x80, 14, 0x80,
            0x80};
    VECTOR_U8_512_A m1 = {
            0xff, 0x3f, 0x3f,
            0xff, 0x3f, 0x3f,
            0xff, 0x3f, 0x3f,
            0xff, 0x3f, 0x3f,
            0xff, 0x3f, 0x3f,
            0xff,
            //
            0x3f, 0xff, 0x3f,
            0x3f, 0xff, 0x3f,
            0x3f, 0xff, 0x3f,
            0x3f, 0xff, 0x3f,
            0x3f, 0xff, 0x3f,
            0x3f,
            //
            0xff, 0x3f, 0x3f,
            0xff, 0x3f, 0x3f,
            0xff, 0x3f, 0x3f,
            0xff, 0x3f, 0x3f,
            0xff, 0x3f, 0x3f,
            0xff,
            //
            0x3f, 0xff, 0x3f,
            0x3f, 0xff, 0x3f,
            0x3f, 0xff, 0x3f,
            0x3f, 0xff, 0x3f,
            0x3f, 0xff, 0x3f,
            0x3f};
    VECTOR_U8_512_A m2 = {
            0xe0, 0x80, 0x80,
            0xe0, 0x80, 0x80,
            0xe0, 0x80, 0x80,
            0xe0, 0x80, 0x80,
            0xe0, 0x80, 0x80,
            0xe0,
            //
            0x80, 0xe0, 0x80,
            0x80, 0xe0, 0x80,
            0x80, 0xe0, 0x80,
            0x80, 0xe0, 0x80,
            0x80, 0xe0, 0x80,
            0x80,
            //
            0xe0, 0x80, 0x80,
            0xe0, 0x80, 0x80,
            0xe0, 0x80, 0x80,
            0xe0, 0x80, 0x80,
            0xe0, 0x80, 0x80,
            0xe0,
            //
            0x80, 0xe0, 0x80,
            0x80, 0xe0, 0x80,
            0x80, 0xe0, 0x80,
            0x80, 0xe0, 0x80,
            0x80, 0xe0, 0x80,
            0x80};
    /* z1 = gh123456|78000000 */
    VECTOR_U16_512_A z1 = _mm512_srli_epi16(z, 6);
    /* z2 = 56780000|00000000 */
    VECTOR_U16_512_A z2 = _mm512_srli_epi16(z, 12);
    /* z3 = 00000000|00000000|abcdefgh */
    VECTOR_U8_512_A z3 = _mm512_shuffle_epi8(z, t1);
    /* z4 = 00000000|gh123456|00000000 */
    VECTOR_U8_512_A z4 = _mm512_shuffle_epi8(z1, t2);
    /* z5 = 56780000|00000000|00000000 */
    VECTOR_U8_512_A z5 = _mm512_shuffle_epi8(z2, t3);
    /* z6 = 56780000|gh123456|abcdefgh */
    VECTOR_U8_512_A z6 = z3 | z4 | z5;
    /* z7 = 5678[mmmm]|gh1234[mm]|abcdef[mm] */
    VECTOR_U8_512_A z7 = (z6 & m1) | m2;
    //
    VECTOR_U16_256_A y, y1, y2;
    {
        SIMD_128 _x1, _x2, _x3, _x4;
        extract_512_four_parts(z, &_x1, &_x2, &_x3, &_x4);
        y = _mm256_set_m128i(_mm_alignr_epi8(_x4, _x3, 10), _mm_alignr_epi8(_x2, _x1, 10));
    }
    {
        SIMD_128 _x1, _x2, _x3, _x4;
        extract_512_four_parts(z1, &_x1, &_x2, &_x3, &_x4);
        y1 = _mm256_set_m128i(_mm_alignr_epi8(_x4, _x3, 10), _mm_alignr_epi8(_x2, _x1, 10));
    }
    {
        SIMD_128 _x1, _x2, _x3, _x4;
        extract_512_four_parts(z2, &_x1, &_x2, &_x3, &_x4);
        y2 = _mm256_set_m128i(_mm_alignr_epi8(_x4, _x3, 12), _mm_alignr_epi8(_x2, _x1, 12));
    }
    /* x3 = gh123456|00000000|00000000 */
    VECTOR_U8_128_A _t3 = _mm512_extracti32x4_epi32(t3, 0);
    VECTOR_U8_256_A y3 = _mm256_shuffle_epi8(y1, _mm256_set_m128i(_t3, _t3));
    /* x4 = 00000000|abcdefgh|00000000 */
    VECTOR_U8_128_A _t2 = _mm512_extracti32x4_epi32(t2, 0);
    VECTOR_U8_256_A y4 = _mm256_shuffle_epi8(y, _mm256_set_m128i(_t2, _t2));
    /* x5 = 00000000|00000000|56780000 */
    VECTOR_U8_128_A _t1 = _mm512_extracti32x4_epi32(t1, 0);
    VECTOR_U8_256_A y5 = _mm256_shuffle_epi8(y2, _mm256_set_m128i(_t1, _t1));
    /* x6 = gh123456|abcdefgh|56780000 */
    VECTOR_U8_256_A y6 = y3 | y4 | y5;
    VECTOR_U8_256_A my1, my2;
    {
        SIMD_128 _x1, _x2, _x3, _x4;
        extract_512_four_parts(m1, &_x1, &_x2, &_x3, &_x4);
        my1 = _mm256_set_m128i(_mm_alignr_epi8(_x4, _x3, 1), _mm_alignr_epi8(_x2, _x1, 1));
    }
    {
        SIMD_128 _x1, _x2, _x3, _x4;
        extract_512_four_parts(m2, &_x1, &_x2, &_x3, &_x4);
        my2 = _mm256_set_m128i(_mm_alignr_epi8(_x4, _x3, 1), _mm_alignr_epi8(_x2, _x1, 1));
    }
    VECTOR_U8_256_A y7 = (y6 & my1) | my2;
    SIMD_128 w1, w2, w3, w4, w5, w6;
    extract_256_two_parts(y7, &w2, &w5);
    extract_512_four_parts(z7, &w1, &w3, &w4, &w6);
    *(VECTOR_U8_128_U *)(writer + 0) = w1;
    *(VECTOR_U8_128_U *)(writer + 16) = w2;
    *(VECTOR_U8_128_U *)(writer + 32) = w3;
    *(VECTOR_U8_128_U *)(writer + 48) = w4;
    *(VECTOR_U8_128_U *)(writer + 64) = w5;
    *(VECTOR_U8_128_U *)(writer + 80) = w6;
}

// /* Read: 42 bytes (21 u16). Write: 64 bytes. Valid in result: 63 bytes. */
// force_inline void ucs2_encode_3bytes_utf8_avx512(u16 *read_in, u8 *writer) {
//     VECTOR_U16_512_A z;
//     memcpy(((VECTOR_U16_128_A *)&z) + 3, read_in + 16, 10); // max read 42 bytes!
//     memcpy(((VECTOR_U16_128_A *)&z) + 2, read_in + 10, sizeof(VECTOR_U16_128_A));
//     memcpy(((VECTOR_U16_128_A *)&z) + 1, read_in + 4, sizeof(VECTOR_U16_128_A));
//     memcpy(((VECTOR_U16_128_A *)&z) + 0, read_in + 0, sizeof(VECTOR_U16_128_A));
//     pyyjson_align(64) static const u8 t1[64] = {
//             0x80, 0x80, 0,
//             0x80, 0x80, 2,
//             0x80, 0x80, 4,
//             0x80, 0x80, 6,
//             0x80, 0x80, 8,
//             0x80, 0x80, 2,
//             0x80, 0x80, 4,
//             0x80, 0x80, 6,
//             0x80, 0x80, 8,
//             0x80, 0x80, 10,
//             0x80, 0x80, 0,
//             0x80, 0x80, 2,
//             0x80, 0x80, 4,
//             0x80, 0x80, 6,
//             0x80, 0x80, 8,
//             0x80, 0x80, 10,
//             0x80, 0x80, 0,
//             0x80, 0x80, 2,
//             0x80, 0x80, 4,
//             0x80, 0x80, 6,
//             0x80, 0x80, 8,
//             0x80};
//     pyyjson_align(64) static const u8 t2[64] = {
//             0x80, 0, 0x80,
//             0x80, 2, 0x80,
//             0x80, 4, 0x80,
//             0x80, 6, 0x80,
//             0x80, 8, 0x80,
//             0x80, 2, 0x80,
//             0x80, 4, 0x80,
//             0x80, 6, 0x80,
//             0x80, 8, 0x80,
//             0x80, 10, 0x80,
//             0x80, 12, 0x80,
//             0x80, 2, 0x80,
//             0x80, 4, 0x80,
//             0x80, 6, 0x80,
//             0x80, 8, 0x80,
//             0x80, 10, 0x80,
//             0x80, 0, 0x80,
//             0x80, 2, 0x80,
//             0x80, 4, 0x80,
//             0x80, 6, 0x80,
//             0x80, 8, 0x80,
//             0x80};
//     pyyjson_align(64) static const u8 t3[64] = {
//             0, 0x80, 0x80,
//             2, 0x80, 0x80,
//             4, 0x80, 0x80,
//             6, 0x80, 0x80,
//             8, 0x80, 0x80,
//             10, 0x80, 0x80,
//             4, 0x80, 0x80,
//             6, 0x80, 0x80,
//             8, 0x80, 0x80,
//             10, 0x80, 0x80,
//             12, 0x80, 0x80,
//             2, 0x80, 0x80,
//             4, 0x80, 0x80,
//             6, 0x80, 0x80,
//             8, 0x80, 0x80,
//             10, 0x80, 0x80,
//             0, 0x80, 0x80,
//             2, 0x80, 0x80,
//             4, 0x80, 0x80,
//             6, 0x80, 0x80,
//             8, 0x80, 0x80,
//             0x80};
//     pyyjson_align(64) static const u8 m1[64] = {
//             0xff, 0x3f, 0x3f,
//             0xff, 0x3f, 0x3f,
//             0xff, 0x3f, 0x3f,
//             0xff, 0x3f, 0x3f,
//             0xff, 0x3f, 0x3f,
//             0xff, 0x3f, 0x3f,
//             0xff, 0x3f, 0x3f,
//             0xff, 0x3f, 0x3f,
//             0xff, 0x3f, 0x3f,
//             0xff, 0x3f, 0x3f,
//             0xff, 0x3f, 0x3f,
//             0xff, 0x3f, 0x3f,
//             0xff, 0x3f, 0x3f,
//             0xff, 0x3f, 0x3f,
//             0xff, 0x3f, 0x3f,
//             0xff, 0x3f, 0x3f,
//             0xff, 0x3f, 0x3f,
//             0xff, 0x3f, 0x3f,
//             0xff, 0x3f, 0x3f,
//             0xff, 0x3f, 0x3f,
//             0xff, 0x3f, 0x3f,
//             0};
//     pyyjson_align(64) static const u8 m2[64] = {
//             0xe0, 0x80, 0x80,
//             0xe0, 0x80, 0x80,
//             0xe0, 0x80, 0x80,
//             0xe0, 0x80, 0x80,
//             0xe0, 0x80, 0x80,
//             0xe0, 0x80, 0x80,
//             0xe0, 0x80, 0x80,
//             0xe0, 0x80, 0x80,
//             0xe0, 0x80, 0x80,
//             0xe0, 0x80, 0x80,
//             0xe0, 0x80, 0x80,
//             0xe0, 0x80, 0x80,
//             0xe0, 0x80, 0x80,
//             0xe0, 0x80, 0x80,
//             0xe0, 0x80, 0x80,
//             0xe0, 0x80, 0x80,
//             0xe0, 0x80, 0x80,
//             0xe0, 0x80, 0x80,
//             0xe0, 0x80, 0x80,
//             0xe0, 0x80, 0x80,
//             0xe0, 0x80, 0x80,
//             0};
//     VECTOR_U16_512_A z1, z2, z3;
//     /*z1 = 00000000|00000000|abcdefgh */
//     z1 = _mm512_shuffle_epi8(z, *(const SIMD_512 *)t1);
//     /*z2 = gh123456|78000000 */
//     z2 = _mm512_srli_epi16(z, 6);
//     /*z2 = 00000000|gh123456|00000000 */
//     z2 = _mm512_shuffle_epi8(z2, *(const SIMD_512 *)t2);
//     /*z3 = 56780000|00000000 */
//     z3 = _mm512_srli_epi16(z, 12);
//     /*z3 = 56780000|00000000|00000000 */
//     z3 = _mm512_shuffle_epi8(z3, *(const SIMD_512 *)t3);
//     /*z = 56780000|gh123456|abcdefgh */
//     z = _mm512_or_si512(z1, _mm512_or_si512(z2, z3));
//     /*z = 56780000|gh123400|abcdef00 */
//     z = _mm512_and_si512(z, *(const SIMD_512 *)m1);
//     // 5678[mmmm]|gh1234[mm]|abcdef[mm]
//     z = _mm512_or_si512(z, *(const SIMD_512 *)m2);
//     _mm512_storeu_si512((void *)writer, z);
// }

force_inline void ucs2_encode_2bytes_utf8_avx512(VECTOR_U16_512_A z, u8 *writer) {
    /* abcdefgh|12300000 -> gh123[mmm]|abcdef[mm] */
    pyyjson_align(16) static const u8 t1[16] = {
            0x80, 0,
            0x80, 2,
            0x80, 4,
            0x80, 6,
            0x80, 8,
            0x80, 10,
            0x80, 12,
            0x80, 14};
    /*z1 = gh123000|00000000 */
    VECTOR_U16_512_A z1 = _mm512_srli_epi16(z, 6);
    /*z2 = 00000000|abcdefgh */
    VECTOR_U16_512_A z2 = _mm512_shuffle_epi8(z, _mm512_broadcast_i32x4(*(const SIMD_128 *)t1));
    /*z = gh123000|abcdefgh */
    z = z1 | z2;
    /*z = gh123000|abcdef00 */
    z = z & broadcast_16_512(0x3fff);
    /*z = gh123[mmm]|abcdef[mm] */
    z = z | broadcast_16_512(0x80c0);
    *(VECTOR_U16_512_U *)writer = z;
}

force_inline void ucs4_encode_3bytes_utf8_avx512(VECTOR_U32_512_A z, u8 *writer) {
    /* abcdefgh|12345678|00000000|00000000 -> 5678[mmmm]|gh1234[mm]|abcdef[mm] */
    VECTOR_U8_512_A t1 = {
            0x80, 0x80, 0x80, 0x80,
            0x80, 0x80, 0,
            0x80, 0x80, 4,
            0x80, 0x80, 8,
            0x80, 0x80, 12,
            //
            0x80, 0x80, 0,
            0x80, 0x80, 4,
            0x80, 0x80, 8,
            0x80, 0x80, 12,
            0x80, 0x80, 0x80, 0x80,
            //
            0x80, 0x80, 0x80, 0x80,
            0x80, 0x80, 0,
            0x80, 0x80, 4,
            0x80, 0x80, 8,
            0x80, 0x80, 12,
            //
            0x80, 0x80, 0,
            0x80, 0x80, 4,
            0x80, 0x80, 8,
            0x80, 0x80, 12,
            0x80, 0x80, 0x80, 0x80};
    VECTOR_U8_512_A t2 = {
            0x80, 0x80, 0x80, 0x80,
            0x80, 0, 0x80,
            0x80, 4, 0x80,
            0x80, 8, 0x80,
            0x80, 12, 0x80,
            //
            0x80, 0, 0x80,
            0x80, 4, 0x80,
            0x80, 8, 0x80,
            0x80, 12, 0x80,
            0x80, 0x80, 0x80, 0x80,
            //
            0x80, 0x80, 0x80, 0x80,
            0x80, 0, 0x80,
            0x80, 4, 0x80,
            0x80, 8, 0x80,
            0x80, 12, 0x80,
            //
            0x80, 0, 0x80,
            0x80, 4, 0x80,
            0x80, 8, 0x80,
            0x80, 12, 0x80,
            0x80, 0x80, 0x80, 0x80};
    VECTOR_U8_512_A t3 = {
            0x80, 0x80, 0x80, 0x80,
            0, 0x80, 0x80,
            4, 0x80, 0x80,
            8, 0x80, 0x80,
            12, 0x80, 0x80,
            //
            0, 0x80, 0x80,
            4, 0x80, 0x80,
            8, 0x80, 0x80,
            12, 0x80, 0x80,
            0x80, 0x80, 0x80, 0x80,
            //
            0x80, 0x80, 0x80, 0x80,
            0, 0x80, 0x80,
            4, 0x80, 0x80,
            8, 0x80, 0x80,
            12, 0x80, 0x80,
            //
            0, 0x80, 0x80,
            4, 0x80, 0x80,
            8, 0x80, 0x80,
            12, 0x80, 0x80,
            0x80, 0x80, 0x80, 0x80};
    VECTOR_U8_512_A m1 = {
            0xff, 0xff, 0xff, 0xff,
            0xff, 0x3f, 0x3f,
            0xff, 0x3f, 0x3f,
            0xff, 0x3f, 0x3f,
            0xff, 0x3f, 0x3f,
            //
            0xff, 0x3f, 0x3f,
            0xff, 0x3f, 0x3f,
            0xff, 0x3f, 0x3f,
            0xff, 0x3f, 0x3f,
            0xff, 0xff, 0xff, 0xff,
            //
            0xff, 0xff, 0xff, 0xff,
            0xff, 0x3f, 0x3f,
            0xff, 0x3f, 0x3f,
            0xff, 0x3f, 0x3f,
            0xff, 0x3f, 0x3f,
            //
            0xff, 0x3f, 0x3f,
            0xff, 0x3f, 0x3f,
            0xff, 0x3f, 0x3f,
            0xff, 0x3f, 0x3f,
            0xff, 0xff, 0xff, 0xff};
    VECTOR_U8_512_A m2 = {
            0, 0, 0, 0,
            0xe0, 0x80, 0x80,
            0xe0, 0x80, 0x80,
            0xe0, 0x80, 0x80,
            0xe0, 0x80, 0x80,
            //
            0xe0, 0x80, 0x80,
            0xe0, 0x80, 0x80,
            0xe0, 0x80, 0x80,
            0xe0, 0x80, 0x80,
            0, 0, 0, 0,
            //
            0, 0, 0, 0,
            0xe0, 0x80, 0x80,
            0xe0, 0x80, 0x80,
            0xe0, 0x80, 0x80,
            0xe0, 0x80, 0x80,
            //
            0xe0, 0x80, 0x80,
            0xe0, 0x80, 0x80,
            0xe0, 0x80, 0x80,
            0xe0, 0x80, 0x80,
            0, 0, 0, 0};
    /* z1 = gh123456|78000000|00000000|00000000 */
    VECTOR_U32_512_A z1 = _mm512_srli_epi32(z, 6);
    /* z2 = 56780000|00000000|00000000|00000000 */
    VECTOR_U32_512_A z2 = _mm512_srli_epi32(z, 12);
    /* z3 = 00000000|00000000|abcdefgh */
    VECTOR_U8_512_A z3 = _mm512_shuffle_epi8(z, t1);
    /* z4 = 00000000|gh123456|00000000 */
    VECTOR_U8_512_A z4 = _mm512_shuffle_epi8(z1, t2);
    /* z5 = 56780000|00000000|00000000 */
    VECTOR_U8_512_A z5 = _mm512_shuffle_epi8(z2, t3);
    VECTOR_U8_512_A z6 = ((z3 | z4 | z5) & m1) | m2;
    _mm512_mask_storeu_epi8(writer - 4, 0xffffff0, z6);
    _mm512_mask_storeu_epi8(writer - 12, 0xffffff000000000, z6);
}

#    endif

/*==============================================================================
 * SIMD half related.
 * This does not have 128-bits version.
 *============================================================================*/
#    if SIMD_BIT_SIZE > 128
force_inline SIMD_HALF_TYPE load_aligned_half(const void *src) {
#        if SIMD_BIT_SIZE == 256
    return load_128_aligned(src);
#        else // SIMD_BIT_SIZE == 512
    return load_256_aligned(src);
#        endif
}

force_inline SIMD_HALF_TYPE load_half(const void *src) {
#        if SIMD_BIT_SIZE == 256
    return load_128(src);
#        else
    return load_256(src);
#        endif
}

#    endif // SIMD_BIT_SIZE > 128

/*==============================================================================
 * Zip unsigned integer related.
 * Zip array of u32/u16 to u16/u8.
 *============================================================================*/

force_inline SIMD_REAL_HALF_TYPE zip_simd_32_to_16(SIMD_TYPE SIMD_VAR) {
#    if SIMD_BIT_SIZE == 512
    /* z = A|B|C|D */
    SIMD_128 x1, x2, x3, x4;
    extract_512_four_parts(z, &x1, &x2, &x3, &x4);
    /* y1 = A|C */
    SIMD_256 y1 = _mm256_set_m128i(x3, x1);
    /* y2 = B|D */
    SIMD_256 y2 = _mm256_set_m128i(x4, x2);
    return _mm256_packus_epi32(y1, y2);
#    elif SIMD_BIT_SIZE == 256
    return zip_256_32_to_16(y);
#    elif __SSE4_1__
    return (SIMD_REAL_HALF_TYPE)real_extract_first_64_from_128(_mm_packus_epi32(x, x));
#    else
    // in this case we don't have the convenient `_mm_packus_epi32`
    // TODO: is this really faster than *dst++ = *src++ ???
    /* x =  aa00bb00|cc00dd00 */
    /* x1 = 00cc00dd|00000000 */
    SIMD_128 x1 = _mm_srli_si128(x, 6);
    /* x2 = bb00cc00|dd000000 */
    SIMD_128 x2 = _mm_srli_si128(x, 4);
    /* x3 = 00dd0000|00000000 */
    SIMD_128 x3 = _mm_srli_si128(x, 10);
    /* x4 = aaccbbdd|cc00dd00 */
    SIMD_128 x4 = simd_or_128(x, x1);
    /* x5 = bbddcc00|dd000000 */
    SIMD_128 x5 = simd_or_128(x2, x3);
    return (SIMD_REAL_HALF_TYPE)real_extract_first_64_from_128(_mm_unpacklo_epi16(x4, x5));
#    endif
}

force_inline SIMD_REAL_HALF_TYPE zip_simd_16_to_8(SIMD_TYPE SIMD_VAR) {
#    if SIMD_BIT_SIZE == 512
    /* z = A|B|C|D */
    SIMD_128 x1, x2, x3, x4;
    extract_512_four_parts(z, &x1, &x2, &x3, &x4);
    /* y1 = A|C */
    SIMD_256 y1 = _mm256_set_m128i(x3, x1);
    /* y2 = B|D */
    SIMD_256 y2 = _mm256_set_m128i(x4, x2);
    return _mm256_packus_epi16(y1, y2);
#    elif SIMD_BIT_SIZE == 256
    return zip_256_16_to_8(y);
#    else
    /* x = aaxxbbxxccxxddxx */
    return (SIMD_REAL_HALF_TYPE)real_extract_first_64_from_128(_mm_packus_epi16(x, x));
#    endif
}

force_inline SIMD_REAL_QUARTER_TYPE zip_simd_32_to_8(SIMD_TYPE SIMD_VAR) {
#    if SIMD_BIT_SIZE == 512
    pyyjson_align(64) static const u8 t1[64] = {
            0, 4, 8, 12,
            0x80, 0x80,
            0x80, 0x80,
            0x80, 0x80,
            0x80, 0x80,
            0x80, 0x80,
            0x80, 0x80,
            // seperate
            0x80, 0x80,
            0x80, 0x80,
            0, 4, 8, 12,
            0x80, 0x80,
            0x80, 0x80,
            0x80, 0x80,
            0x80, 0x80,
            // seperate
            0x80, 0x80,
            0x80, 0x80,
            0x80, 0x80,
            0x80, 0x80,
            0, 4, 8, 12,
            0x80, 0x80,
            0x80, 0x80,
            // seperate
            0x80, 0x80,
            0x80, 0x80,
            0x80, 0x80,
            0x80, 0x80,
            0x80, 0x80,
            0x80, 0x80,
            0, 4, 8, 12};
    SIMD_512 z1 = _mm512_shuffle_epi8(z, load_512_aligned(t1));
    SIMD_128 x1, x2, x3, x4;
    extract_512_four_parts(z1, &x1, &x2, &x3, &x4);
    return simd_or_128(simd_or_128(x1, x2), simd_or_128(x3, x4));
#    elif SIMD_BIT_SIZE == 256
    return zip_256_32_to_8(y);
#    elif __SSSE3__
    pyyjson_align(64) static const u8 t1[16] = {0, 4, 8, 12,
                                                0x80, 0x80,
                                                0x80, 0x80,
                                                0x80, 0x80,
                                                0x80, 0x80,
                                                0x80, 0x80,
                                                0x80, 0x80};
    return (SIMD_REAL_QUARTER_TYPE)real_extract_first_32_from_128(_mm_shuffle_epi8(x, load_128_aligned(t1)));
#    else
    // first using signed pack to u16. The values in `x` are below 256, so signed pack is equivalent to unsigned pack.
    SIMD_128 x1 = _mm_packs_epi32(x, x);
    // then use unsigned pack to u8
    return real_extract_first_32_from_128(_mm_packus_epi16(x1, x1));
#    endif
}

/*==============================================================================
 * Half/quarter write.
 *============================================================================*/
force_inline void write_real_quarter(void *dst, SIMD_REAL_QUARTER_TYPE quat) {
#    if SIMD_BIT_SIZE == 512
    write_128(dst, quat);
#    elif SIMD_BIT_SIZE == 256
    *(u64 *)dst = quat;
#    else
    *(u32 *)dst = quat;
#    endif
}

force_inline void write_real_half(void *dst, SIMD_REAL_HALF_TYPE half) {
#    if SIMD_BIT_SIZE == 512
    write_256(dst, half);
#    elif SIMD_BIT_SIZE == 256
    write_128(dst, half);
#    else
    *(u64 *)dst = half;
#    endif
}
#elif PYYJSON_AARCH

force_inline void write_u8_128(void *dst, VECTOR_U8_128_A x) {
    memcpy(dst, &x, sizeof(x));
}

force_inline void write_u16_128(void *dst, VECTOR_U16_128_A x) {
    memcpy(dst, &x, sizeof(x));
}

force_inline void write_u32_128(void *dst, VECTOR_U32_128_A x) {
    memcpy(dst, &x, sizeof(x));
}

force_inline VECTOR_U16_128_A elevate_1_2_to_128(VECTOR_U8_64_A _in) {
    return vmovl_u8(_in);
}

force_inline VECTOR_U32_128_A elevate_2_4_to_128(VECTOR_U16_64_A _in) {
    return vmovl_u16(_in);
}

force_inline VECTOR_U32_128_A elevate_1_4_to_128(VECTOR_U8_32_A _in) {
    VECTOR_U32_128_A _out;
    for (int i = 0; i < 4; ++i) {
        _out[i] = _in[i];
    }
    return _out;
}

#endif
#endif // PYYJSON_SIMD_IMPL_H
