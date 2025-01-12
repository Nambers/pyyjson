#ifndef ENCODE_SIMD_IMPL_H
#define ENCODE_SIMD_IMPL_H

#include "pyyjson.h"
#include "simd/simd_detect.h"
#include <immintrin.h>
#if defined(_MSC_VER)
#    include <intrin.h>
#endif


#if SIMD_BIT_SIZE == 512
#    define SIMD_VAR z
#    define SIMD_TYPE __m512i
#    define SIMD_MASK_TYPE u64
#    define SIMD_SMALL_MASK_TYPE u16
#    define SIMD_BIT_MASK_TYPE u64
#    define SIMD_HALF_TYPE __m256i
#    define SIMD_EXTRACT_QUARTER _mm512_extracti32x4_epi32
#    define SIMD_EXTRACT_HALF _mm512_extracti64x4_epi64
#    define SIMD_REAL_HALF_TYPE __m256i
#    define SIMD_REAL_QUARTER_TYPE __m128i
#elif SIMD_BIT_SIZE == 256
#    define SIMD_VAR y
#    define SIMD_TYPE __m256i
#    define SIMD_MASK_TYPE SIMD_TYPE
#    define SIMD_SMALL_MASK_TYPE __m128i
#    define SIMD_BIT_MASK_TYPE u32
#    define SIMD_HALF_TYPE __m128i
#    define SIMD_EXTRACT_HALF _mm256_extracti128_si256
#    define SIMD_REAL_HALF_TYPE __m128i
#    define SIMD_REAL_QUARTER_TYPE u64
#else
#    define SIMD_VAR x
#    define SIMD_TYPE __m128i
#    define SIMD_MASK_TYPE SIMD_TYPE
#    define SIMD_SMALL_MASK_TYPE SIMD_TYPE
#    define SIMD_BIT_MASK_TYPE u16
#    define SIMD_HALF_TYPE SIMD_TYPE
#    define SIMD_REAL_HALF_TYPE u64
#    define SIMD_REAL_QUARTER_TYPE u32
#endif

#define SIMD_STORER PYYJSON_CONCAT2(write, SIMD_BIT_SIZE)
#define SIMD_AND PYYJSON_CONCAT2(simd_and, SIMD_BIT_SIZE)

/*==============================================================================
 * common SIMD code
 *============================================================================*/

/*
 * Right shift 128 bits. Shifted bits should be multiple of 8.
 */
#define RIGHT_SHIFT_128BITS(x, bits, out_ptr)                  \
    do {                                                       \
        static_assert(((bits) % 8) == 0, "((bits) % 8) == 0"); \
        *(out_ptr) = _mm_bsrli_si128((x), (bits) / 8);         \
    } while (0)

/*
 * Load memory to a simd variable.
 * This is unaligned.
 */
force_inline SIMD_TYPE load_simd(const void *src) {
#if SIMD_BIT_SIZE == 512
    return _mm512_loadu_si512(src);
#elif SIMD_BIT_SIZE == 256
    return _mm256_lddqu_si256((const SIMD_256_IU *)src);
#elif __SSE3__
    return _mm_lddqu_si128((const SIMD_128_IU *)src);
#else
    return _mm_loadu_si128((const SIMD_128_IU *)src);
#endif
}

/*
 * Write memory with length sizeof(SIMD_TYPE) to `dst`.
 * This is unaligned.
 */
force_inline void write_simd(void *dst, SIMD_TYPE SIMD_VAR) {
#if SIMD_BIT_SIZE == 512
    _mm512_storeu_si512(dst, SIMD_VAR);
#elif SIMD_BIT_SIZE == 256
    _mm256_storeu_si256((SIMD_256_IU *)dst, SIMD_VAR);
#else
    _mm_storeu_si128((SIMD_128_IU *)dst, SIMD_VAR);
#endif
}

/*
 * write memory with length sizeof(SIMD_TYPE) to `dst`.
 * The `dst` must be aligned.
 */
force_inline void write_aligned(void *dst, SIMD_TYPE SIMD_VAR) {
#if SIMD_BIT_SIZE == 128
    _mm_store_si128((__m128i *)dst, SIMD_VAR);
#elif SIMD_BIT_SIZE == 256
    _mm256_store_si256((__m256i *)dst, SIMD_VAR);
#else
    _mm512_store_si512(dst, SIMD_VAR);
#endif
}

/*
 * write memory with length sizeof(SIMD_TYPE) to dst.
 */
