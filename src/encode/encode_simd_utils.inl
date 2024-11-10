#include "simd_detect.h"
#include <immintrin.h>

#ifndef COMPILE_READ_UCS_LEVEL
#error "COMPILE_READ_UCS_LEVEL is not defined"
#endif

#ifndef COMPILE_WRITE_UCS_LEVEL
#error "COMPILE_WRITE_UCS_LEVEL is not defined"
#endif


#if COMPILE_WRITE_UCS_LEVEL == 4
#define _WRITER U32_WRITER
#define _TARGET_TYPE u32
#elif COMPILE_WRITE_UCS_LEVEL == 2
#define _WRITER U16_WRITER
#define _TARGET_TYPE u16
#elif COMPILE_WRITE_UCS_LEVEL == 1
#define _WRITER U8_WRITER
#define _TARGET_TYPE u8
#else
#error "COMPILE_WRITE_UCS_LEVEL must be 1, 2 or 4"
#endif

#if COMPILE_READ_UCS_LEVEL == 4
#define CHECK_MASK_TYPE u16
#define _FROM_TYPE u32
#elif COMPILE_READ_UCS_LEVEL == 2
#define CHECK_MASK_TYPE u32
#define _FROM_TYPE u16
#elif COMPILE_READ_UCS_LEVEL == 1
#define CHECK_MASK_TYPE u64
#define _FROM_TYPE u8
#else
#error "COMPILE_READ_UCS_LEVEL must be 1, 2 or 4"
#endif

#if SIMD_BIT_SIZE < 512
#undef CHECK_MASK_TYPE
#define CHECK_MASK_TYPE SIMD_MASK_TYPE
#endif

#define CHECK_COUNT_MAX (SIMD_BIT_SIZE / 8 / sizeof(_FROM_TYPE))

// read only
#define CHECK_ESCAPE_IMPL_GET_MASK PYYJSON_CONCAT2(check_escape_impl_get_mask, COMPILE_READ_UCS_LEVEL)
#define CHECK_MASK_ZERO PYYJSON_CONCAT2(check_mask_zero, COMPILE_READ_UCS_LEVEL)
#define CHECK_MASK_ZERO_SMALL PYYJSON_CONCAT2(check_mask_zero_small, COMPILE_READ_UCS_LEVEL)
#define MASK_RIGHT_SHIFT PYYJSON_CONCAT2(mask_right_shift, COMPILE_READ_UCS_LEVEL)
#define SIMD_RIGHT_SHIFT PYYJSON_CONCAT2(simd_right_shift, COMPILE_READ_UCS_LEVEL)
#define CHECK_ESCAPE_TAIL_IMPL_GET_MASK_512 PYYJSON_CONCAT2(check_escape_tail_impl_get_mask_512, COMPILE_READ_UCS_LEVEL)
// write only or read-write
#define WRITE_SIMD_IMPL PYYJSON_CONCAT3(write_simd_impl, COMPILE_READ_UCS_LEVEL, COMPILE_WRITE_UCS_LEVEL)
#define WRITE_SIMD_256_WITH_WRITEMASK PYYJSON_CONCAT2(write_simd_256_with_writemask, COMPILE_WRITE_UCS_LEVEL)
#define WRITE_SIMD_WITH_TAIL_LEN PYYJSON_CONCAT3(write_simd_with_tail_len, COMPILE_READ_UCS_LEVEL, COMPILE_WRITE_UCS_LEVEL)
#define MASK_ELEVATE_WRITE_512 PYYJSON_CONCAT3(mask_elevate_write_512, COMPILE_READ_UCS_LEVEL, COMPILE_WRITE_UCS_LEVEL)

