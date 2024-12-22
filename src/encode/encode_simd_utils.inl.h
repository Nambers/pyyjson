#include "simd/mask_table.h"
#include "simd/simd_detect.h"
#include <immintrin.h>

#include "commondef/r_in.inl.h"
#include "commondef/w_in.inl.h"


// read only
#define CHECK_ESCAPE_IMPL_GET_MASK PYYJSON_CONCAT2(check_escape_impl_get_mask, COMPILE_READ_UCS_LEVEL)
#define CHECK_MASK_ZERO PYYJSON_CONCAT2(check_mask_zero, COMPILE_READ_UCS_LEVEL)
#define CHECK_ESCAPE_TAIL_IMPL_GET_MASK_512 PYYJSON_CONCAT2(check_escape_tail_impl_get_mask_512, COMPILE_READ_UCS_LEVEL)
// write only or read-write
#define WRITE_SIMD_IMPL PYYJSON_CONCAT3(write_simd_impl, COMPILE_READ_UCS_LEVEL, COMPILE_WRITE_UCS_LEVEL)
#define WRITE_SIMD_256_WITH_WRITEMASK PYYJSON_CONCAT2(write_simd_256_with_writemask, COMPILE_WRITE_UCS_LEVEL)
#define BACK_WRITE_SIMD256_WITH_TAIL_LEN PYYJSON_CONCAT3(back_write_simd256_with_tail_len, COMPILE_READ_UCS_LEVEL, COMPILE_WRITE_UCS_LEVEL)
#define MASK_ELEVATE_WRITE_512 PYYJSON_CONCAT3(mask_elevate_write_512, COMPILE_READ_UCS_LEVEL, COMPILE_WRITE_UCS_LEVEL)

