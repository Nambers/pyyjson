// requires: READ
#include "commondef/r_in.inl.h"
#include "pyyjson.h"
#include "simd_impl.h"

#define _CHECK_ESCAPE_IMPL_GET_MASK_INTERNAL PYYJSON_CONCAT2(_check_escape_impl_get_mask_internal, COMPILE_READ_UCS_LEVEL)
#define _GET_DONE_COUNT_FROM_MASK_128 PYYJSON_CONCAT2(_get_done_count_from_mask_128, COMPILE_READ_UCS_LEVEL)
#define _GET_DONE_COUNT_FROM_MASK_256 PYYJSON_CONCAT2(_get_done_count_from_mask_256, COMPILE_READ_UCS_LEVEL)
#define _GET_DONE_COUNT_FROM_MASK_512 PYYJSON_CONCAT2(_get_done_count_from_mask_512, COMPILE_READ_UCS_LEVEL)
#define GET_DONE_COUNT_FROM_MASK PYYJSON_CONCAT2(get_done_count_from_mask, COMPILE_READ_UCS_LEVEL)
#define CHECK_ESCAPE_TAIL_IMPL_GET_MASK_512 PYYJSON_CONCAT2(check_escape_tail_impl_get_mask_512, COMPILE_READ_UCS_LEVEL)

force_inline VECTOR_MASK_TYPE _CHECK_ESCAPE_IMPL_GET_MASK_INTERNAL(_VEC_A_ v) {
    _VEC_A_ t1, t2, t3;
    t1 = SET_ALL(_Quote);
    t2 = SET_ALL(_Slash);
    t3 = SET_ALL(ControlMax);
    //
#if PYYJSON_X86 && SIMD_BIT_SIZE == 512
#    define CMPEQ PYYJSON_SIMPLE_CONCAT3(_mm512_cmpeq_epi, READ_BIT_SIZE, _mask)
#    define CMPLT PYYJSON_SIMPLE_CONCAT3(_mm512_cmplt_epu, READ_BIT_SIZE, _mask)
    VECTOR_MASK_TYPE m1 = CMPEQ(v, t1);
    VECTOR_MASK_TYPE m2 = CMPEQ(v, t2);
    VECTOR_MASK_TYPE m3 = CMPLT(v, t3);
    return (m1 | m2 | m3);
#    undef CMPEQ
#    undef CMPLT
#else
    _VEC_A_ m1 = (_VEC_A_)(v == t1);
    _VEC_A_ m2 = (_VEC_A_)(v == t2);
    _VEC_A_ m3 = (_VEC_A_)(v < t3);
    return (_VEC_A_)(m1 | m2 | m3);
#endif
}

force_inline VECTOR_MASK_TYPE CHECK_ESCAPE_IMPL_GET_MASK(const _FROM_TYPE *restrict src, _VEC_A_ *restrict _out_vec) {
    _VEC_A_ v = LOAD_U(src);
    *_out_vec = v;
    _VEC_A_ t1, t2, t3;
    t1 = SET_ALL(_Quote);
    t2 = SET_ALL(_Slash);
    t3 = SET_ALL(ControlMax);
#if PYYJSON_X86 && SIMD_BIT_SIZE == 512
#    define CMPEQ PYYJSON_SIMPLE_CONCAT3(_mm512_cmpeq_epi, READ_BIT_SIZE, _mask)
#    define CMPLT PYYJSON_SIMPLE_CONCAT3(_mm512_cmplt_epu, READ_BIT_SIZE, _mask)
    SIMD_MASK_TYPE m1 = CMPEQ(v, t1);
    SIMD_MASK_TYPE m2 = CMPEQ(v, t2);
    SIMD_MASK_TYPE m3 = CMPLT(v, t3);
    return (m1 | m2 | m3);
#    undef CMPEQ
#    undef CMPLT
#else
    _VEC_A_ m1 = (_VEC_A_)(v == t1);
    _VEC_A_ m2 = (_VEC_A_)(v == t2);
    _VEC_A_ m3 = (_VEC_A_)(v < t3);
    return (_VEC_A_)(m1 | m2 | m3);
#endif
}