#if COMPILE_WRITE_UCS_LEVEL == 4
force_inline bool CHECK_MASK_ZERO_SMALL(SIMD_SMALL_MASK_TYPE small_mask) {
#if SIMD_BIT_SIZE == 512
    return small_mask == 0;
#elif SIMD_BIT_SIZE == 256
    return (bool) _mm_testz_si128(small_mask, small_mask); // ptest, sse4.1
#else
#if defined(__SSE4_1__)
    return (bool) _mm_testz_si128(small_mask, small_mask); // ptest, sse4.1
#else
    return _mm_movemask_epi8(_mm_cmpeq_epi8(small_mask, _mm_setzero_si128())) == 0xFFFF;
#endif
#endif
}
#endif // COMPILE_WRITE_UCS_LEVEL == 1

#if COMPILE_WRITE_UCS_LEVEL == 4
force_inline CHECK_MASK_TYPE CHECK_ESCAPE_IMPL_GET_MASK(const _FROM_TYPE *restrict src, SIMD_TYPE *restrict SIMD_VAR) {
#if COMPILE_READ_UCS_LEVEL == 1
#if SIMD_BIT_SIZE == 512
    *SIMD_VAR = load_512((const void *) src);
    SIMD_512 t1 = load_512_aligned((const void *) _Quote_i8);
    SIMD_512 t2 = load_512_aligned((const void *) _Slash_i8);
    SIMD_512 t3 = load_512_aligned((const void *) _ControlMax_i8);
    CHECK_MASK_TYPE m1 = _mm512_cmpeq_epi8_mask(*SIMD_VAR, t1); // AVX512BW
    CHECK_MASK_TYPE m2 = _mm512_cmpeq_epi8_mask(*SIMD_VAR, t2); // AVX512BW
    CHECK_MASK_TYPE m3 = _mm512_cmplt_epu8_mask(*SIMD_VAR, t3); // AVX512BW
    return m1 | m2 | m3;
#elif SIMD_BIT_SIZE == 256
    *SIMD_VAR = _mm256_loadu_si256((const __m256i_u *) src);
    __m256i t1 = _mm256_set1_epi8(_Quote); //_mm256_load_si256((__m256i *) _Quote_i8);      // vmovdqa, AVX
    __m256i t2 = _mm256_set1_epi8(_Slash); //_mm256_load_si256((__m256i *) _Slash_i8);      // vmovdqa, AVX
    // __m256i t3 = _mm256_load_si256((__m256i *) _MinusOne_i8);   // vmovdqa, AVX
    __m256i t4 = _mm256_set1_epi8(32);             //_mm256_load_si256((__m256i *) _ControlMax_i8); // vmovdqa, AVX
    __m256i m1 = _mm256_cmpeq_epi8(*SIMD_VAR, t1); // vpcmpeqb, AVX2
    __m256i m2 = _mm256_cmpeq_epi8(*SIMD_VAR, t2); // vpcmpeqb, AVX2
    // __m256i _1 = _mm256_cmpgt_epi8(*SIMD_VAR, t3);              // u > -1, vpcmpgtb, AVX2
    __m256i m3 = _mm256_subs_epu8(t4, *SIMD_VAR); // 32 > u, vpcmpgtb, AVX2
    // __m256i m3 = _mm256_and_si256(_1, _2);                      // vpand, AVX2
    __m256i r = _mm256_or_si256(_mm256_or_si256(m1, m2), m3);
    return r;
#else  // SIMD_BIT_SIZE
    *SIMD_VAR = load_128((const void *) src);
    SIMD_128 t1 = load_128_aligned((const void *) _Quote_i8);
    SIMD_128 t2 = load_128_aligned((const void *) _Slash_i8);
    SIMD_128 t4 = _mm_set1_epi8(32);
    SIMD_128 m1 = cmpeq_8_128(*SIMD_VAR, t1);
    SIMD_128 m2 = cmpeq_8_128(*SIMD_VAR, t2);
    SIMD_128 m3 = _mm_subs_epu8(t4, *SIMD_VAR);
    SIMD_128 r = simd_or_128(simd_or_128(m1, m2), m3);
    return r;
#endif // SIMD_BIT_SIZE
#elif COMPILE_READ_UCS_LEVEL == 2
#if SIMD_BIT_SIZE == 512
    *SIMD_VAR = load_512((const void *) src);
    SIMD_512 t1 = load_512_aligned((const void *) _Quote_i16);
    SIMD_512 t2 = load_512_aligned((const void *) _Slash_i16);
    SIMD_512 t3 = load_512_aligned((const void *) _ControlMax_i16);
    CHECK_MASK_TYPE m1 = _mm512_cmpeq_epi16_mask(*SIMD_VAR, t1); // AVX512BW
    CHECK_MASK_TYPE m2 = _mm512_cmpeq_epi16_mask(*SIMD_VAR, t2); // AVX512BW
    CHECK_MASK_TYPE m3 = _mm512_cmplt_epu16_mask(*SIMD_VAR, t3); // AVX512BW
    return m1 | m2 | m3;
#elif SIMD_BIT_SIZE == 256
    *SIMD_VAR = _mm256_loadu_si256((const __m256i_u *) src);
    __m256i t1 = _mm256_load_si256((__m256i *) _Quote_i16);      // vmovdqa, AVX
    __m256i t2 = _mm256_load_si256((__m256i *) _Slash_i16);      // vmovdqa, AVX
    __m256i t3 = _mm256_load_si256((__m256i *) _MinusOne_i16);   // vmovdqa, AVX
    __m256i t4 = _mm256_load_si256((__m256i *) _ControlMax_i16); // vmovdqa, AVX
    __m256i m1 = _mm256_cmpeq_epi16(*SIMD_VAR, t1);              // vpcmpeqb, AVX2
    __m256i m2 = _mm256_cmpeq_epi16(*SIMD_VAR, t2);              // vpcmpeqb, AVX2
    __m256i _1 = _mm256_cmpgt_epi16(*SIMD_VAR, t3);              // u > -1, vpcmpgtb, AVX2
    __m256i _2 = _mm256_cmpgt_epi16(t4, *SIMD_VAR);              // 32 > u, vpcmpgtb, AVX2
    __m256i m3 = _mm256_and_si256(_1, _2);                       // vpand, AVX2
    __m256i r = _mm256_or_si256(_mm256_or_si256(m1, m2), m3);
    return r;
#else  // SIMD_BIT_SIZE
    *SIMD_VAR = load_128((const void *) src);
    SIMD_128 t1 = load_128_aligned((const void *) _Quote_i16);
    SIMD_128 t2 = load_128_aligned((const void *) _Slash_i16);
    SIMD_128 t4 = _mm_set1_epi16(32);
    SIMD_128 m1 = cmpeq_16_128(*SIMD_VAR, t1);
    SIMD_128 m2 = cmpeq_16_128(*SIMD_VAR, t2);
    SIMD_128 m3 = _mm_subs_epu16(t4, *SIMD_VAR);
    SIMD_128 r = simd_or_128(simd_or_128(m1, m2), m3);
    return r;
#endif // SIMD_BIT_SIZE
#elif COMPILE_READ_UCS_LEVEL == 4
#if SIMD_BIT_SIZE == 512
    *SIMD_VAR = load_512((const void *) src);
    SIMD_512 t1 = load_512_aligned((const void *) _Quote_i32);
    SIMD_512 t2 = load_512_aligned((const void *) _Slash_i32);
    SIMD_512 t3 = load_512_aligned((const void *) _ControlMax_i32);
    CHECK_MASK_TYPE m1 = _mm512_cmpeq_epi32_mask(*SIMD_VAR, t1); // AVX512F
    CHECK_MASK_TYPE m2 = _mm512_cmpeq_epi32_mask(*SIMD_VAR, t2); // AVX512F
    CHECK_MASK_TYPE m3 = _mm512_cmplt_epu32_mask(*SIMD_VAR, t3); // AVX512F
    return m1 | m2 | m3;
#elif SIMD_BIT_SIZE == 256
    *SIMD_VAR = _mm256_loadu_si256((const __m256i_u *) src);
    __m256i t1 = _mm256_load_si256((__m256i *) _Quote_i32);      // vmovdqa, AVX
    __m256i t2 = _mm256_load_si256((__m256i *) _Slash_i32);      // vmovdqa, AVX
    __m256i t3 = _mm256_load_si256((__m256i *) _MinusOne_i32);   // vmovdqa, AVX
    __m256i t4 = _mm256_load_si256((__m256i *) _ControlMax_i32); // vmovdqa, AVX
    __m256i m1 = _mm256_cmpeq_epi32(*SIMD_VAR, t1);              // vpcmpeqb, AVX2
    __m256i m2 = _mm256_cmpeq_epi32(*SIMD_VAR, t2);              // vpcmpeqb, AVX2
    __m256i _1 = _mm256_cmpgt_epi32(*SIMD_VAR, t3);              // u > -1, vpcmpgtb, AVX2
    __m256i _2 = _mm256_cmpgt_epi32(t4, *SIMD_VAR);              // 32 > u, vpcmpgtb, AVX2
    __m256i m3 = _mm256_and_si256(_1, _2);                       // vpand, AVX2
    __m256i r = _mm256_or_si256(_mm256_or_si256(m1, m2), m3);
    return r;
#else  // SIMD_BIT_SIZE
    *SIMD_VAR = load_128((const void *) src);
    SIMD_128 t1 = load_128_aligned((const void *) _Quote_i32);
    SIMD_128 t2 = load_128_aligned((const void *) _Slash_i32);
    SIMD_128 t3 = load_128_aligned((const void *) _MinusOne_i32);
    SIMD_128 t4 = load_128_aligned((const void *) _ControlMax_i32);
    SIMD_128 m1 = cmpeq_32_128(*SIMD_VAR, t1);
    SIMD_128 m2 = cmpeq_32_128(*SIMD_VAR, t2);
    // unfortunately we don't have saturated minus for 32 bits
    SIMD_128 _1 = cmpgt_i32_128(*SIMD_VAR, t3);
    SIMD_128 _2 = cmpgt_i32_128(t4, *SIMD_VAR);
    SIMD_128 m3 = simd_and_128(_1, _2);
    SIMD_128 r = simd_or_128(simd_or_128(m1, m2), m3);
    return r;
#endif // SIMD_BIT_SIZE
#endif
}
#endif // COMPILE_WRITE_UCS_LEVEL == 1

