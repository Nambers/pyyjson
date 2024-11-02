#ifndef ENCODE_SIMD_IMPL_H
#define ENCODE_SIMD_IMPL_H

#include "simd_detect.h"
#include <immintrin.h>

#if SIMD_BIT_SIZE == 512
#define SIMD_VAR z
#define SIMD_TYPE __m512i
#define SIMD_MASK_TYPE u64
#define SIMD_SMALL_MASK_TYPE u16
#define SIMD_HALF_TYPE __m256i
#elif SIMD_BIT_SIZE == 256
#define SIMD_VAR y
#define SIMD_TYPE __m256i
#define SIMD_MASK_TYPE SIMD_TYPE
#define SIMD_SMALL_MASK_TYPE __m128i
#define SIMD_HALF_TYPE __m128i
#else
#define SIMD_VAR x
#define SIMD_TYPE __m128i
#define SIMD_MASK_TYPE SIMD_TYPE
#define SIMD_SMALL_MASK_TYPE SIMD_TYPE
#endif

/*==============================================================================
 * common SIMD code
 *============================================================================*/
#define RIGHT_SHIFT_128BITS(x, bits, out_ptr)                  \
    do {                                                       \
        static_assert(((bits) % 8) == 0, "((bits) % 8) == 0"); \
        *(out_ptr) = _mm_bsrli_si128((x), (bits) / 8);         \
    } while (0)

force_inline void write_simd(void *dst, SIMD_TYPE SIMD_VAR) {
#if SIMD_BIT_SIZE == 512
    _mm512_storeu_si512(dst, SIMD_VAR);
#elif SIMD_BIT_SIZE == 256
    _mm256_storeu_si256((__m256i_u *) dst, SIMD_VAR);
#else
    _mm_storeu_si128((__m128i_u *) dst, SIMD_VAR);
#endif
}

force_inline void write_simd_128_impl(void *dst, SIMD_128 x) {
    _mm_storeu_si128((__m128i_u *) dst, x);
}

force_inline SIMD_128 load_128(const void *src) {
#if __SSE3__
    return _mm_lddqu_si128((const __m128i_u *) src);
#else
    return _mm_loadu_si128((const __m128i_u *) src);
#endif
}

force_inline void write_aligned(void *dst, SIMD_TYPE SIMD_VAR) {
#if SIMD_BIT_SIZE == 128
    _mm_store_si128((__m128i *) dst, SIMD_VAR);
#elif SIMD_BIT_SIZE == 256
    _mm256_store_si256((__m256i *) dst, SIMD_VAR);
#else
    _mm512_store_si512(dst, SIMD_VAR);
#endif
}

/*==============================================================================
 * AVX2 only SIMD code
 *============================================================================*/
#if __AVX2__
force_inline SIMD_256 elevate_2_4_to_256(SIMD_128 x) {
    return _mm256_cvtepu16_epi32(x);
}

force_inline SIMD_256 elevate_1_2_to_256(SIMD_128 x) {
    return _mm256_cvtepu8_epi16(x);
}

force_inline SIMD_256 elevate_1_4_to_256(SIMD_64 x) {
    return _mm256_cvtepu8_epi32(x);
}

force_inline SIMD_256 simd_and_256(SIMD_256 a, SIMD_256 b) {
    return _mm256_and_si256(a, b);
}
#endif

/*==============================================================================
 * AVX only SIMD code
 *============================================================================*/
#if __AVX__
force_inline SIMD_256 load_256_aligned(const void *src) {
    return _mm256_load_si256((const __m256i *) src);
}

force_inline SIMD_256 load_256(const void *src) {
    return _mm256_lddqu_si256((const __m256i_u *) src);
}

force_inline void write_256(void *dst, SIMD_256 y) {
    _mm256_storeu_si256((__m256i_u *) dst, y);
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
    return _mm_load_si128((const __m128i *) src);
#else
    return _mm256_load_si256((const __m256i *) src);
#endif
}

force_inline SIMD_HALF_TYPE load_half(const void *src) {
#if SIMD_BIT_SIZE == 256
    return _mm_loadu_si128((const __m128i *) src);
#else
    return _mm256_loadu_si256((const __m256i *) src);
#endif
}

#endif // defined(SIMD_HALF_TYPE)


// #undef SIMD_VAR
// #undef SIMD_TYPE
// #undef SIMD_MASK_TYPE
// #undef SIMD_SMALL_MASK_TYPE

#endif // ENCODE_SIMD_IMPL_H
