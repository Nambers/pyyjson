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
#define READ_BIT_SIZE 32
#elif COMPILE_READ_UCS_LEVEL == 2
#define CHECK_MASK_TYPE u32
#define _FROM_TYPE u16
#define READ_BIT_SIZE 16
#elif COMPILE_READ_UCS_LEVEL == 1
#define CHECK_MASK_TYPE u64
#define _FROM_TYPE u8
#define READ_BIT_SIZE 8
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
force_inline CHECK_MASK_TYPE CHECK_ESCAPE_IMPL_GET_MASK(const _FROM_TYPE *src, SIMD_TYPE *restrict SIMD_VAR) {
#if SIMD_BIT_SIZE == 512
#define CUR_QUOTE PYYJSON_SIMPLE_CONCAT2(_Quote_i, READ_BIT_SIZE)
#define CUR_SLASH PYYJSON_SIMPLE_CONCAT2(_Slash_i, READ_BIT_SIZE)
#define CUR_CONTROL_MAX PYYJSON_SIMPLE_CONCAT2(_ControlMax_i, READ_BIT_SIZE)
#define CMPEQ PYYJSON_SIMPLE_CONCAT3(_mm512_cmpeq_epi, READ_BIT_SIZE, _mask)
#define CMPLT PYYJSON_SIMPLE_CONCAT3(_mm512_cmplt_epu, READ_BIT_SIZE, _mask)
    *z = load_512((const void *) src);
    SIMD_512 t1 = load_512_aligned((const void *) CUR_QUOTE);
    SIMD_512 t2 = load_512_aligned((const void *) CUR_SLASH);
    SIMD_512 t3 = load_512_aligned((const void *) CUR_CONTROL_MAX);
    CHECK_MASK_TYPE m1 = CMPEQ(*z, t1); // AVX512BW, AVX512F
    CHECK_MASK_TYPE m2 = CMPEQ(*z, t2); // AVX512BW, AVX512F
    CHECK_MASK_TYPE m3 = CMPLT(*z, t3); // AVX512BW, AVX512F
    return m1 | m2 | m3;
#undef CMPLT
#undef CMPEQ
#undef CUR_CONTROL_MAX
#undef CUR_SLASH
#undef CUR_QUOTE
#elif SIMD_BIT_SIZE == 256
#define SET1 PYYJSON_SIMPLE_CONCAT2(_mm256_set1_epi, READ_BIT_SIZE)
#define CMPEQ PYYJSON_SIMPLE_CONCAT2(_mm256_cmpeq_epi, READ_BIT_SIZE)
#define SUBS PYYJSON_SIMPLE_CONCAT2(_mm256_subs_epu, READ_BIT_SIZE)
    *y = load_256((const void *) src);
    __m256i t1 = SET1(_Quote);
    __m256i t2 = SET1(_Slash);
    __m256i t4 = SET1(32);
    __m256i m1 = CMPEQ(*y, t1);
    __m256i m2 = CMPEQ(*y, t2);
#if COMPILE_READ_UCS_LEVEL != 4
    __m256i m3 = SUBS(t4, *y);
#else  // COMPILE_READ_UCS_LEVEL == 4
    // there is no `_mm256_subs_epu32`
    __m256i t3 = SET1(_MinusOne);
    __m256i _1 = _mm256_cmpgt_epi32(*y, t3);
    __m256i _2 = _mm256_cmpgt_epi32(t4, *y);
    __m256i m3 = simd_and_256(_1, _2);
#endif // COMPILE_READ_UCS_LEVEL
    __m256i r = simd_or_256(simd_or_256(m1, m2), m3);
    return r;
#undef SUBS
#undef CMPEQ
#undef SET1
#elif SIMD_BIT_SIZE == 128
#define SET1 PYYJSON_SIMPLE_CONCAT2(_mm_set1_epi, READ_BIT_SIZE)
#define CMPEQ PYYJSON_CONCAT3(cmpeq, READ_BIT_SIZE, 128)
#define SUBS PYYJSON_SIMPLE_CONCAT2(_mm_subs_epu, READ_BIT_SIZE)
    *x = load_128((const void *) src);
    SIMD_128 t1 = SET1(_Quote);
    SIMD_128 t2 = SET1(_Slash);
    SIMD_128 t4 = SET1(ControlMax);
    SIMD_128 m1 = CMPEQ(*x, t1);
    SIMD_128 m2 = CMPEQ(*x, t2);
#if COMPILE_READ_UCS_LEVEL != 4
    SIMD_128 m3 = SUBS(t4, *x);
#else  // COMPILE_READ_UCS_LEVEL == 4
    // there is no `_mm_subs_epu32`
    SIMD_128 t3 = SET1(-1);
    SIMD_128 _1 = cmpgt_i32_128(*x, t3);
    SIMD_128 _2 = cmpgt_i32_128(t4, *x);
    SIMD_128 m3 = simd_and_128(_1, _2);
#endif // COMPILE_READ_UCS_LEVEL
    SIMD_128 r = simd_or_128(simd_or_128(m1, m2), m3);
    return r;
#undef SUBS
#undef CMPEQ
#undef SET1
#endif // SIMD_BIT_SIZE
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
#if SIMD_BIT_SIZE == 512
CHECK_MASK_TYPE CHECK_ESCAPE_TAIL_IMPL_GET_MASK_512(SIMD_512 z, CHECK_MASK_TYPE rw_mask) {
#define CUR_QUOTE PYYJSON_SIMPLE_CONCAT2(_Quote_i, READ_BIT_SIZE)
#define CUR_SLASH PYYJSON_SIMPLE_CONCAT2(_Slash_i, READ_BIT_SIZE)
#define CUR_CONTROL_MAX PYYJSON_SIMPLE_CONCAT2(_ControlMax_i, READ_BIT_SIZE)
#define CMPEQ PYYJSON_SIMPLE_CONCAT3(_mm512_mask_cmpeq_epi, READ_BIT_SIZE, _mask)
#define CMPLT PYYJSON_SIMPLE_CONCAT3(_mm512_mask_cmplt_epu, READ_BIT_SIZE, _mask)
    SIMD_512 t1 = load_512_aligned((const void *) CUR_QUOTE);
    SIMD_512 t2 = load_512_aligned((const void *) CUR_SLASH);
    SIMD_512 t3 = load_512_aligned((const void *) CUR_CONTROL_MAX);
    CHECK_MASK_TYPE m1 = CMPEQ(rw_mask, z, t1); // AVX512BW / AVX512F
    CHECK_MASK_TYPE m2 = CMPEQ(rw_mask, z, t2); // AVX512BW / AVX512F
    CHECK_MASK_TYPE m3 = CMPLT(rw_mask, z, t3); // AVX512BW / AVX512F
    return m1 | m2 | m3;
#undef CMPLT
#undef CMPEQ
#undef CUR_CONTROL_MAX
#undef CUR_SLASH
#undef CUR_QUOTE
}
#endif // SIMD_BIT_SIZE == 512
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

#if COMPILE_READ_UCS_LEVEL == 1 && SIMD_BIT_SIZE == 256
force_inline void WRITE_SIMD_256_WITH_WRITEMASK(_TARGET_TYPE *dst, SIMD_256 y, SIMD_256 mask) {
#if COMPILE_WRITE_UCS_LEVEL == 4
    // we can use _mm256_maskstore_epi32
    _mm256_maskstore_epi32((i32 *) dst, mask, y);
#elif COMPILE_WRITE_UCS_LEVEL < 4
    // load-then-blend-then-write
    SIMD_256 blend;
    blend = load_256(dst);
    y = _mm256_blendv_epi8(blend, y, mask);
    _mm256_storeu_si256((__m256i *) dst, y);
#else
#error "Compiler unreachable code"
#endif
}
#endif // COMPILE_READ_UCS_LEVEL == 1 && SIMD_BIT_SIZE == 256

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
#undef CHECK_MASK_ZERO_SMALL
#undef CHECK_MASK_ZERO
#undef CHECK_ESCAPE_IMPL_GET_MASK
#undef CHECK_COUNT_MAX
#undef READ_BIT_SIZE
#undef _FROM_TYPE
#undef CHECK_MASK_TYPE
#undef _TARGET_TYPE
#undef _WRITER
