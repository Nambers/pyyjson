#ifndef PYYJSON_SIMD_SSSE3_COMMON_H
#define PYYJSON_SIMD_SSSE3_COMMON_H
#if !defined(__SSSE3__) || !__SSSE3__
#    error "SSSE3 is required for this file"
#endif

#include "simd/simd_detect.h"
#include "simd/vector_types.h"

#define alignr_128(_x1_, _x2_, _imm_) (_mm_alignr_epi8((_x2_), (_x1_), (_imm_)))

force_inline SIMD_128 shuffle_128(SIMD_128 x, SIMD_128 shuffle) {
    return _mm_shuffle_epi8(x, shuffle);
}


#endif // PYYJSON_SIMD_SSSE3_COMMON_H