#if COMPILE_WRITE_UCS_LEVEL == 4
force_inline bool CHECK_MASK_ZERO(CHECK_MASK_TYPE SIMD_VAR) {
#if SIMD_BIT_SIZE == 512
    return SIMD_VAR == 0;
#elif SIMD_BIT_SIZE == 256
    return (bool) _mm256_testz_si256(SIMD_VAR, SIMD_VAR);
#else
#if defined(__SSE4_1__)
    return (bool) _mm_testz_si128(SIMD_VAR, SIMD_VAR); // ptest, sse4.1
#else
    return _mm_movemask_epi8(_mm_cmpeq_epi8(SIMD_VAR, _mm_setzero_si128())) == 0xFFFF;
#endif
#endif
}
#endif // COMPILE_WRITE_UCS_LEVEL == 4

#if COMPILE_WRITE_UCS_LEVEL == 4
force_inline void MASK_RIGHT_SHIFT(SIMD_MASK_TYPE mask, usize shift_count, SIMD_MASK_TYPE *write_mask) {
#if SIMD_BIT_SIZE == 512
    *write_mask = mask >> (shift_count * sizeof(_FROM_TYPE));
#elif SIMD_BIT_SIZE == 256
    _FROM_TYPE xbuf[2 * CHECK_COUNT_MAX] = {0};
    _mm256_storeu_si256((__m256i_u *) (xbuf + CHECK_COUNT_MAX - shift_count), mask);
    *write_mask = _mm256_loadu_si256((const __m256i_u *) (xbuf + CHECK_COUNT_MAX));
#else

#endif
}
#endif // COMPILE_WRITE_UCS_LEVEL == 4

