// requires: READ
#include "commondef/r_in.inl.h"
#include "pyyjson.h"
#include "simd_impl.h"

#define GET_DONE_COUNT_FROM_MASK PYYJSON_CONCAT2(get_done_count_from_mask, COMPILE_READ_UCS_LEVEL)
#define CHECK_ESCAPE_TAIL_IMPL_GET_MASK_512 PYYJSON_CONCAT2(check_escape_tail_impl_get_mask_512, COMPILE_READ_UCS_LEVEL)

force_inline VECTOR_MASK_TYPE CHECK_ESCAPE_IMPL_GET_MASK(const _FROM_TYPE *restrict src, VECTOR_TYPE *restrict _out_vec) {
    VECTOR_TYPE v = LOAD_U(src);
    *_out_vec = v;
    VECTOR_TYPE t1, t2, t3;
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
    VECTOR_TYPE m1 = (VECTOR_TYPE)(v == t1);
    VECTOR_TYPE m2 = (VECTOR_TYPE)(v == t2);
    VECTOR_TYPE m3 = (VECTOR_TYPE)(v < t3);
    return (VECTOR_TYPE)(m1 | m2 | m3);
#endif
}

#if PYYJSON_X86
force_inline u32 GET_DONE_COUNT_FROM_MASK(SIMD_MASK_TYPE mask) {
    SIMD_BIT_MASK_TYPE bit_mask;
#    if SIMD_BIT_SIZE == 512
    bit_mask = mask;
    assert(bit_mask);
    u32 done_count = u64_tz_bits(bit_mask); // / sizeof(_FROM_TYPE);
#elif SIMD_BIT_SIZE == 256
    // for bit size < 512, we don't have cmp_epu8, the mask is calculated by subs_epu8
    // so we have to cmpeq with zero to get the real bit mask.
    mask = cmpeq0_8_256(mask);
    bit_mask = to_bitmask_256(mask);
    bit_mask = ~bit_mask;
    assert(bit_mask);
    u32 done_count = u32_tz_bits(bit_mask) / sizeof(_FROM_TYPE);
#    else // SIMD_BIT_SIZE
#        if COMPILE_READ_UCS_LEVEL != 4
    // for bit size < 512, we don't have cmp_epu8,
    // the mask is calculated by subs_epu for ucs < 4
    // so we have to cmpeq with zero to get the real bit mask.
    mask = cmpeq0_8_128(mask);
    bit_mask = to_bitmask_128(mask);
    bit_mask = ~bit_mask;
#        else
    // ucs4 does not have subs_epu, so we don't need cmpeq0.
    // The mask itself is ready for use
    bit_mask = to_bitmask_128(mask);
#        endif // COMPILE_READ_UCS_LEVEL
    assert(bit_mask);
    u32 done_count = u32_tz_bits((u32)bit_mask) / sizeof(_FROM_TYPE);
#    endif
    return done_count;
}
#elif PYYJSON_AARCH
// force_inline u32 GET_DONE_COUNT_FROM_MASK(VECTOR_TYPE mask) {
// }
#endif

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
#include "commondef/r_out.inl.h"
