// requires: READ, WRITE

#include "commondef/rw_in.inl.h"
#include "simd_impl.h"

#define WRITE_SIMD_IMPL PYYJSON_CONCAT3(write_simd_impl, COMPILE_READ_UCS_LEVEL, COMPILE_WRITE_UCS_LEVEL)
#define TAIL_WRITE_SIMD_IMPL PYYJSON_CONCAT3(tail_write_simd_impl, COMPILE_READ_UCS_LEVEL, COMPILE_WRITE_UCS_LEVEL)

#define WRITE_PARTIAL_HEAD PYYJSON_CONCAT2(write_partial_head, COMPILE_WRITE_UCS_LEVEL)
#define WRITE_PARTIAL_TAIL PYYJSON_CONCAT2(write_partial_tail, COMPILE_WRITE_UCS_LEVEL)

#if PYYJSON_X86
#    if SIMD_BIT_SIZE == 512 || !PYYJSON_HAS_BLENDV
force_inline void WRITE_PARTIAL_HEAD(void *restrict dst, SIMD_TYPE SIMD_VAR, Py_ssize_t head_cnt);
#    else
force_inline void WRITE_PARTIAL_TAIL(void *restrict dst, SIMD_TYPE SIMD_VAR, Py_ssize_t tail_cnt);
#    endif
#elif PYYJSON_AARCH
force_inline void WRITE_PARTIAL_HEAD(void *restrict dst, poly128_t SIMD_VAR, Py_ssize_t head_cnt);
#endif

force_inline void WRITE_SIMD_IMPL(_TARGET_TYPE *dst, SIMD_TYPE SIMD_VAR) {
#if COMPILE_READ_UCS_LEVEL == COMPILE_WRITE_UCS_LEVEL
    // write as it is
    write_simd((void *)dst, SIMD_VAR);
#else
#    if COMPILE_READ_UCS_LEVEL == 1 && COMPILE_WRITE_UCS_LEVEL == 4
#        define EXTRACTOR PYYJSON_CONCAT3(extract, SIMD_BIT_SIZE, four_parts)
#        define ELEVATOR PYYJSON_CONCAT5(elevate, COMPILE_READ_UCS_LEVEL, COMPILE_WRITE_UCS_LEVEL, to, SIMD_BIT_SIZE)
    SIMD_128 x1, x2, x3, x4;
    EXTRACTOR(SIMD_VAR, &x1, &x2, &x3, &x4);
    // 0
    write_simd((void *)dst, ELEVATOR(x1));
    dst += READ_BATCH_COUNT / 4;
    // 1
    write_simd((void *)dst, ELEVATOR(x2));
    dst += READ_BATCH_COUNT / 4;
    // 2
    write_simd((void *)dst, ELEVATOR(x3));
    dst += READ_BATCH_COUNT / 4;
    // 3
    write_simd((void *)dst, ELEVATOR(x4));
    dst += READ_BATCH_COUNT / 4;
#        undef ELEVATOR
#        undef EXTRACTOR
#    else
#        define EXTRACTOR PYYJSON_CONCAT3(extract, SIMD_BIT_SIZE, two_parts)
#        define ELEVATOR PYYJSON_CONCAT5(elevate, COMPILE_READ_UCS_LEVEL, COMPILE_WRITE_UCS_LEVEL, to, SIMD_BIT_SIZE)
    SIMD_HALF_TYPE v1, v2;
    EXTRACTOR(SIMD_VAR, &v1, &v2);
    // 0
    write_simd((void *)dst, ELEVATOR(v1));
    dst += READ_BATCH_COUNT / 2;
    // 1
    write_simd((void *)dst, ELEVATOR(v2));
    dst += READ_BATCH_COUNT / 2;
#        undef ELEVATOR
#        undef EXTRACTOR
#    endif // r->w == 1->4
#endif
}

