#include "simd/mask_table.h"
#include "simd/simd_detect.h"
#include <immintrin.h>

#include "commondef/r_in.inl.h"
#include "commondef/w_in.inl.h"


// write only or read-write
#define WRITE_SIMD_256_WITH_WRITEMASK PYYJSON_CONCAT2(write_simd_256_with_writemask, COMPILE_WRITE_UCS_LEVEL)
#define BACK_WRITE_SIMD256_WITH_TAIL_LEN PYYJSON_CONCAT3(back_write_simd256_with_tail_len, COMPILE_READ_UCS_LEVEL, COMPILE_WRITE_UCS_LEVEL)
#define MASK_ELEVATE_WRITE_512 PYYJSON_CONCAT3(mask_elevate_write_512, COMPILE_READ_UCS_LEVEL, COMPILE_WRITE_UCS_LEVEL)


#if COMPILE_READ_UCS_LEVEL == 1 && SIMD_BIT_SIZE == 256
force_inline void WRITE_SIMD_256_WITH_WRITEMASK(_TARGET_TYPE *dst, SIMD_256 y, SIMD_256 mask) {
#    if COMPILE_WRITE_UCS_LEVEL == 4
    // we can use _mm256_maskstore_epi32
    _mm256_maskstore_epi32((i32 *)dst, mask, y);
#    elif COMPILE_WRITE_UCS_LEVEL < 4
    // load-then-blend-then-write
    SIMD_256 blend;
    blend = load_256(dst);
    y = _mm256_blendv_epi8(blend, y, mask);
    _mm256_storeu_si256((__m256i *)dst, y);
#    else
#        error "Compiler unreachable code"
#    endif
}
#endif // COMPILE_READ_UCS_LEVEL == 1 && SIMD_BIT_SIZE == 256

#if SIMD_BIT_SIZE == 256 && COMPILE_READ_UCS_LEVEL != COMPILE_WRITE_UCS_LEVEL
force_inline void BACK_WRITE_SIMD256_WITH_TAIL_LEN(_TARGET_TYPE *dst, SIMD_256 y, Py_ssize_t len, UnicodeVector *vec) {
    // vec is not used, only for verifying addr
#    if COMPILE_READ_UCS_LEVEL == 1 && COMPILE_WRITE_UCS_LEVEL == 4
    // 1->4
    // 64(128)->256
    SIMD_256 writemask;
    Py_ssize_t part1, part2, part3, part4;
    SIMD_128 x1, x2, x3, x4;
    split_tail_len_four_parts(len, CHECK_COUNT_MAX, &part1, &part2, &part3, &part4);
    extract_256_four_parts(SIMD_VAR, &x1, &x2, &x3, &x4);
    // NOTE: for this case (x86_64 and COMPILE_WRITE_UCS_LEVEL is 4),
    // `write_simd_256_with_writemask_4` uses `_mm256_maskstore_epi32` to write.
    // There will be no invalid write as long as the mask table is correct.
    // also note that CHECK_COUNT_MAX / 4 * sizeof(u32) == SIMD_BIT_SIZE / 8 == 32
    static const Py_ssize_t write_count_max = CHECK_COUNT_MAX / 4; // is 8
    // 0
    writemask = load_256_aligned(read_tail_mask_table_32(write_count_max - part1));
    assert(part1 || testz_256(writemask));
    write_simd_256_with_writemask_4(dst, elevate_1_4_to_256(x1), writemask);
    dst += write_count_max;
    // 1
    writemask = load_256_aligned(read_tail_mask_table_32(write_count_max - part2));
    assert(part2 || testz_256(writemask));
    write_simd_256_with_writemask_4(dst, elevate_1_4_to_256(x2), writemask);
    dst += write_count_max;
    // 2
    writemask = load_256_aligned(read_tail_mask_table_32(write_count_max - part3));
    assert(part3 || testz_256(writemask));
    write_simd_256_with_writemask_4(dst, elevate_1_4_to_256(x3), writemask);
    dst += write_count_max;
    // 3
    writemask = load_256_aligned(read_tail_mask_table_32(write_count_max - part4));
    assert(part4 || testz_256(writemask));
    write_simd_256_with_writemask_4(dst, elevate_1_4_to_256(x4), writemask);
    dst += write_count_max;
#    else // COMPILE_READ_UCS_LEVEL != 1 || COMPILE_WRITE_UCS_LEVEL != 4
#        define MASK_TABLE_READER PYYJSON_CONCAT2(read_tail_mask_table, WRITE_BIT_SIZE)
#        define MASK_WRITER PYYJSON_CONCAT2(write_simd_256_with_writemask, COMPILE_WRITE_UCS_LEVEL)
#        define ELEVATOR PYYJSON_CONCAT5(elevate, COMPILE_READ_UCS_LEVEL, COMPILE_WRITE_UCS_LEVEL, to, SIMD_BIT_SIZE)
    // 128->256
    SIMD_256 writemask;
    Py_ssize_t part1, part2;
    SIMD_128 x1, x2;
    split_tail_len_two_parts(len, CHECK_COUNT_MAX, &part1, &part2);
    extract_256_two_parts(y, &x1, &x2);
    // NOTE: for the case COMPILE_WRITE_UCS_LEVEL is 4 (x86_64),
    // MASK_WRITER, i.e. `write_simd_256_with_writemask_4`
    // uses `_mm256_maskstore_epi32` to write.
    // There will be no invalid write as long as the mask table is correct.
    // 0
    if (COMPILE_WRITE_UCS_LEVEL == 4 || part1) {
        assert(COMPILE_WRITE_UCS_LEVEL == 4 || (Py_ssize_t)dst >= (Py_ssize_t)vec);
        writemask = load_256_aligned(MASK_TABLE_READER(CHECK_COUNT_MAX / 2 - part1));
        MASK_WRITER(dst, ELEVATOR(x1), writemask);
    }
    dst += CHECK_COUNT_MAX / 2;
    // 1
    if (COMPILE_WRITE_UCS_LEVEL == 4 || part2) {
        assert(COMPILE_WRITE_UCS_LEVEL == 4 || (Py_ssize_t)dst >= (Py_ssize_t)vec);
        writemask = load_256_aligned(MASK_TABLE_READER(CHECK_COUNT_MAX / 2 - part2));
        MASK_WRITER(dst, ELEVATOR(x2), writemask);
    }
    dst += CHECK_COUNT_MAX / 2;
#        undef ELEVATOR
#        undef MASK_TABLE_READER
#        undef MASK_WRITER
#    endif // COMPILE_READ_UCS_LEVEL, COMPILE_WRITE_UCS_LEVEL
}
#endif // SIMD_BIT_SIZE == 256 && COMPILE_READ_UCS_LEVEL != COMPILE_WRITE_UCS_LEVEL