force_inline void write_128(void *dst, SIMD_128 x) {
    _mm_storeu_si128((SIMD_128_IU *)dst, x);
}

force_inline SIMD_128 load_128(const void *src) {
#if __SSE3__
    return _mm_lddqu_si128((const SIMD_128_IU *)src);
#else
    return _mm_loadu_si128((const SIMD_128_IU *)src);
#endif
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
#if defined(_MSC_VER) && !defined(_M_IX86)
    return _mm_set1_epi64x(v);
#else
    return _mm_set1_epi64((__m64)v);
#endif
}

#if __SSSE3__
force_inline const void *read_rshift_mask_table(int row);
#endif
/*
 * Right shift 128 bits for the case imm8 cannot be determined at compile time.
 Shifted bits should be multiple of 8; imm8 is the number of "bytes" to shift.
 */
force_inline SIMD_128 runtime_right_shift_128bits(SIMD_128 x, int imm8) {
#if __SSSE3__
    return _mm_shuffle_epi8(x, load_128_aligned(read_rshift_mask_table(imm8)));
#else
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
#endif
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

force_inline SIMD_128 satureate_minus_128(SIMD_128 a, SIMD_128 b) {
    return _mm_subs_epu8(a, b);
}

/* Elevate utilities.
 * See https://github.com/samyvilar/dyn_perf/blob/master/sse2.h
 */
force_inline SIMD_128 elevate_1_2_to_128(SIMD_128 a) {
#if __SSE4_1__
    return _mm_cvtepu8_epi16(a);
#elif __SSSE3__
    return _mm_shuffle_epi8(a, _mm_set_epi8(0x80, 7, 0x80, 6, 0x80, 5, 0x80, 4, 0x80, 3, 0x80, 2, 0x80, 1, 0x80, 0));
#else
    return _mm_unpacklo_epi8(a, _mm_setzero_si128()); // ~2 cycles ..
#endif
}

force_inline SIMD_128 elevate_1_4_to_128(SIMD_128 a) {
#if __SSE4_1__
    return _mm_cvtepu8_epi32(a);
#elif __SSSE3__
    return _mm_shuffle_epi8(
            a,
            _mm_set_epi8(
                    0x80, 0x80, 0x80, 3,
                    0x80, 0x80, 0x80, 2,
                    0x80, 0x80, 0x80, 1,
                    0x80, 0x80, 0x80, 0));
#else
    a = _mm_unpacklo_epi8(a, a);                         // a0, a0, a1, a1, a2, a2, a3, a3, ....
    return _mm_srli_epi32(_mm_unpacklo_epi16(a, a), 24); // ~ 3 cycles ...
#endif
}

force_inline SIMD_128 elevate_2_4_to_128(SIMD_128 a) {

#if defined(__SSE4_1__)
    return _mm_cvtepu16_epi32(a);
#elif defined(__SSSE3__)
    return _mm_shuffle_epi8(
            a,
            _mm_set_epi8(
                    0x80, 0x80, 7, 6,
                    0x80, 0x80, 5, 4,
                    0x80, 0x80, 3, 2,
                    0x80, 0x80, 1, 0));
#else
    return _mm_unpacklo_epi16(a, _mm_setzero_si128());
#endif
}

force_inline void extract_128_two_parts(SIMD_128 x, SIMD_128 *restrict x1, SIMD_128 *restrict x2) {
    *x1 = x;
    *x2 = unpack_hi_64_128(x, x);
}

force_inline void extract_128_four_parts(SIMD_128 x, SIMD_128 *restrict x1, SIMD_128 *restrict x2, SIMD_128 *restrict x3, SIMD_128 *restrict x4) {
#if __SSE3__
#    define MOVEHDUP(_x) (SIMD_128) _mm_movehdup_ps((__m128)(_x))
#else
#    define MOVEHDUP(_x) _mm_bsrli_si128((_x), 4)
#endif
    *x1 = x;
    *x2 = MOVEHDUP(x);
    *x3 = unpack_hi_64_128(x, x);
    *x4 = MOVEHDUP(*x3);
#undef MOVEHDUP
}

force_inline u64 real_extract_first_64_from_128(SIMD_128 x) {
#if defined(_MSC_VER) && !defined(_M_IX86)
    return (u64)_mm_cvtsi128_si64x(x);
#else
    return (u64)_mm_cvtsi128_si64(x);
#endif
}

force_inline u32 real_extract_first_32_from_128(SIMD_128 x) {
    return (u32)_mm_cvtsi128_si32(x);
}

/* (a & b) == 0 */
force_inline bool testz_128(SIMD_128 a, SIMD_128 b) {
#if defined(__SSE4_1__)
    return (bool)_mm_testz_si128(a, b);
#else
    return _mm_movemask_epi8(_mm_cmpeq_epi8(simd_and_128(a, b), _mm_setzero_si128())) == 0xFFFF;
#endif
}

/*
 * Mask utilities.
 */
force_inline bool check_mask_zero(SIMD_MASK_TYPE mask) {
#if SIMD_BIT_SIZE == 512
    return mask == 0;
#elif SIMD_BIT_SIZE == 256
    return (bool)_mm256_testz_si256(mask, mask);
#else
#    if defined(__SSE4_1__)
    return (bool)_mm_testz_si128(mask, mask);
#    else
    return _mm_movemask_epi8(_mm_cmpeq_epi8(mask, _mm_setzero_si128())) == 0xFFFF;
#    endif
#endif
}

/*==============================================================================
 * SSE4.1 only SIMD code
 *============================================================================*/
#if __SSE4_1__
force_inline SIMD_128 blendv_128(SIMD_128 blend, SIMD_128 x, SIMD_128 mask) {
    return _mm_blendv_epi8(blend, x, mask);
}
#endif

/*==============================================================================
 * SSE4.2 only SIMD code
 *============================================================================*/
#if __SSE4_2__
#endif

/*==============================================================================
 * AVX only SIMD code
 *============================================================================*/
#if __AVX__
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

force_inline SIMD_256 broadcast_8_256(i8 v) {
    return _mm256_set1_epi8(v);
}

force_inline SIMD_256 broadcast_16_256(i16 v) {
    return _mm256_set1_epi16(v);
}

force_inline SIMD_256 broadcast_32_256(i32 v) {
    return _mm256_set1_epi32(v);
}
#endif

/*==============================================================================
 * AVX2 only SIMD code
 *============================================================================*/
#if __AVX2__
force_inline SIMD_256 elevate_1_2_to_256(SIMD_128 x) {
    return _mm256_cvtepu8_epi16(x);
}

force_inline SIMD_256 elevate_1_4_to_256(SIMD_64 x) {
    return _mm256_cvtepu8_epi32(x);
}

force_inline SIMD_256 elevate_2_4_to_256(SIMD_128 x) {
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

force_inline SIMD_256 cmpeq0_8_256(SIMD_256 a) {
    return _mm256_cmpeq_epi8(a, _mm256_setzero_si256());
}

force_inline SIMD_256 blendv_256(SIMD_256 blend, SIMD_256 SIMD_VAR, SIMD_256 mask) {
    return _mm256_blendv_epi8(blend, SIMD_VAR, mask);
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

force_inline SIMD_256 cmpgt_i32_256(SIMD_256 a, SIMD_256 b) {
    return _mm256_cmpgt_epi32(a, b);
}
#endif

/*==============================================================================
 * AVX512F only SIMD code
 *============================================================================*/
#if __AVX512F__
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

force_inline SIMD_512 elevate_2_4_to_512(SIMD_256 y) {
    return _mm512_cvtepu16_epi32(y);
}

force_inline SIMD_512 elevate_1_4_to_512(SIMD_128 x) {
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
#endif

/*==============================================================================
 * AVX512BW only SIMD code
 *============================================================================*/
#if __AVX512BW__
force_inline SIMD_512 elevate_1_2_to_512(SIMD_256 y) {
    return _mm512_cvtepu8_epi16(y);
}
#endif

/*==============================================================================
 * SIMD half related.
 * This does not have 128-bits version.
 *============================================================================*/
#if SIMD_BIT_SIZE > 128
force_inline SIMD_HALF_TYPE load_aligned_half(const void *src) {
#    if SIMD_BIT_SIZE == 256
    return load_128_aligned(src);
#    else // SIMD_BIT_SIZE == 512
    return load_256_aligned(src);
#    endif
}

force_inline SIMD_HALF_TYPE load_half(const void *src) {
#    if SIMD_BIT_SIZE == 256
    return load_128(src);
#    else
    return load_256(src);
#    endif
}

#endif // SIMD_BIT_SIZE > 128

/*==============================================================================
 * Zip unsigned integer related.
 * Zip array of u32/u16 to u16/u8.
 *============================================================================*/

force_inline SIMD_REAL_HALF_TYPE zip_simd_32_to_16(SIMD_TYPE SIMD_VAR) {
#if SIMD_BIT_SIZE == 512
    /* z = A|B|C|D */
    SIMD_128 x1, x2, x3, x4;
    extract_512_four_parts(z, &x1, &x2, &x3, &x4);
    /* y1 = A|C */
    SIMD_256 y1 = _mm256_set_m128i(x3, x1);
    /* y2 = B|D */
    SIMD_256 y2 = _mm256_set_m128i(x4, x2);
    return _mm256_packus_epi32(y1, y2);
#elif SIMD_BIT_SIZE == 256
    __m128i x_low = _mm256_extracti128_si256(y, 0);
    __m128i x_high = _mm256_extracti128_si256(y, 1);
    return _mm_packus_epi32(x_low, x_high);
#elif __SSE4_1__
    return (SIMD_REAL_HALF_TYPE)real_extract_first_64_from_128(_mm_packus_epi32(x, x));
#else
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
#endif
}

force_inline SIMD_REAL_HALF_TYPE zip_simd_16_to_8(SIMD_TYPE SIMD_VAR) {
#if SIMD_BIT_SIZE == 512
    /* z = A|B|C|D */
    SIMD_128 x1, x2, x3, x4;
    extract_512_four_parts(z, &x1, &x2, &x3, &x4);
    /* y1 = A|C */
    SIMD_256 y1 = _mm256_set_m128i(x3, x1);
    /* y2 = B|D */
    SIMD_256 y2 = _mm256_set_m128i(x4, x2);
    return _mm256_packus_epi16(y1, y2);
#elif SIMD_BIT_SIZE == 256
    __m128i x_low = _mm256_extracti128_si256(y, 0);
    __m128i x_high = _mm256_extracti128_si256(y, 1);
    return _mm_packus_epi16(x_low, x_high);
#else
    /* x = aaxxbbxxccxxddxx */
    return (SIMD_REAL_HALF_TYPE)real_extract_first_64_from_128(_mm_packus_epi16(x, x));
#endif
}

force_inline SIMD_REAL_QUARTER_TYPE zip_simd_32_to_8(SIMD_TYPE SIMD_VAR) {
#if SIMD_BIT_SIZE == 512
    static const u8 t1[64] = {
            0, 4, 8, 12,
            16, 20, 24, 28,
            32, 36, 40, 44,
            48, 52, 56, 60,
            // seperate
            0x80, 0x80,
            0x80, 0x80,
            0x80, 0x80,
            0x80, 0x80,
            0x80, 0x80,
            0x80, 0x80,
            0x80, 0x80,
            0x80, 0x80,
            // seperate
            0x80, 0x80,
            0x80, 0x80,
            0x80, 0x80,
            0x80, 0x80,
            0x80, 0x80,
            0x80, 0x80,
            0x80, 0x80,
            0x80, 0x80,
            // seperate
            0x80, 0x80,
            0x80, 0x80,
            0x80, 0x80,
            0x80, 0x80,
            0x80, 0x80,
            0x80, 0x80,
            0x80, 0x80,
            0x80, 0x80};
    SIMD_512 z1 = _mm512_shuffle_epi8(z, load_512_aligned(t1));
    return SIMD_EXTRACT_QUARTER(z1, 0);
#elif SIMD_BIT_SIZE == 256
    /*y = axxxbxxxcxxxdxxx|exxxfxxxgxxxhxxx */
    static const u8 t1[32] = {0, 4, 8, 12,
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
    return (SIMD_REAL_QUARTER_TYPE)(i1 | i2);
#elif __SSSE3__
    static const u8 t1[16] = {0, 4, 8, 12,
                              0x80, 0x80,
                              0x80, 0x80,
                              0x80, 0x80,
                              0x80, 0x80,
                              0x80, 0x80,
                              0x80, 0x80};
    return (SIMD_REAL_QUARTER_TYPE)real_extract_first_32_from_128(_mm_shuffle_epi8(x, load_128_aligned(t1)));
#else
    // first using signed pack to u16. The values in `x` are below 256, so signed pack is equivalent to unsigned pack.
    SIMD_128 x1 = _mm_packs_epi32(x, x);
    // then use unsigned pack to u8
    return real_extract_first_32_from_128(_mm_packus_epi16(x1, x1));
#endif
}

#endif // ENCODE_SIMD_IMPL_H