force_inline void TAIL_WRITE_SIMD_IMPL(const _FROM_TYPE *src, _TARGET_TYPE *dst, Py_ssize_t tail_count) {
    // step 1. load the src.
#if SIMD_BIT_SIZE == 512
#    define _LOAD_WITH_MASKU 1
#    define _NEED_RUNTIME_SHIFT 0
#    define _WRITE_HEAD_FIRST 1
#elif PYYJSON_HAS_BLENDV
#    define _LOAD_WITH_MASKU 0
#    define _NEED_RUNTIME_SHIFT 0
#    define _WRITE_HEAD_FIRST 0
#else
#    define _LOAD_WITH_MASKU 0
#    define _NEED_RUNTIME_SHIFT 1
#    define _WRITE_HEAD_FIRST 1
#endif

#if _LOAD_WITH_MASKU
    static_assert(SIMD_BIT_SIZE == 512, "SIMD_BIT_SIZE == 512");
#    define _MASKZ_LOADU PYYJSON_SIMPLE_CONCAT2(_mm512_maskz_loadu_epi, READ_BIT_SIZE)
    SIMD_TYPE SIMD_VAR = _MASKZ_LOADU(((READ_512_MASK_TYPE)1 << tail_count) - 1, (const void *)src);
#    undef _MASKZ_LOADU
#else
    const _FROM_TYPE *load_start = src + tail_count - READ_BATCH_COUNT;
    SIMD_TYPE SIMD_VAR = load_simd((const void *)load_start);
#endif
#if _NEED_RUNTIME_SHIFT
    static_assert(SIMD_BIT_SIZE == 128, "SIMD_BIT_SIZE == 128");
    SIMD_VAR = runtime_right_shift_128bits(SIMD_VAR, (READ_BATCH_COUNT - tail_count) * sizeof(_TARGET_TYPE));
#endif

#undef _LOAD_WITH_MASKU
#undef _NEED_RUNTIME_SHIFT

    // step 2. extract parts.
    SIMD_SUB_TYPE base[WR_DIV];
    Py_ssize_t tail_split[WR_DIV];
#if WR_DIV == 1
#    define EXTRACTOR
    base[0] = SIMD_VAR;
    tail_split[0] = tail_count;
#elif WR_DIV == 2
#    define EXTRACTOR PYYJSON_CONCAT3(extract, SIMD_BIT_SIZE, two_parts)

    EXTRACTOR(SIMD_VAR, &base[0], &base[1]);
#    if _WRITE_HEAD_FIRST
    split_tail_len_two_parts(tail_count, READ_BATCH_COUNT, &tail_split[1], &tail_split[0]);
#    else
    split_tail_len_two_parts(tail_count, READ_BATCH_COUNT, &tail_split[0], &tail_split[1]);
#    endif
#else // WR_DIV == 4
#    define EXTRACTOR PYYJSON_CONCAT3(extract, SIMD_BIT_SIZE, four_parts)
    EXTRACTOR(SIMD_VAR, &base[0], &base[1], &base[2], &base[3]);
#    if _WRITE_HEAD_FIRST
    split_tail_len_four_parts(tail_count, READ_BATCH_COUNT, &tail_split[3], &tail_split[2], &tail_split[1], &tail_split[0]);
#    else
    split_tail_len_four_parts(tail_count, READ_BATCH_COUNT, &tail_split[0], &tail_split[1], &tail_split[2], &tail_split[3]);
#    endif
#endif
#undef EXTRACTOR

// step 3. elevate and write
#if WR_DIV != 1
#    define ELEVATOR PYYJSON_CONCAT5(elevate, COMPILE_READ_UCS_LEVEL, COMPILE_WRITE_UCS_LEVEL, to, SIMD_BIT_SIZE)
#else
#    define ELEVATOR
#endif
#if !_WRITE_HEAD_FIRST
    dst = dst + tail_count - READ_BATCH_COUNT;
#endif
    for (usize i = 0; i < WR_DIV; ++i) {
#if _WRITE_HEAD_FIRST
        WRITE_PARTIAL_HEAD(dst, ELEVATOR(base[i]), tail_split[i]);
#else
        WRITE_PARTIAL_TAIL(dst, ELEVATOR(base[i]), tail_split[i]);
#endif
        dst += READ_BATCH_COUNT / WR_DIV;
    }
#undef ELEVATOR
#undef _WRITE_HEAD_FIRST

    // #if SIMD_BIT_SIZE == 512
    //     // step 1. load the src. Use mask load to avoid invalid read.
    // #    define _MASKZ_LOADU PYYJSON_SIMPLE_CONCAT2(_mm512_maskz_loadu_epi, READ_BIT_SIZE)
    //     SIMD_TYPE SIMD_VAR = _MASKZ_LOADU(((READ_512_MASK_TYPE)1 << tail_count) - 1, (const void *)src);
    // #    undef _MASKZ_LOADU

    //     // step 2. extract parts. Note that we reverse the `split_tail_len` result here
    //     // to achieve `split_head_len`
    //     SIMD_SUB_TYPE base[WR_DIV];
    //     Py_ssize_t tail_split[WR_DIV];
    // #    if WR_DIV == 1
    // #        define EXTRACTOR
    //     base[0] = SIMD_VAR;
    //     tail_split[0] = tail_count;
    // #    elif WR_DIV == 2
    // #        define EXTRACTOR PYYJSON_CONCAT3(extract, SIMD_BIT_SIZE, two_parts)
    //     EXTRACTOR(SIMD_VAR, &base[0], &base[1]);
    //     split_tail_len_two_parts(tail_count, READ_BATCH_COUNT, &tail_split[1], &tail_split[0]);
    // #    else
    // #        define EXTRACTOR PYYJSON_CONCAT3(extract, SIMD_BIT_SIZE, four_parts)
    //     EXTRACTOR(SIMD_VAR, &base[0], &base[1], &base[2], &base[3]);
    //     split_tail_len_four_parts(tail_count, READ_BATCH_COUNT, &tail_split[3], &tail_split[2], &tail_split[1], &tail_split[0]);
    // #    endif
    // #    undef EXTRACTOR

    //     // step 3. elevate and write
    //     for (usize i = 0; i < WR_DIV; ++i) {
    //         WRITE_PARTIAL_HEAD(dst, ELEVATOR(base[i]), tail_split[i]);
    //         dst += READ_BATCH_COUNT / WR_DIV;
    //     }
    // #elif WRITE_SUPPORT_MASK_WRITE || PYYJSON_HAS_BLENDV
    //     // avx2 and _TARGET_TYPE is u32 || sse4.1 or above

    //     // step 1. load the src.
    //     // since we can ensure the 32 bytes before src is always readable,
    //     // load directly
    //     static_assert(SIMD_BIT_SIZE < 512);
    //     _FROM_TYPE *load_start = src + tail_count - READ_BATCH_COUNT;
    //     SIMD_TYPE SIMD_VAR = simd_load((const void *)src);

    //     // step 2. extract parts
    //     SIMD_SUB_TYPE base[WR_DIV];
    //     Py_ssize_t tail_split[WR_DIV];
    // #    if WR_DIV == 1
    // #        define EXTRACTOR
    //     base[0] = SIMD_VAR;
    //     tail_split[0] = tail_count;
    // #    elif WR_DIV == 2
    // #        define EXTRACTOR PYYJSON_CONCAT3(extract, SIMD_BIT_SIZE, two_parts)
    //     EXTRACTOR(SIMD_VAR, &base[0], &base[1]);
    //     split_tail_len_two_parts(tail_count, READ_BATCH_COUNT, &tail_split[0], &tail_split[1]);
    // #    else
    // #        define EXTRACTOR PYYJSON_CONCAT3(extract, SIMD_BIT_SIZE, four_parts)
    //     EXTRACTOR(SIMD_VAR, &base[0], &base[1], &base[2], &base[3]);
    //     split_tail_len_four_parts(tail_count, READ_BATCH_COUNT, &tail_split[0], &tail_split[1], &tail_split[2], &tail_split[3]);
    // #    endif
    // #    undef EXTRACTOR

    //     // step 3. elevate and write
    //     for (usize i = 0; i < WR_DIV; ++i) {
    //         WRITE_PARTIAL_TAIL(dst, ELEVATOR(base[i]), tail_split[i]);
    //         dst += READ_BATCH_COUNT / WR_DIV;
    //     }

    // #else
    //     // other, simd bit 128. use runtime shift
    //     static_assert(SIMD_BIT_SIZE == 128, "SIMD_BIT_SIZE == 128");

    //     // step 1. load the src and do runtime shift.
    //     // since we can ensure the 32 bytes before src is always readable,
    //     // load directly
    //     static_assert(SIMD_BIT_SIZE < 512);
    //     _FROM_TYPE *load_start = src + tail_count - READ_BATCH_COUNT;
    //     SIMD_128 x = simd_load((const void *)src);
    //     x = runtime_right_shift_128bits(x, (READ_BATCH_COUNT - tail_count) * sizeof(_TARGET_TYPE));

    //     // step 2. extract parts. Note that we reverse the `split_tail_len` result here
    //     // to achieve `split_head_len`
    //     SIMD_SUB_TYPE base[WR_DIV];
    //     Py_ssize_t tail_split[WR_DIV];
    // #    if WR_DIV == 1
    // #        define EXTRACTOR
    //     base[0] = SIMD_VAR;
    //     tail_split[0] = tail_count;
    // #    elif WR_DIV == 2
    // #        define EXTRACTOR PYYJSON_CONCAT3(extract, SIMD_BIT_SIZE, two_parts)
    //     EXTRACTOR(SIMD_VAR, &base[0], &base[1]);
    //     split_tail_len_two_parts(tail_count, READ_BATCH_COUNT, &tail_split[1], &tail_split[0]);
    // #    else
    // #        define EXTRACTOR PYYJSON_CONCAT3(extract, SIMD_BIT_SIZE, four_parts)
    //     EXTRACTOR(SIMD_VAR, &base[0], &base[1], &base[2], &base[3]);
    //     split_tail_len_four_parts(tail_count, READ_BATCH_COUNT, &tail_split[3], &tail_split[2], &tail_split[1], &tail_split[0]);
    // #    endif
    // #    undef EXTRACTOR

    //     // step 3. elevate and write
    //     for (usize i = 0; i < WR_DIV; ++i) {
    //         WRITE_PARTIAL_HEAD(dst, ELEVATOR(base[i]), tail_split[i]);
    //         dst += READ_BATCH_COUNT / WR_DIV;
    //     }
    // #endif
}
#if WR_DIV == 2
force_inline _WVECx4_A_ VECTOR_ELEVATE4(_VECx2_A_ src_vec) {
    register union {
        _WVECx4_A_ v4;
        _WVEC_A_ v1[4];
    } ret;
#    if PYYJSON_X86 && SIMD_BIT_SIZE >= 256
    register union {
        _VECx2_A_ v2;
        _VEC_half_A_ vh[4];
    } src;

    src.v2 = src_vec;
#        define ELEVATOR PYYJSON_CONCAT5(elevate, COMPILE_READ_UCS_LEVEL, COMPILE_WRITE_UCS_LEVEL, to, SIMD_BIT_SIZE)
    ret.v1[0] = ELEVATOR(src.vh[0]);
    ret.v1[1] = ELEVATOR(src.vh[1]);
    ret.v1[2] = ELEVATOR(src.vh[2]);
    ret.v1[3] = ELEVATOR(src.vh[3]);
#        undef ELEVATOR
#    elif PYYJSON_X86

    register union {
        _VECx2_A_ v2;
        _VEC_A_ v1[2];
    } src;

    src.v2 = src_vec;
#        define ELEVATOR PYYJSON_CONCAT5(elevate, COMPILE_READ_UCS_LEVEL, COMPILE_WRITE_UCS_LEVEL, to, 128)
    ret.v1[0] = ELEVATOR(src.v1[0]);
    ret.v1[2] = ELEVATOR(src.v1[1]);
    ret.v1[1] = ELEVATOR(unpack_hi_64_128(src.v1[0], src.v1[0]));
    ret.v1[3] = ELEVATOR(unpack_hi_64_128(src.v1[1], src.v1[1]));
#        undef ELEVATOR
#    endif
    return ret.v4;
}
#elif WR_DIV == 4
force_inline _WVECx4_A_ VECTOR_ELEVATE4(_VEC_A_ src_vec) {
    register union {
        _WVECx4_A_ v4;
        _WVEC_A_ v1[4];
    } ret;

#    if PYYJSON_X86 && SIMD_BIT_SIZE == 512
    register union {
        _VEC_A_ v1;
        _VEC_quad_A_ vq[4];
    } src;

    src.v1 = src_vec;
    ret.v1[0] = elevate_1_4_to_512(src.vq[0]);
    ret.v1[1] = elevate_1_4_to_512(src.vq[1]);
    ret.v1[2] = elevate_1_4_to_512(src.vq[2]);
    ret.v1[3] = elevate_1_4_to_512(src.vq[3]);

#    elif PYYJSON_X86 && SIMD_BIT_SIZE == 256
    register union {
        _VEC_A_ v1;
        _VEC_half_A_ vh[2];
    } src;

    src.v1 = src_vec;
    ret.v1[0] = elevate_1_4_to_256(src.vh[0]);
    ret.v1[2] = elevate_1_4_to_256(src.vh[1]);
    ret.v1[1] = elevate_1_4_to_256(unpack_hi_64_128(src.vh[0], src.vh[0]));
    ret.v1[3] = elevate_1_4_to_256(unpack_hi_64_128(src.vh[1], src.vh[1]));

#    elif PYYJSON_X86
    register _VEC_A_ src = src_vec;
    SIMD_128 src_q[4];
    extract_128_four_parts(src, src_q + 0, src_q + 1, src_q + 2, src_q + 3);
    ret.v1[0] = elevate_1_4_to_128(src_q[0]);
    ret.v1[1] = elevate_1_4_to_128(src_q[1]);
    ret.v1[2] = elevate_1_4_to_128(src_q[2]);
    ret.v1[3] = elevate_1_4_to_128(src_q[3]);
#    else

#    endif
    return ret.v4;
}
#endif

#undef WRITE_PARTIAL_TAIL
#undef WRITE_PARTIAL_HEAD
#undef TAIL_WRITE_SIMD_IMPL
#undef WRITE_SIMD_IMPL
#include "commondef/rw_out.inl.h"