#if COMPILE_WRITE_UCS_LEVEL == 4
#if SIMD_BIT_SIZE == 512
CHECK_MASK_TYPE CHECK_ESCAPE_TAIL_IMPL_GET_MASK_512(SIMD_512 z, CHECK_MASK_TYPE rw_mask) {
#if COMPILE_READ_UCS_LEVEL == 1
    // *SIMD_VAR = load_512((const void *) src);
    SIMD_512 t1 = load_512_aligned((const void *) _Quote_i8);
    SIMD_512 t2 = load_512_aligned((const void *) _Slash_i8);
    SIMD_512 t3 = load_512_aligned((const void *) _ControlMax_i8);
    CHECK_MASK_TYPE m1 = _mm512_mask_cmpeq_epi8_mask(rw_mask, z, t1); // AVX512BW
    CHECK_MASK_TYPE m2 = _mm512_mask_cmpeq_epi8_mask(rw_mask, z, t2); // AVX512BW
    CHECK_MASK_TYPE m3 = _mm512_mask_cmplt_epu8_mask(rw_mask, z, t3); // AVX512BW
    return m1 | m2 | m3;
#elif COMPILE_READ_UCS_LEVEL == 2
    SIMD_512 t1 = load_512_aligned((const void *) _Quote_i16);
    SIMD_512 t2 = load_512_aligned((const void *) _Slash_i16);
    SIMD_512 t3 = load_512_aligned((const void *) _ControlMax_i16);
    CHECK_MASK_TYPE m1 = _mm512_mask_cmpeq_epi16_mask(rw_mask, z, t1); // AVX512BW
    CHECK_MASK_TYPE m2 = _mm512_mask_cmpeq_epi16_mask(rw_mask, z, t2); // AVX512BW
    CHECK_MASK_TYPE m3 = _mm512_mask_cmplt_epu16_mask(rw_mask, z, t3); // AVX512BW
    return m1 | m2 | m3;
#else
    SIMD_512 t1 = load_512_aligned((const void *) _Quote_i32);
    SIMD_512 t2 = load_512_aligned((const void *) _Slash_i32);
    SIMD_512 t3 = load_512_aligned((const void *) _ControlMax_i32);
    CHECK_MASK_TYPE m1 = _mm512_mask_cmpeq_epi32_mask(rw_mask, z, t1); // AVX512F
    CHECK_MASK_TYPE m2 = _mm512_mask_cmpeq_epi32_mask(rw_mask, z, t2); // AVX512F
    CHECK_MASK_TYPE m3 = _mm512_mask_cmplt_epu32_mask(rw_mask, z, t3); // AVX512F
    return m1 | m2 | m3;
#endif
}
#endif // SIMD_BIT_SIZE == 512
#endif // COMPILE_WRITE_UCS_LEVEL == 4

