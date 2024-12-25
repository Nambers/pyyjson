#include "simd/mask_table.h"
#include "simd/simd_detect.h"
#include <immintrin.h>

#include "commondef/r_in.inl.h"
#include "commondef/w_in.inl.h"


// read only
#define CHECK_ESCAPE_TAIL_IMPL_GET_MASK_512 PYYJSON_CONCAT2(check_escape_tail_impl_get_mask_512, COMPILE_READ_UCS_LEVEL)
// write only or read-write
#define WRITE_SIMD_IMPL PYYJSON_CONCAT3(write_simd_impl, COMPILE_READ_UCS_LEVEL, COMPILE_WRITE_UCS_LEVEL)
#define WRITE_SIMD_256_WITH_WRITEMASK PYYJSON_CONCAT2(write_simd_256_with_writemask, COMPILE_WRITE_UCS_LEVEL)
#define BACK_WRITE_SIMD256_WITH_TAIL_LEN PYYJSON_CONCAT3(back_write_simd256_with_tail_len, COMPILE_READ_UCS_LEVEL, COMPILE_WRITE_UCS_LEVEL)
#define MASK_ELEVATE_WRITE_512 PYYJSON_CONCAT3(mask_elevate_write_512, COMPILE_READ_UCS_LEVEL, COMPILE_WRITE_UCS_LEVEL)


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
#undef CHECK_COUNT_MAX
