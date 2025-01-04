// requires: READ

#include "simd_impl.h"

#define CHECK_ESCAPE_IMPL_GET_MASK PYYJSON_CONCAT2(check_escape_impl_get_mask, COMPILE_READ_UCS_LEVEL)
#define GET_DONE_COUNT_FROM_MASK PYYJSON_CONCAT2(get_done_count_from_mask, COMPILE_READ_UCS_LEVEL)
#define CHECK_ESCAPE_TAIL_IMPL_GET_MASK_512 PYYJSON_CONCAT2(check_escape_tail_impl_get_mask_512, COMPILE_READ_UCS_LEVEL)

force_inline SIMD_MASK_TYPE CHECK_ESCAPE_IMPL_GET_MASK(const _FROM_TYPE *restrict src, SIMD_TYPE *restrict SIMD_VAR) {
#if SIMD_BIT_SIZE == 512
#    define CUR_QUOTE PYYJSON_SIMPLE_CONCAT2(_Quote_i, READ_BIT_SIZE)
#    define CUR_SLASH PYYJSON_SIMPLE_CONCAT2(_Slash_i, READ_BIT_SIZE)
#    define CUR_CONTROL_MAX PYYJSON_SIMPLE_CONCAT2(_ControlMax_i, READ_BIT_SIZE)
#    define CMPEQ PYYJSON_SIMPLE_CONCAT3(_mm512_cmpeq_epi, READ_BIT_SIZE, _mask)
#    define CMPLT PYYJSON_SIMPLE_CONCAT3(_mm512_cmplt_epu, READ_BIT_SIZE, _mask)
    *z = load_512((const void *)src);
    SIMD_512 t1 = load_512_aligned((const void *)CUR_QUOTE);
    SIMD_512 t2 = load_512_aligned((const void *)CUR_SLASH);
    SIMD_512 t3 = load_512_aligned((const void *)CUR_CONTROL_MAX);
    SIMD_MASK_TYPE m1 = CMPEQ(*z, t1); // AVX512BW, AVX512F
    SIMD_MASK_TYPE m2 = CMPEQ(*z, t2); // AVX512BW, AVX512F
    SIMD_MASK_TYPE m3 = CMPLT(*z, t3); // AVX512BW, AVX512F
    return m1 | m2 | m3;
#    undef CMPLT
#    undef CMPEQ
#    undef CUR_CONTROL_MAX
#    undef CUR_SLASH
#    undef CUR_QUOTE
#else
#    if SIMD_BIT_SIZE == 256
#        define MM_PREFIX _mm256
#    elif SIMD_BIT_SIZE == 128
#        define MM_PREFIX _mm
#    endif
#    define SET1 PYYJSON_SIMPLE_CONCAT3(MM_PREFIX, _set1_epi, READ_BIT_SIZE)
#    define CMPEQ PYYJSON_SIMPLE_CONCAT3(MM_PREFIX, _cmpeq_epi, READ_BIT_SIZE)
#    define SUBS PYYJSON_SIMPLE_CONCAT3(MM_PREFIX, _subs_epu, READ_BIT_SIZE)
#    define CMPGT PYYJSON_CONCAT2(cmpgt_i32, SIMD_BIT_SIZE)
#    define AND PYYJSON_CONCAT2(simd_and, SIMD_BIT_SIZE)
#    define OR PYYJSON_CONCAT2(simd_or, SIMD_BIT_SIZE)
    *SIMD_VAR = load_simd((const void *)src);
    SIMD_TYPE t1 = SET1(_Quote);
    SIMD_TYPE t2 = SET1(_Slash);
    SIMD_TYPE t4 = SET1(ControlMax);
    SIMD_TYPE m1 = CMPEQ(*SIMD_VAR, t1);
    SIMD_TYPE m2 = CMPEQ(*SIMD_VAR, t2);
#    if COMPILE_READ_UCS_LEVEL != 4
    SIMD_TYPE m3 = SUBS(t4, *SIMD_VAR);
#    else  // COMPILE_READ_UCS_LEVEL == 4
    // there is no `MM_PREFIX_subs_epu32`
    SIMD_TYPE t3 = SET1(_MinusOne);
    SIMD_TYPE _1 = CMPGT(*SIMD_VAR, t3);
    SIMD_TYPE _2 = CMPGT(t4, *SIMD_VAR);
    SIMD_TYPE m3 = AND(_1, _2);
#    endif // COMPILE_READ_UCS_LEVEL
    SIMD_TYPE r = OR(OR(m1, m2), m3);
    return r;
#    undef OR
#    undef AND
#    undef CMPGT
#    undef SUBS
#    undef CMPEQ
#    undef SET1
#    undef MM_PREFIX
#endif // SIMD_BIT_SIZE
}