#if COMPILE_WRITE_UCS_LEVEL == 4

force_inline void SIMD_RIGHT_SHIFT(SIMD_TYPE SIMD_VAR, usize shift_count, SIMD_TYPE *write_var) {
#if SIMD_BIT_SIZE == 512
#else
    MASK_RIGHT_SHIFT(SIMD_VAR, shift_count, write_var);
#endif
}
#endif // COMPILE_WRITE_UCS_LEVEL == 4

force_inline void WRITE_SIMD_IMPL(_TARGET_TYPE *dst, SIMD_TYPE SIMD_VAR) {
#if COMPILE_READ_UCS_LEVEL == COMPILE_WRITE_UCS_LEVEL
    // write as it is
    write_simd((void *) dst, SIMD_VAR);
#else
#if COMPILE_READ_UCS_LEVEL == 2
// 2->4
#if SIMD_BIT_SIZE == 512
    // 256->512
    __m256i _y;
    __m512i _z;
    // 0
    _y = SIMD_EXTRACT_HALF(SIMD_VAR, 0);
    _z = elevate_2_4_to_512(_y);
    write_512(dst, _z);
    dst += CHECK_COUNT_MAX / 2;
    // 1
    _y = SIMD_EXTRACT_HALF(SIMD_VAR, 1);
    _z = elevate_2_4_to_512(_y);
    write_512(dst, _z);
    dst += CHECK_COUNT_MAX / 2;
#elif SIMD_BIT_SIZE == 256
    // 128->256
    __m128i _x;
    __m256i _y;
    // 0
    _x = SIMD_EXTRACT_HALF(SIMD_VAR, 0);
    _y = elevate_2_4_to_256(_x);
    write_simd((void *) dst, _y);
    dst += CHECK_COUNT_MAX / 2;
    // 1
    _x = SIMD_EXTRACT_HALF(SIMD_VAR, 1);
    _y = elevate_2_4_to_256(_x);
    write_simd((void *) dst, _y);
    dst += CHECK_COUNT_MAX / 2;
#else  // SIMD_BIT_SIZE == 128
    // 64(128)->128
    __m128i _x;
    // 0
    _x = elevate_2_4_to_128(SIMD_VAR);
    write_simd((void *) dst, _x);
    dst += CHECK_COUNT_MAX / 2;
    // 1
    RIGHT_SHIFT_128BITS(SIMD_VAR, 64, &SIMD_VAR);
    _x = elevate_2_4_to_128(SIMD_VAR);
    write_simd((void *) dst, _x);
    dst += CHECK_COUNT_MAX / 2;
#endif // SIMD_BIT_SIZE
#else  // COMPILE_READ_UCS_LEVEL == 1
#if COMPILE_WRITE_UCS_LEVEL == 2
// 1->2
#if SIMD_BIT_SIZE == 512
    // 256->512
    __m512i _z;
    __m256i _y;
    // 0
    _y = SIMD_EXTRACT_HALF(SIMD_VAR, 0);
    _z = elevate_1_2_to_512(_y);
    write_512(dst, _z);
    dst += CHECK_COUNT_MAX / 2;
    // 1
    _y = SIMD_EXTRACT_HALF(SIMD_VAR, 1);
    _z = elevate_1_2_to_512(_y);
    write_512(dst, _z);
    dst += CHECK_COUNT_MAX / 2;
#elif SIMD_BIT_SIZE == 256
    // 128->256
    __m128i _x;
    __m256i _y;
    _x = SIMD_EXTRACT_HALF(SIMD_VAR, 0);
    _y = elevate_1_2_to_256(_x);
    write_simd((void *) dst, _y);
    dst += CHECK_COUNT_MAX / 2;
    _x = SIMD_EXTRACT_HALF(SIMD_VAR, 1);
    _y = elevate_1_2_to_256(_x);
    write_simd((void *) dst, _y);
    dst += CHECK_COUNT_MAX / 2;
#else  // SIMD_BIT_SIZE == 128
    // 64(128)->128
    __m128i _x;
    _x = elevate_1_2_to_128(SIMD_VAR);
    write_simd((void *) dst, _x);
    dst += CHECK_COUNT_MAX / 2;
    RIGHT_SHIFT_128BITS(SIMD_VAR, 64, &SIMD_VAR);
    _x = elevate_1_2_to_128(SIMD_VAR);
    write_simd((void *) dst, _x);
    dst += CHECK_COUNT_MAX / 2;
#endif // SIMD_BIT_SIZE
#else  // COMPILE_WRITE_UCS_LEVEL == 4
// 1->4
#if SIMD_BIT_SIZE == 512
    // 128->512
    __m512i _z;
    __m128i _x;
    // 0
    _x = SIMD_EXTRACT_PART(SIMD_VAR, 0);
    _z = elevate_1_4_to_512(_x);
    write_512(dst, _z);
    dst += CHECK_COUNT_MAX / 4;
    // 1
    _x = SIMD_EXTRACT_PART(SIMD_VAR, 1);
    _z = elevate_1_4_to_512(_x);
    write_512(dst, _z);
    dst += CHECK_COUNT_MAX / 4;
    // 2
    _x = SIMD_EXTRACT_PART(SIMD_VAR, 2);
    _z = elevate_1_4_to_512(_x);
    write_512(dst, _z);
    dst += CHECK_COUNT_MAX / 4;
    // 3
    _x = SIMD_EXTRACT_PART(SIMD_VAR, 3);
    _z = elevate_1_4_to_512(_x);
    write_512(dst, _z);
    dst += CHECK_COUNT_MAX / 4;
#elif SIMD_BIT_SIZE == 256
    // 64(128)->256
    __m128i _x;
    __m256i _y;
    // 0
    _x = SIMD_EXTRACT_HALF(SIMD_VAR, 0);
    _y = elevate_1_4_to_256(_x);
    write_simd((void *) dst, _y);
    dst += CHECK_COUNT_MAX / 4;
    // 1
    RIGHT_SHIFT_128BITS(_x, 64, &_x);
    _y = elevate_1_4_to_256(_x);
    write_simd((void *) dst, _y);
    dst += CHECK_COUNT_MAX / 4;
    // 2
    _x = SIMD_EXTRACT_HALF(SIMD_VAR, 1);
    _y = elevate_1_4_to_256(_x);
    write_simd((void *) dst, _y);
    dst += CHECK_COUNT_MAX / 4;
    // 3
    RIGHT_SHIFT_128BITS(_x, 64, &_x);
    _y = elevate_1_4_to_256(_x);
    write_simd((void *) dst, _y);
    dst += CHECK_COUNT_MAX / 4;
#else  // SIMD_BIT_SIZE == 128
    // 32(128)->128
    // 0
    __m128i _x;
    _x = elevate_1_4_to_128(SIMD_VAR);
    write_simd((void *) dst, _x);
    dst += CHECK_COUNT_MAX / 4;
    // 1
    RIGHT_SHIFT_128BITS(SIMD_VAR, 32, &SIMD_VAR);
    _x = elevate_1_4_to_128(SIMD_VAR);
    write_simd((void *) dst, _x);
    dst += CHECK_COUNT_MAX / 4;
    // 2
    RIGHT_SHIFT_128BITS(SIMD_VAR, 32, &SIMD_VAR);
    _x = elevate_1_4_to_128(SIMD_VAR);
    write_simd((void *) dst, _x);
    dst += CHECK_COUNT_MAX / 4;
    // 3
    RIGHT_SHIFT_128BITS(SIMD_VAR, 32, &SIMD_VAR);
    _x = elevate_1_4_to_128(SIMD_VAR);
    write_simd((void *) dst, _x);
    dst += CHECK_COUNT_MAX / 4;
#endif // SIMD_BIT_SIZE
#endif

#endif
#endif
}