#if SIMD_BIT_SIZE == 512 && COMPILE_READ_UCS_LEVEL != COMPILE_WRITE_UCS_LEVEL
force_inline void MASK_ELEVATE_WRITE_512(_TARGET_TYPE *dst, SIMD_512 z, Py_ssize_t len) {
#    if COMPILE_READ_UCS_LEVEL == 1 && COMPILE_WRITE_UCS_LEVEL == 4
    assert(len < CHECK_COUNT_MAX && len > 0);
    assert(CHECK_COUNT_MAX == 64);
    usize parts = (usize)size_align_up(len, CHECK_COUNT_MAX / 4) / (CHECK_COUNT_MAX / 4);
    SIMD_128 x;
    SIMD_512 to_write;
    switch (parts) {
        case 4: {
            x = SIMD_EXTRACT_QUARTER(z, 3);
            to_write = elevate_1_4_to_512(x);
            write_512((void *)(dst + (CHECK_COUNT_MAX / 4) * 3), to_write);
        }
        case 3: {
            x = SIMD_EXTRACT_QUARTER(z, 2);
            to_write = elevate_1_4_to_512(x);
            write_512((void *)(dst + (CHECK_COUNT_MAX / 4) * 2), to_write);
        }
        case 2: {
            x = SIMD_EXTRACT_QUARTER(z, 1);
            to_write = elevate_1_4_to_512(x);
            write_512((void *)(dst + (CHECK_COUNT_MAX / 4) * 1), to_write);
        }
        case 1: {
            x = SIMD_EXTRACT_QUARTER(z, 0);
            to_write = elevate_1_4_to_512(x);
            write_512((void *)dst, to_write);
            break;
        }
        default: {
            Py_UNREACHABLE();
            assert(false);
        }
    }
#    else
#        if COMPILE_READ_UCS_LEVEL == 1
#            define ELEVATOR elevate_1_2_to_512
#        elif COMPILE_READ_UCS_LEVEL == 2
#            define ELEVATOR elevate_2_4_to_512
#        else
#            error "Compiler unreachable code"
#        endif
    assert(len < CHECK_COUNT_MAX && len > 0);
    usize parts = (usize)size_align_up(len, CHECK_COUNT_MAX / 2) / (CHECK_COUNT_MAX / 2);
    SIMD_256 y;
    SIMD_512 to_write;
    switch (parts) {
        case 2: {
            y = SIMD_EXTRACT_HALF(z, 1);
            to_write = ELEVATOR(y);
            write_512((void *)(dst + (CHECK_COUNT_MAX / 2) * 1), to_write);
        }
        case 1: {
            y = SIMD_EXTRACT_HALF(z, 0);
            to_write = ELEVATOR(y);
            write_512((void *)dst, to_write);
            break;
        }
        default: {
            Py_UNREACHABLE();
            assert(false);
        }
    }
#        undef ELEVATOR
#    endif
}
#endif // SIMD_BIT_SIZE == 512 && COMPILE_READ_UCS_LEVEL != COMPILE_WRITE_UCS_LEVEL

#include "commondef/r_out.inl.h"
#include "commondef/w_out.inl.h"

#undef MASK_ELEVATE_WRITE_512
#undef BACK_WRITE_SIMD256_WITH_TAIL_LEN
#undef WRITE_SIMD_256_WITH_WRITEMASK