force_inline u32 GET_DONE_COUNT_FROM_MASK(SIMD_MASK_TYPE mask) {
    SIMD_BIT_MASK_TYPE bit_mask;
#if SIMD_BIT_SIZE == 512
    bit_mask = mask;
    assert(bit_mask);
    u32 done_count = u64_tz_bits(bit_mask) / sizeof(_FROM_TYPE);
#elif SIMD_BIT_SIZE == 256
    // for bit size < 512, we don't have cmp_epu8, the mask is calculated by subs_epu8
    // so we have to cmpeq with zero to get the real bit mask.
    mask = cmpeq0_8_256(mask);
    bit_mask = to_bitmask_256(mask);
    bit_mask = ~bit_mask;
    assert(bit_mask);
    u32 done_count = u32_tz_bits(bit_mask) / sizeof(_FROM_TYPE);
#else // SIMD_BIT_SIZE
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
#endif
    return done_count;
}

#if SIMD_BIT_SIZE == 512
force_inline SIMD_MASK_TYPE CHECK_ESCAPE_TAIL_IMPL_GET_MASK_512(SIMD_512 z, SIMD_MASK_TYPE rw_mask) {
#    define CUR_QUOTE PYYJSON_SIMPLE_CONCAT2(_Quote_i, READ_BIT_SIZE)
#    define CUR_SLASH PYYJSON_SIMPLE_CONCAT2(_Slash_i, READ_BIT_SIZE)
#    define CUR_CONTROL_MAX PYYJSON_SIMPLE_CONCAT2(_ControlMax_i, READ_BIT_SIZE)
#    define CMPEQ PYYJSON_SIMPLE_CONCAT3(_mm512_mask_cmpeq_epi, READ_BIT_SIZE, _mask)
#    define CMPLT PYYJSON_SIMPLE_CONCAT3(_mm512_mask_cmplt_epu, READ_BIT_SIZE, _mask)
    SIMD_512 t1 = load_512_aligned((const void *)CUR_QUOTE);
    SIMD_512 t2 = load_512_aligned((const void *)CUR_SLASH);
    SIMD_512 t3 = load_512_aligned((const void *)CUR_CONTROL_MAX);
    SIMD_MASK_TYPE m1 = CMPEQ(rw_mask, z, t1); // AVX512BW / AVX512F
    SIMD_MASK_TYPE m2 = CMPEQ(rw_mask, z, t2); // AVX512BW / AVX512F
    SIMD_MASK_TYPE m3 = CMPLT(rw_mask, z, t3); // AVX512BW / AVX512F
    return m1 | m2 | m3;
#    undef CMPLT
#    undef CMPEQ
#    undef CUR_CONTROL_MAX
#    undef CUR_SLASH
#    undef CUR_QUOTE
}
#endif // SIMD_BIT_SIZE == 512

#undef CHECK_ESCAPE_TAIL_IMPL_GET_MASK_512
#undef GET_DONE_COUNT_FROM_MASK
#undef CHECK_ESCAPE_IMPL_GET_MASK
