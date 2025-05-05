#ifndef PYYJSON_SIMD_AVX2_CVT_H
#define PYYJSON_SIMD_AVX2_CVT_H

#include "simd/simd_detect.h"
#include "simd/vector_types.h"
//
#include "common.h"
#include "simd/avx/cvt.h"
#include "simd/sse2/common.h"

#if __AVX512F__ && __AVX512CD__
force_inline vector_a_u32_512 cvt_u8_to_u32_512(vector_a_u8_128 x);
force_inline vector_a_u32_512 cvt_u16_to_u32_512(vector_a_u16_256 y);
#endif
#if __AVX512VL__ && __AVX512DQ__ && __AVX512BW__
force_inline vector_a_u16_512 cvt_u8_to_u16_512(vector_a_u8_256 y);
#endif

force_inline vector_a_u16_256 cvt_u8_to_u16_256(vector_a_u8_128 x) {
    return _mm256_cvtepu8_epi16(x);
}

force_inline vector_a_u32_256 cvt_u8_to_u32_256(vector_a_u8_128 x) {
    return _mm256_cvtepu8_epi32(x);
}

force_inline vector_a_u32_256 cvt_u16_to_u32_256(vector_a_u16_128 x) {
    return _mm256_cvtepu16_epi32(x);
}

// cvt up

force_inline void cvt_to_dst_u8_u16_256(u16 *dst, vector_a_u8_256 y) {
#if __AVX512VL__ && __AVX512DQ__ && __AVX512BW__
    *(vector_u_u16_512 *)dst = cvt_u8_to_u16_512(y);
#else
    *(vector_u_u16_256 *)(dst + 0) = cvt_u8_to_u16_256(extract_128_from_256(y, 0));
    *(vector_u_u16_256 *)(dst + 16) = cvt_u8_to_u16_256(extract_128_from_256(y, 1));
#endif
}

force_inline void cvt_to_dst_u8_u32_256(u32 *dst, vector_a_u8_256 y) {
    vector_a_u8_128 x1, x2;
    x1 = extract_128_from_256(y, 0);
    x2 = extract_128_from_256(y, 1);
#if __AVX512F__ && __AVX512CD__
    *(vector_u_u32_512 *)(dst + 0) = cvt_u8_to_u32_512(x1);
    *(vector_u_u32_512 *)(dst + 16) = cvt_u8_to_u32_512(x1);
#else
    *(vector_u_u32_256 *)(dst + 0) = cvt_u8_to_u32_256(x1);
    *(vector_u_u32_256 *)(dst + 8) = cvt_u8_to_u32_256(byte_rshift_128(x1, 8));
    *(vector_u_u32_256 *)(dst + 16) = cvt_u8_to_u32_256(x2);
    *(vector_u_u32_256 *)(dst + 24) = cvt_u8_to_u32_256(byte_rshift_128(x2, 8));
#endif
}

force_inline void cvt_to_dst_u16_u32_256(u32 *dst, vector_a_u16_256 y) {
#if __AVX512F__ && __AVX512CD__
    *(vector_u_u32_512 *)dst = cvt_u16_to_u32_512(y);
#else
    *(vector_u_u32_256 *)(dst + 0) = cvt_u16_to_u32_256(extract_128_from_256(y, 0));
    *(vector_u_u32_256 *)(dst + 8) = cvt_u16_to_u32_256(extract_128_from_256(y, 1));
#endif
}

// cvt down

force_inline void cvt_to_dst_u16_u8_256(u8 *dst, vector_a_u16_256 y) {
    *(vector_u_u8_128 *)dst = cvt_u16_to_u8_256(y);
}

force_inline void cvt_to_dst_u32_u8_256(u8 *dst, vector_a_u32_256 y) {
    *(vector_u_u8_64 *)dst = cvt_u32_to_u8_256(y);
}

force_inline void cvt_to_dst_u32_u16_256(u16 *dst, vector_a_u32_256 y) {
    *(vector_u_u16_128 *)dst = cvt_u32_to_u16_256(y);
}

#endif // PYYJSON_SIMD_AVX2_CVT_H
