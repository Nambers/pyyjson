#ifndef ENCODE_SIMD_IMPL_H
#define ENCODE_SIMD_IMPL_H

#include "simd_detect.h"
#include <immintrin.h>

#if SIMD_BIT_SIZE == 512
#define SIMD_VAR z
#define SIMD_TYPE __m512i
#define SIMD_MASK_TYPE u64
#define SIMD_SMALL_MASK_TYPE u16
#define SIMD_BIT_MASK_TYPE u64
#define SIMD_HALF_TYPE __m256i
#define SIMD_EXTRACT_PART _mm512_extracti32x4_epi32
#define SIMD_MASK_EXTRACT_PART(m, i) (u16)((m >> (i * 16)) & 0xFFFF)
#define SIMD_EXTRACT_HALF _mm512_extracti32x8_epi32
#elif SIMD_BIT_SIZE == 256
#define SIMD_VAR y
#define SIMD_TYPE __m256i
#define SIMD_MASK_TYPE SIMD_TYPE
#define SIMD_SMALL_MASK_TYPE __m128i
#define SIMD_BIT_MASK_TYPE u32
#define SIMD_HALF_TYPE __m128i
#define SIMD_EXTRACT_PART _mm256_extracti128_si256
#define SIMD_MASK_EXTRACT_PART SIMD_EXTRACT_PART
#define SIMD_EXTRACT_HALF SIMD_EXTRACT_PART
#else
#define SIMD_VAR x
#define SIMD_TYPE __m128i
#define SIMD_MASK_TYPE SIMD_TYPE
#define SIMD_SMALL_MASK_TYPE SIMD_TYPE
#define SIMD_BIT_MASK_TYPE u16
#endif

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
 * Write memory with length sizeof(SIMD_TYPE) to `dst`.
 * This is unaligned.
 */
force_inline void write_simd(void *dst, SIMD_TYPE SIMD_VAR) {
#if SIMD_BIT_SIZE == 512
    _mm512_storeu_si512(dst, SIMD_VAR);
#elif SIMD_BIT_SIZE == 256
    _mm256_storeu_si256((__m256i_u *) dst, SIMD_VAR);
#else
    _mm_storeu_si128((__m128i_u *) dst, SIMD_VAR);
#endif
}

/*
 * Right shift 128 bits for the case imm8 cannot be determined at compile time.
 Shifted bits should be multiple of 8; imm8 is the number of "bytes" to shift.
 */
force_inline SIMD_128 runtime_right_shift_128bits(SIMD_128 x, int imm8) {
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
}

/*
 * write memory with length sizeof(SIMD_TYPE) to `dst`.
 * The `dst` must be aligned.
 */
force_inline void write_aligned(void *dst, SIMD_TYPE SIMD_VAR) {
#if SIMD_BIT_SIZE == 128
    _mm_store_si128((__m128i *) dst, SIMD_VAR);
#elif SIMD_BIT_SIZE == 256
    _mm256_store_si256((__m256i *) dst, SIMD_VAR);
#else
    _mm512_store_si512(dst, SIMD_VAR);
#endif
}

/*
 * write memory with length sizeof(SIMD_TYPE) to dst.
 */
force_inline void write_128(void *dst, SIMD_128 x) {
    _mm_storeu_si128((__m128i_u *) dst, x);
}

force_inline SIMD_128 load_128(const void *src) {
#if __SSE3__
    return _mm_lddqu_si128((const __m128i_u *) src);
#else
    return _mm_loadu_si128((const __m128i_u *) src);
#endif
}

force_inline SIMD_128 load_128_aligned(const void *src) {
    return _mm_load_si128((const __m128i *) src);
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

force_inline SIMD_128 broadcast_32_128(i32 v) {
    return _mm_set1_epi32(v);
}

force_inline SIMD_128 broadcast_64_128(i64 v) {
    return _mm_set1_epi64((__m64) v);
}

force_inline SIMD_128 set_32_128(i32 a, i32 b, i32 c, i32 d) {
    return _mm_set_epi32(a, b, c, d);
}

force_inline SIMD_128 unpack_hi_64_128(SIMD_128 a, SIMD_128 b) {
    return _mm_unpackhi_epi64(a, b);
}

force_inline SIMD_128 cmpeq0_8_128(SIMD_128 a) {
    return _mm_cmpeq_epi8(a, _mm_setzero_si128());
}

force_inline u16 to_bitmask_128(SIMD_128 a) {
    return (u16) _mm_movemask_epi8(a);
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
    return _mm256_lddqu_si256((const __m256i_u *) src);
}

force_inline SIMD_256 load_256_aligned(const void *src) {
    return _mm256_load_si256((const __m256i *) src);
}

force_inline void write_256(void *dst, SIMD_256 y) {
    _mm256_storeu_si256((__m256i_u *) dst, y);
}

force_inline void write_256_aligned(void *dst, SIMD_256 y) {
    _mm256_store_si256((__m256i *) dst, y);
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

force_inline u32 to_bitmask_256(SIMD_256 a) {
    int t = _mm256_movemask_epi8(a);
    return (u32) t;
}

force_inline SIMD_256 cmpeq0_8_256(SIMD_256 a) {
    return _mm256_cmpeq_epi8(a, _mm256_setzero_si256());
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
#endif

/*==============================================================================
 * blendv
 * This does not have AVX512 version.
 *============================================================================*/
#if SIMD_BIT_SIZE != 512
force_inline SIMD_TYPE blendv(SIMD_TYPE blend, SIMD_TYPE SIMD_VAR, SIMD_MASK_TYPE mask) {
#if SIMD_BIT_SIZE == 256
    return _mm256_blendv_epi8(blend, SIMD_VAR, mask);
#elif SIMD_BIT_SIZE == 128
    return _mm_blendv_epi8(blend, SIMD_VAR, mask);
#endif
}
#endif

/*==============================================================================
 * SIMD half related.
 * This does not have 128-bits version.
 *============================================================================*/
#if defined(SIMD_HALF_TYPE)
force_inline SIMD_HALF_TYPE load_aligned_half(const void *src) {
#if SIMD_BIT_SIZE == 256
    return load_128_aligned(src);
#else // SIMD_BIT_SIZE == 512
    return load_256_aligned(src);
#endif
}

force_inline SIMD_HALF_TYPE load_half(const void *src) {
#if SIMD_BIT_SIZE == 256
    return load_128(src);
#else
    return load_256(src);
#endif
}

#endif // defined(SIMD_HALF_TYPE)

/*==============================================================================
 * TZCNT.
 *============================================================================*/
force_noinline u32 tzcnt_u32(u32 x) {
    // never pass a zero here.
    assert(x);
    return _mm_tzcnt_32(x);
}

force_noinline u64 tzcnt_u64(u64 x) {
    // never pass a zero here.
    assert(x);
    return _mm_tzcnt_64(x);
}

#endif // ENCODE_SIMD_IMPL_H
