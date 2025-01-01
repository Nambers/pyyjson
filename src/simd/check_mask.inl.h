// requires: READ

#include "simd_impl.h"

#define CHECK_ESCAPE_IMPL_GET_MASK PYYJSON_CONCAT2(check_escape_impl_get_mask, COMPILE_READ_UCS_LEVEL)
#define GET_DONE_COUNT_FROM_MASK PYYJSON_CONCAT2(get_done_count_from_mask, COMPILE_READ_UCS_LEVEL)
#define CHECK_ESCAPE_TAIL_IMPL_GET_MASK_512 PYYJSON_CONCAT2(check_escape_tail_impl_get_mask_512, COMPILE_READ_UCS_LEVEL)

force_inline SIMD_MASK_TYPE CHECK_ESCAPE_IMPL_GET_MASK(const _FROM_TYPE *src, SIMD_TYPE *restrict SIMD_VAR) {
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
#elif SIMD_BIT_SIZE == 256
#    define SET1 PYYJSON_SIMPLE_CONCAT2(_mm256_set1_epi, READ_BIT_SIZE)
#    define CMPEQ PYYJSON_SIMPLE_CONCAT2(_mm256_cmpeq_epi, READ_BIT_SIZE)
#    define SUBS PYYJSON_SIMPLE_CONCAT2(_mm256_subs_epu, READ_BIT_SIZE)
    *y = load_256((const void *)src);
    __m256i t1 = SET1(_Quote);
    __m256i t2 = SET1(_Slash);
    __m256i t4 = SET1(32);
    __m256i m1 = CMPEQ(*y, t1);
    __m256i m2 = CMPEQ(*y, t2);
#    if COMPILE_READ_UCS_LEVEL != 4
    __m256i m3 = SUBS(t4, *y);
#    else  // COMPILE_READ_UCS_LEVEL == 4
    // there is no `_mm256_subs_epu32`
    __m256i t3 = SET1(_MinusOne);
    __m256i _1 = _mm256_cmpgt_epi32(*y, t3);
    __m256i _2 = _mm256_cmpgt_epi32(t4, *y);
    __m256i m3 = simd_and_256(_1, _2);
#    endif // COMPILE_READ_UCS_LEVEL
    __m256i r = simd_or_256(simd_or_256(m1, m2), m3);
    return r;
#    undef SUBS
#    undef CMPEQ
#    undef SET1
#elif SIMD_BIT_SIZE == 128
#    define SET1 PYYJSON_SIMPLE_CONCAT2(_mm_set1_epi, READ_BIT_SIZE)
#    define CMPEQ PYYJSON_CONCAT3(cmpeq, READ_BIT_SIZE, 128)
#    define SUBS PYYJSON_SIMPLE_CONCAT2(_mm_subs_epu, READ_BIT_SIZE)
    *x = load_128((const void *)src);
    SIMD_128 t1 = SET1(_Quote);
    SIMD_128 t2 = SET1(_Slash);
    SIMD_128 t4 = SET1(ControlMax);
    SIMD_128 m1 = CMPEQ(*x, t1);
    SIMD_128 m2 = CMPEQ(*x, t2);
#    if COMPILE_READ_UCS_LEVEL != 4
    SIMD_128 m3 = SUBS(t4, *x);
#    else  // COMPILE_READ_UCS_LEVEL == 4
    // there is no `_mm_subs_epu32`
    SIMD_128 t3 = SET1(-1);
    SIMD_128 _1 = cmpgt_i32_128(*x, t3);
    SIMD_128 _2 = cmpgt_i32_128(t4, *x);
    SIMD_128 m3 = simd_and_128(_1, _2);
#    endif // COMPILE_READ_UCS_LEVEL
    SIMD_128 r = simd_or_128(simd_or_128(m1, m2), m3);
    return r;
#    undef SUBS
#    undef CMPEQ
#    undef SET1
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