#if PYYJSON_X86
#    if SIMD_BIT_SIZE == 512

force_inline u32 _GET_DONE_COUNT_FROM_MASK_512(u64 mask) {
    u64 bit_mask;
    bit_mask = mask;
    assert(bit_mask);
    u32 done_count = u64_tz_bits(bit_mask); // / sizeof(_FROM_TYPE);
    return done_count;
}
#    endif

#    if SIMD_BIT_SIZE >= 256
force_inline u32 _GET_DONE_COUNT_FROM_MASK_256(__m256i mask) {
    u32 bit_mask;
    // for bit size < 512, we don't have cmp_epu8, the mask is calculated by subs_epu8
    // so we have to cmpeq with zero to get the real bit mask.
    mask = cmpeq0_8_256(mask);
    bit_mask = to_bitmask_256(mask);
    bit_mask = ~bit_mask;
    assert(bit_mask);
    u32 done_count = u32_tz_bits(bit_mask) / sizeof(_FROM_TYPE);
    return done_count;
}
#    endif

force_inline u32 _GET_DONE_COUNT_FROM_MASK_128(__m128i mask) {
    u16 bit_mask;
#    if COMPILE_READ_UCS_LEVEL != 4
    // for bit size < 512, we don't have cmp_epu8,
    // the mask is calculated by subs_epu for ucs < 4
    // so we have to cmpeq with zero to get the real bit mask.
    mask = cmpeq0_8_128(mask);
    bit_mask = to_bitmask_128(mask);
    bit_mask = ~bit_mask;
#    else
    // ucs4 does not have subs_epu, so we don't need cmpeq0.
    // The mask itself is ready for use
    bit_mask = to_bitmask_128(mask);
#    endif // COMPILE_READ_UCS_LEVEL
    assert(bit_mask);
    u32 done_count = u32_tz_bits((u32)bit_mask) / sizeof(_FROM_TYPE);
    return done_count;
}

force_inline u32 GET_DONE_COUNT_FROM_MASK(SIMD_MASK_TYPE mask) {
#    if SIMD_BIT_SIZE == 512
    return _GET_DONE_COUNT_FROM_MASK_512(mask);
#    elif SIMD_BIT_SIZE == 256
    return _GET_DONE_COUNT_FROM_MASK_256(mask);
#    else
    return _GET_DONE_COUNT_FROM_MASK_128(mask);
#    endif
}
#elif PYYJSON_AARCH
// force_inline u32 GET_DONE_COUNT_FROM_MASK(_VEC_A_ mask) {
// }
#endif

force_inline void CHECK_MASK_AND_GET_DONE_COUNT(_VEC_A_ vec, bool *out_checked, usize *out_done_count) {
    VECTOR_MASK_TYPE check_mask = _CHECK_ESCAPE_IMPL_GET_MASK_INTERNAL(vec);
    bool checked = check_mask_zero(check_mask);
    *out_checked = checked;
    if (likely(checked)) {
        return;
    }
    *out_done_count = GET_DONE_COUNT_FROM_MASK(check_mask);
}

force_inline void CHECK_MASK_AND_GET_DONE_COUNTx2(_VECx2_A_ vec2, bool *out_checked, usize *out_done_count) {
    VECTOR_MASK_TYPE check_mask[2];
    VECTOR_MASK_TYPE merged_mask;
    check_mask[0] = _CHECK_ESCAPE_IMPL_GET_MASK_INTERNAL(PYYJSON_CAST(_VEC_A_ *, &vec2)[0]);
    check_mask[1] = _CHECK_ESCAPE_IMPL_GET_MASK_INTERNAL(PYYJSON_CAST(_VEC_A_ *, &vec2)[1]);
    merged_mask = check_mask[0] | check_mask[1];
    bool checked = check_mask_zero(merged_mask);
    *out_checked = checked;
    if (likely(checked)) {
        return;
    }
    if (check_mask_zero(check_mask[0])) {
        *out_done_count = READ_BATCH_COUNT + GET_DONE_COUNT_FROM_MASK(check_mask[1]);
        return;
    }
    *out_done_count = GET_DONE_COUNT_FROM_MASK(check_mask[0]);
    return;
}