#if COMPILE_READ_UCS_LEVEL == 1
force_inline void WRITE_SIMD_256_WITH_WRITEMASK(_TARGET_TYPE *dst, SIMD_256 y, SIMD_256 mask) {
#if SIMD_BIT_SIZE == 256
#if COMPILE_WRITE_UCS_LEVEL == 4
    // we can use _mm256_maskstore_epi32
    _mm256_maskstore_epi32((i32 *) dst, mask, y);
#else
    // load-then-blend-then-write
    SIMD_256 blend;
    blend = load_256(dst);
    y = _mm256_blendv_epi8(blend, y, mask);
    _mm256_storeu_si256((__m256i *) dst, y);
#endif
#endif // SIMD_BIT_SIZE == 256
    Py_UNREACHABLE();
    assert(false);
}
#endif // COMPILE_READ_UCS_LEVEL == 1

#if SIMD_BIT_SIZE == 512 && COMPILE_READ_UCS_LEVEL != COMPILE_WRITE_UCS_LEVEL
force_inline void MASK_ELEVATE_WRITE_512(_TARGET_TYPE *dst, SIMD_512 z, Py_ssize_t len) {
#if COMPILE_READ_UCS_LEVEL == 1 && COMPILE_WRITE_UCS_LEVEL == 4
    assert(len < CHECK_COUNT_MAX && len > 0);
    assert(CHECK_COUNT_MAX == 64);
    usize parts = (usize) size_align_up(len, CHECK_COUNT_MAX / 4) / (CHECK_COUNT_MAX / 4);
    SIMD_128 x;
    SIMD_512 to_write;
    switch (parts) {
        case 4: {
            x = SIMD_EXTRACT_PART(z, 3);
            to_write = elevate_1_4_to_512(x);
            write_512((void *) (dst + (CHECK_COUNT_MAX / 4) * 3), to_write);
        }
        case 3: {
            x = SIMD_EXTRACT_PART(z, 2);
            to_write = elevate_1_4_to_512(x);
            write_512((void *) (dst + (CHECK_COUNT_MAX / 4) * 2), to_write);
        }
        case 2: {
            x = SIMD_EXTRACT_PART(z, 1);
            to_write = elevate_1_4_to_512(x);
            write_512((void *) (dst + (CHECK_COUNT_MAX / 4) * 1), to_write);
        }
        case 1: {
            x = SIMD_EXTRACT_PART(z, 0);
            to_write = elevate_1_4_to_512(x);
            write_512((void *) dst, to_write);
            break;
        }
        default: {
            Py_UNREACHABLE();
            assert(false);
        }
    }
#else
#if COMPILE_READ_UCS_LEVEL == 1
#define ELEVATOR elevate_1_2_to_512
#elif COMPILE_READ_UCS_LEVEL == 2
#define ELEVATOR elevate_2_4_to_512
#else
#error "Compiler unreachable code"
#endif
    assert(len < CHECK_COUNT_MAX && len > 0);
    usize parts = (usize) size_align_up(len, CHECK_COUNT_MAX / 2) / (CHECK_COUNT_MAX / 2);
    SIMD_256 y;
    SIMD_512 to_write;
    switch (parts) {
        case 2: {
            y = SIMD_EXTRACT_HALF(z, 1);
            to_write = ELEVATOR(y);
            write_512((void *) (dst + (CHECK_COUNT_MAX / 2) * 1), to_write);
        }
        case 1: {
            y = SIMD_EXTRACT_HALF(z, 0);
            to_write = ELEVATOR(y);
            write_512((void *) dst, to_write);
            break;
        }
        default: {
            Py_UNREACHABLE();
            assert(false);
        }
    }
#undef ELEVATOR
#endif
}
#endif // SIMD_BIT_SIZE == 512 && COMPILE_READ_UCS_LEVEL != COMPILE_WRITE_UCS_LEVEL


#undef MASK_ELEVATE_WRITE_512
#undef WRITE_SIMD_WITH_TAIL_LEN
#undef WRITE_SIMD_256_WITH_WRITEMASK
#undef WRITE_SIMD_IMPL
#undef CHECK_ESCAPE_TAIL_IMPL_GET_MASK_512
#undef SIMD_RIGHT_SHIFT
#undef MASK_RIGHT_SHIFT
#undef CHECK_MASK_ZERO_SMALL
#undef CHECK_MASK_ZERO
#undef CHECK_ESCAPE_IMPL_GET_MASK
#undef CHECK_COUNT_MAX
#undef _FROM_TYPE
#undef CHECK_MASK_TYPE
#undef _TARGET_TYPE
#undef _WRITER