#if COMPILE_WRITE_UCS_LEVEL == 4
force_inline SIMD_MASK_TYPE CHECK_ESCAPE_IMPL_GET_MASK(const _FROM_TYPE *src, SIMD_TYPE *restrict SIMD_VAR) {
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
    SIMD_MASK_TYPE m1 = CMPEQ(*z, t1); // AVX512BW, AVX512F
    SIMD_MASK_TYPE m2 = CMPEQ(*z, t2); // AVX512BW, AVX512F
    SIMD_MASK_TYPE m3 = CMPLT(*z, t3); // AVX512BW, AVX512F
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
force_inline bool CHECK_MASK_ZERO(SIMD_MASK_TYPE mask) {
#if SIMD_BIT_SIZE == 512
    return mask == 0;
#elif SIMD_BIT_SIZE == 256
    return (bool) _mm256_testz_si256(mask, mask);
#else
#if defined(__SSE4_1__)
    return (bool) _mm_testz_si128(mask, mask); // ptest, sse4.1
#else
    return _mm_movemask_epi8(_mm_cmpeq_epi8(mask, _mm_setzero_si128())) == 0xFFFF;
#endif
#endif
}
#endif // COMPILE_WRITE_UCS_LEVEL == 4


#if COMPILE_WRITE_UCS_LEVEL == 4
#if SIMD_BIT_SIZE == 512
force_inline SIMD_MASK_TYPE CHECK_ESCAPE_TAIL_IMPL_GET_MASK_512(SIMD_512 z, SIMD_MASK_TYPE rw_mask) {
#define CUR_QUOTE PYYJSON_SIMPLE_CONCAT2(_Quote_i, READ_BIT_SIZE)
#define CUR_SLASH PYYJSON_SIMPLE_CONCAT2(_Slash_i, READ_BIT_SIZE)
#define CUR_CONTROL_MAX PYYJSON_SIMPLE_CONCAT2(_ControlMax_i, READ_BIT_SIZE)
#define CMPEQ PYYJSON_SIMPLE_CONCAT3(_mm512_mask_cmpeq_epi, READ_BIT_SIZE, _mask)
#define CMPLT PYYJSON_SIMPLE_CONCAT3(_mm512_mask_cmplt_epu, READ_BIT_SIZE, _mask)
    SIMD_512 t1 = load_512_aligned((const void *) CUR_QUOTE);
    SIMD_512 t2 = load_512_aligned((const void *) CUR_SLASH);
    SIMD_512 t3 = load_512_aligned((const void *) CUR_CONTROL_MAX);
    SIMD_MASK_TYPE m1 = CMPEQ(rw_mask, z, t1); // AVX512BW / AVX512F
    SIMD_MASK_TYPE m2 = CMPEQ(rw_mask, z, t2); // AVX512BW / AVX512F
    SIMD_MASK_TYPE m3 = CMPLT(rw_mask, z, t3); // AVX512BW / AVX512F
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
#if COMPILE_READ_UCS_LEVEL == 1 && COMPILE_WRITE_UCS_LEVEL == 4
#define EXTRACTOR PYYJSON_CONCAT3(extract, SIMD_BIT_SIZE, four_parts)
#define ELEVATOR PYYJSON_CONCAT5(elevate, COMPILE_READ_UCS_LEVEL, COMPILE_WRITE_UCS_LEVEL, to, SIMD_BIT_SIZE)
    SIMD_128 x1, x2, x3, x4;
    EXTRACTOR(SIMD_VAR, &x1, &x2, &x3, &x4);
    // 0
    write_simd((void *) dst, ELEVATOR(x1));
    dst += CHECK_COUNT_MAX / 4;
    // 1
    write_simd((void *) dst, ELEVATOR(x2));
    dst += CHECK_COUNT_MAX / 4;
    // 2
    write_simd((void *) dst, ELEVATOR(x3));
    dst += CHECK_COUNT_MAX / 4;
    // 3
    write_simd((void *) dst, ELEVATOR(x4));
    dst += CHECK_COUNT_MAX / 4;
#undef ELEVATOR
#undef EXTRACTOR
#else
#define EXTRACTOR PYYJSON_CONCAT3(extract, SIMD_BIT_SIZE, two_parts)
#define ELEVATOR PYYJSON_CONCAT5(elevate, COMPILE_READ_UCS_LEVEL, COMPILE_WRITE_UCS_LEVEL, to, SIMD_BIT_SIZE)
    SIMD_HALF_TYPE v1, v2;
    EXTRACTOR(SIMD_VAR, &v1, &v2);
    // 0
    write_simd((void *) dst, ELEVATOR(v1));
    dst += CHECK_COUNT_MAX / 2;
    // 1
    write_simd((void *) dst, ELEVATOR(v2));
    dst += CHECK_COUNT_MAX / 2;
#undef ELEVATOR
#undef EXTRACTOR
#endif // r->w == 1->4
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

#if SIMD_BIT_SIZE == 256 && COMPILE_READ_UCS_LEVEL != COMPILE_WRITE_UCS_LEVEL
force_inline void BACK_WRITE_SIMD256_WITH_TAIL_LEN(_TARGET_TYPE *dst, SIMD_256 y, Py_ssize_t len) {
#define MASK_TABLE_READ PYYJSON_CONCAT2(mask_table_read, WRITE_BIT_SIZE)
#define MASK_WRITER PYYJSON_CONCAT2(write_simd_256_with_writemask, COMPILE_WRITE_UCS_LEVEL)
#define ELEVATOR PYYJSON_CONCAT5(elevate, COMPILE_READ_UCS_LEVEL, COMPILE_WRITE_UCS_LEVEL, to, SIMD_BIT_SIZE)
#if COMPILE_READ_UCS_LEVEL == 1 && COMPILE_WRITE_UCS_LEVEL == 4
    // 1->4
    // 64(128)->256
    SIMD_256 writemask;
    Py_ssize_t part1, part2, part3, part4;
    SIMD_128 x1, x2, x3, x4;
    split_tail_len_four_parts(len, CHECK_COUNT_MAX, &part1, &part2, &part3, &part4);
    extract_256_four_parts(SIMD_VAR, &x1, &x2, &x3, &x4);
    // 0
    ;
    writemask = load_256_aligned(MASK_TABLE_READ(CHECK_COUNT_MAX / 4 - part1));
    MASK_WRITER(dst, ELEVATOR(x1), writemask);
    dst += CHECK_COUNT_MAX / 4;
    // 1
    writemask = load_256_aligned(MASK_TABLE_READ(CHECK_COUNT_MAX / 4 - part2));
    MASK_WRITER(dst, ELEVATOR(x2), writemask);
    dst += CHECK_COUNT_MAX / 4;
    // 2
    writemask = load_256_aligned(MASK_TABLE_READ(CHECK_COUNT_MAX / 4 - part3));
    MASK_WRITER(dst, ELEVATOR(x3), writemask);
    dst += CHECK_COUNT_MAX / 4;
    // 3
    writemask = load_256_aligned(MASK_TABLE_READ(CHECK_COUNT_MAX / 4 - part4));
    MASK_WRITER(dst, ELEVATOR(x4), writemask);
    dst += CHECK_COUNT_MAX / 4;
#else  // COMPILE_READ_UCS_LEVEL != 1 || COMPILE_WRITE_UCS_LEVEL != 4
    // 128->256
    SIMD_256 writemask;
    Py_ssize_t part1, part2;
    SIMD_128 x1, x2;
    split_tail_len_two_parts(len, CHECK_COUNT_MAX, &part1, &part2);
    extract_256_two_parts(y, &x1, &x2);
    // 0

    writemask = load_256_aligned(MASK_TABLE_READ(CHECK_COUNT_MAX / 2 - part1));
    MASK_WRITER(dst, ELEVATOR(x1), writemask);
    dst += CHECK_COUNT_MAX / 2;
    // 1
    writemask = load_256_aligned(MASK_TABLE_READ(CHECK_COUNT_MAX / 2 - part2));
    MASK_WRITER(dst, ELEVATOR(x2), writemask);
    dst += CHECK_COUNT_MAX / 2;
#endif // COMPILE_READ_UCS_LEVEL, COMPILE_WRITE_UCS_LEVEL
#undef ELEVATOR
#undef MASK_TABLE_READ
#undef MASK_WRITER
}
#endif // SIMD_BIT_SIZE == 256 && COMPILE_READ_UCS_LEVEL != COMPILE_WRITE_UCS_LEVEL


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
            x = SIMD_EXTRACT_QUARTER(z, 3);
            to_write = elevate_1_4_to_512(x);
            write_512((void *) (dst + (CHECK_COUNT_MAX / 4) * 3), to_write);
        }
        case 3: {
            x = SIMD_EXTRACT_QUARTER(z, 2);
            to_write = elevate_1_4_to_512(x);
            write_512((void *) (dst + (CHECK_COUNT_MAX / 4) * 2), to_write);
        }
        case 2: {
            x = SIMD_EXTRACT_QUARTER(z, 1);
            to_write = elevate_1_4_to_512(x);
            write_512((void *) (dst + (CHECK_COUNT_MAX / 4) * 1), to_write);
        }
        case 1: {
            x = SIMD_EXTRACT_QUARTER(z, 0);
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

#include "commondef/r_out.inl.h"
#include "commondef/w_out.inl.h"

#undef MASK_ELEVATE_WRITE_512
#undef BACK_WRITE_SIMD256_WITH_TAIL_LEN
#undef WRITE_SIMD_256_WITH_WRITEMASK
#undef WRITE_SIMD_IMPL
#undef CHECK_ESCAPE_TAIL_IMPL_GET_MASK_512
#undef CHECK_MASK_ZERO
#undef CHECK_ESCAPE_IMPL_GET_MASK
#undef CHECK_COUNT_MAX
