#ifndef DECODE_STR_COMMON_H
#define DECODE_STR_COMMON_H

#include "decode.h"
#include "pyyjson.h"
#include "simd/simd_impl.h"

force_inline bool _ucs4_checkmax(SIMD_TYPE SIMD_VAR, ReadStrState *read_state, u32 lower_bound_minus_1, int target_max_type) {
#if SIMD_BIT_SIZE == 512
    const SIMD_512 t = _mm512_set1_epi32(lower_bound_minus_1);
    __mmask16 mask = _mm512_cmpgt_epu32_mask(SIMD_VAR, t);
    if (unlikely(mask != 0)) {
        update_max_char_type(read_state, target_max_type);
        return false;
    }
    return true;
#else
#    define CMPGT PYYJSON_CONCAT2(cmpgt_i32, SIMD_BIT_SIZE)
#    define BROADCAST PYYJSON_CONCAT2(broadcast_32, SIMD_BIT_SIZE)
    const SIMD_TYPE t = BROADCAST(lower_bound_minus_1);
    SIMD_TYPE mask = CMPGT(SIMD_VAR, t);
    if (unlikely(!check_mask_zero(mask))) {
        update_max_char_type(read_state, target_max_type);
        return false;
    }
    return true;
#    undef BROADCAST
#    undef CMPGT
#endif
}

force_inline bool _ucs2_checkmax(SIMD_TYPE SIMD_VAR, ReadStrState *read_state, u16 lower_bound_minus_1, int target_max_type) {
#if SIMD_BIT_SIZE == 512
    const SIMD_512 t = _mm512_set1_epi16(lower_bound_minus_1);
    __mmask32 mask = _mm512_cmpgt_epu16_mask(SIMD_VAR, t);
    if (unlikely(mask != 0)) {
        update_max_char_type(read_state, target_max_type);
        return false;
    }
    return true;
#else
#    if SIMD_BIT_SIZE == 256
#        define MM_PREFIX _mm256
#    elif SIMD_BIT_SIZE == 128
#        define MM_PREFIX _mm
#    endif
#    define SUBS PYYJSON_SIMPLE_CONCAT3(MM_PREFIX, _subs_epu, 16)
#    define BROADCAST PYYJSON_CONCAT3(broadcast, 16, SIMD_BIT_SIZE)
    const SIMD_TYPE t = BROADCAST(lower_bound_minus_1);
    SIMD_TYPE mask = SUBS(SIMD_VAR, t);
    if (unlikely(!check_mask_zero(mask))) {
        update_max_char_type(read_state, target_max_type);
        return false;
    }
    return true;
#    undef BROADCAST
#    undef SUBS
#    undef MM_PREFIX
#endif
}

force_inline bool _ucs1_checkmax(SIMD_TYPE SIMD_VAR, ReadStrState *read_state, u8 lower_bound_minus_1, int target_max_type) {
#if SIMD_BIT_SIZE == 512
    const SIMD_512 t = _mm512_set1_epi8(lower_bound_minus_1);
    __mmask64 mask = _mm512_cmpgt_epu8_mask(SIMD_VAR, t);
    if (unlikely(mask != 0)) {
        update_max_char_type(read_state, target_max_type);
        return false;
    }
    return true;
#else
#    if SIMD_BIT_SIZE == 256
#        define MM_PREFIX _mm256
#    elif SIMD_BIT_SIZE == 128
#        define MM_PREFIX _mm
#    endif
#    define SUBS PYYJSON_SIMPLE_CONCAT3(MM_PREFIX, _subs_epu, 8)
#    define BROADCAST PYYJSON_CONCAT3(broadcast, 8, SIMD_BIT_SIZE)
    const SIMD_TYPE t = BROADCAST(lower_bound_minus_1);
    SIMD_TYPE mask = SUBS(SIMD_VAR, t);
    if (unlikely(!check_mask_zero(mask))) {
        update_max_char_type(read_state, target_max_type);
        return false;
    }
    return true;
#    undef BROADCAST
#    undef SUBS
#    undef MM_PREFIX
#endif
}

#endif // DECODE_STR_COMMON_H
