#ifndef PYYJSON_SIMD_AVX512VLDQBW_CHECKMAX_H
#define PYYJSON_SIMD_AVX512VLDQBW_CHECKMAX_H
#if !defined(__AVX512VL__) || !__AVX512VL__ || !defined(__AVX512DQ__) || !__AVX512DQ__ || !defined(__AVX512BW__) || !__AVX512BW__
#    error "AVX512VL, AVX512DQ and AVX512BW is required for this file"
#endif

#include "common.h"
#include "simd/simd_detect.h"
#include "simd/vector_types.h"

// checkmax_u32_512: AVX512F+CD

force_inline bool checkmax_u16_512(vector_a_u16_512 z, u16 lower_bound_minus_1) {
    const vector_a_u16_512 t = broadcast_u16_512(lower_bound_minus_1);
    if (unlikely(unsigned_cmplt_bitmask_u16_512(t, z))) {
        return false;
    }
    return true;
}

force_inline bool checkmax_u8_512(vector_a_u8_512 z, u8 lower_bound_minus_1) {
    const vector_a_u8_512 t = broadcast_u8_512(lower_bound_minus_1);
    if (unlikely(unsigned_cmplt_bitmask_u8_512(t, z))) {
        return false;
    }
    return true;
}

#endif // PYYJSON_SIMD_AVX512VLDQBW_CHECKMAX_H