force_inline void CHECK_MASK_AND_GET_DONE_COUNTx4(_VECx4_A_ vec4, bool *out_checked, usize *out_done_count) {
    VECTOR_MASK_TYPE check_mask[4];
    VECTOR_MASK_TYPE merged_mask[2];
    check_mask[0] = _CHECK_ESCAPE_IMPL_GET_MASK_INTERNAL(PYYJSON_CAST(_VEC_A_ *, &vec4)[0]);
    check_mask[1] = _CHECK_ESCAPE_IMPL_GET_MASK_INTERNAL(PYYJSON_CAST(_VEC_A_ *, &vec4)[1]);
    check_mask[2] = _CHECK_ESCAPE_IMPL_GET_MASK_INTERNAL(PYYJSON_CAST(_VEC_A_ *, &vec4)[2]);
    check_mask[3] = _CHECK_ESCAPE_IMPL_GET_MASK_INTERNAL(PYYJSON_CAST(_VEC_A_ *, &vec4)[3]);
    merged_mask[0] = check_mask[0] | check_mask[1];
    merged_mask[1] = check_mask[2] | check_mask[3];
    merged_mask[0] = merged_mask[0] | merged_mask[1];
    bool checked = check_mask_zero(merged_mask[0]);
    *out_checked = checked;
    if (likely(checked)) {
        return;
    }
    if (!check_mask_zero(check_mask[0])) {
        *out_done_count = GET_DONE_COUNT_FROM_MASK(check_mask[0]);
        return;
    }
    if (!check_mask_zero(check_mask[1])) {
        *out_done_count = READ_BATCH_COUNT + GET_DONE_COUNT_FROM_MASK(check_mask[1]);
        return;
    }
    if (!check_mask_zero(check_mask[2])) {
        *out_done_count = READ_BATCH_COUNT * 2 + GET_DONE_COUNT_FROM_MASK(check_mask[2]);
        return;
    }
    *out_done_count = READ_BATCH_COUNT * 3 + GET_DONE_COUNT_FROM_MASK(check_mask[3]);
    return;
}

force_inline void CHECK_MASK_AND_GET_DONE_COUNT_128_WITH_MASK(CHECK_MASK_128_SRC_T v, bool *out_checked, usize *out_done_count, CHECK_MASK_128_SRC_MASK_T *optional_mask) {
#define SET_ALL_128 PYYJSON_CONCAT3(broadcast, READ_BIT_SIZE, 128)
    CHECK_MASK_128_SRC_T t1, t2, t3;
    t1 = SET_ALL_128(_Quote);
    t2 = SET_ALL_128(_Slash);
    t3 = SET_ALL_128(ControlMax);

#if PYYJSON_X86 && SIMD_BIT_SIZE == 512
    __mmask16 bit_mask;
#    define CMPEQ PYYJSON_SIMPLE_CONCAT3(_mm_cmpeq_epi, READ_BIT_SIZE, _mask)
#    define CMPLT PYYJSON_SIMPLE_CONCAT3(_mm_cmplt_epu, READ_BIT_SIZE, _mask)
    __mmask16 m1 = (__mmask16)CMPEQ(v, t1);
    __mmask16 m2 = (__mmask16)CMPEQ(v, t2);
    __mmask16 m3 = (__mmask16)CMPLT(v, t3);
    bit_mask = (m1 | m2 | m3);
    if (optional_mask) {
        bit_mask = bit_mask & (*optional_mask);
    }
    *out_checked = !bit_mask;
    if (unlikely(bit_mask)) {
        u32 done_count = u32_tz_bits((u32)bit_mask);
        *out_done_count = (usize)done_count;
    }
#    undef CMPEQ
#    undef CMPLT
#else
    CHECK_MASK_128_SRC_T m1 = (CHECK_MASK_128_SRC_T)(v == t1);
    CHECK_MASK_128_SRC_T m2 = (CHECK_MASK_128_SRC_T)(v == t2);
    CHECK_MASK_128_SRC_T m3 = (CHECK_MASK_128_SRC_T)(v < t3);
    CHECK_MASK_128_SRC_T mask = (CHECK_MASK_128_SRC_T)(m1 | m2 | m3);
    if (optional_mask) {
        mask = mask & (*optional_mask);
    }
    bool checked = testz_128(mask, mask);
    *out_checked = checked;
    if (unlikely(!checked)) {
        u16 bit_mask;
#    if COMPILE_READ_UCS_LEVEL != 4
        // for bit size < 512, we don't have cmp_epu8,
        // the mask is calculated by subs_epu for ucs < 4
        // so we have to cmpeq with zero to get the real bit mask.
        mask = cmpeq0_8_128(mask);
        bit_mask = to_bitmask_128(mask);
        bit_mask = ~bit_mask;
#    else
        // ucs4 does not have subs_epu, so we don't need cmpeq0.
        // The mask itself is ready for use
        bit_mask = to_bitmask_128(mask);
#    endif // COMPILE_READ_UCS_LEVEL
        u32 done_count = u32_tz_bits((u32)bit_mask) / sizeof(_FROM_TYPE);
        *out_done_count = (usize)done_count;
    }
#endif
#undef SET_ALL_128
}

#if PYYJSON_X86 && SIMD_BIT_SIZE == 512
force_inline SIMD_MASK_TYPE CHECK_ESCAPE_TAIL_IMPL_GET_MASK_512(SIMD_512 z, u64 rw_mask) {
#    define CUR_QUOTE PYYJSON_SIMPLE_CONCAT2(_Quote_i, READ_BIT_SIZE)
#    define CUR_SLASH PYYJSON_SIMPLE_CONCAT2(_Slash_i, READ_BIT_SIZE)
#    define CUR_CONTROL_MAX PYYJSON_SIMPLE_CONCAT2(_ControlMax_i, READ_BIT_SIZE)
#    define CMPEQ PYYJSON_SIMPLE_CONCAT3(_mm512_mask_cmpeq_epi, READ_BIT_SIZE, _mask)
#    define CMPLT PYYJSON_SIMPLE_CONCAT3(_mm512_mask_cmplt_epu, READ_BIT_SIZE, _mask)
#    define SET1 PYYJSON_SIMPLE_CONCAT2(_mm512_set1_epi, READ_BIT_SIZE)
    const SIMD_512 t1 = SET1(_Quote);          //load_512_aligned((const void *)CUR_QUOTE);
    const SIMD_512 t2 = SET1(_Slash);          //load_512_aligned((const void *)CUR_SLASH);
    const SIMD_512 t3 = SET1(ControlMax);      //load_512_aligned((const void *)CUR_CONTROL_MAX);
    SIMD_MASK_TYPE m1 = CMPEQ(rw_mask, z, t1); // AVX512BW / AVX512F
    SIMD_MASK_TYPE m2 = CMPEQ(rw_mask, z, t2); // AVX512BW / AVX512F
    SIMD_MASK_TYPE m3 = CMPLT(rw_mask, z, t3); // AVX512BW / AVX512F
    return m1 | m2 | m3;
#    undef SET1
#    undef CMPLT
#    undef CMPEQ
#    undef CUR_CONTROL_MAX
#    undef CUR_SLASH
#    undef CUR_QUOTE
}
#endif // SIMD_BIT_SIZE == 512

#undef CHECK_ESCAPE_TAIL_IMPL_GET_MASK_512
#undef GET_DONE_COUNT_FROM_MASK
#undef _GET_DONE_COUNT_FROM_MASK_512
#undef _GET_DONE_COUNT_FROM_MASK_256
#undef _GET_DONE_COUNT_FROM_MASK_128
#undef _CHECK_ESCAPE_IMPL_GET_MASK_INTERNAL
#include "commondef/r_out.inl.h"
