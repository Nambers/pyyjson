#ifndef PYYJSON_SIMD_SSSE3_UTF8_H
#define PYYJSON_SIMD_SSSE3_UTF8_H
#if !defined(__SSSE3__) || !__SSSE3__
#    error "SSSE3 is required for this file"
#endif
#include "simd/simd_detect.h"
#include "simd/vector_types.h"
//
#include "simd/sse2/common.h"
#include "simd/ssse3/common.h"

/*
 * Encode UCS2 string to UTF-8, using SSSE3.
 * Need: pshufb, palignr
 */


force_inline void ucs4_encode_3bytes_utf8_ssse3(vector_a_u32_128 x, u8 *writer) {
    static const vector_a_u8_128 t1 = {
            0x80, 0x80, 0,
            0x80, 0x80, 4,
            0x80, 0x80, 8,
            0x80, 0x80, 12,
            0x80, 0x80, 0x80, 0x80};
    static const vector_a_u8_128 t2 = {
            0x80, 0, 0x80,
            0x80, 4, 0x80,
            0x80, 8, 0x80,
            0x80, 12, 0x80,
            0x80, 0x80, 0x80, 0x80};
    static const vector_a_u8_128 t3 = {
            0, 0x80, 0x80,
            4, 0x80, 0x80,
            8, 0x80, 0x80,
            12, 0x80, 0x80,
            0x80, 0x80, 0x80, 0x80};
    static const vector_a_u8_128 m1 = {
            0xff, 0x3f, 0x3f,
            0xff, 0x3f, 0x3f,
            0xff, 0x3f, 0x3f,
            0xff, 0x3f, 0x3f,
            0xff, 0xff, 0xff, 0xff};
    static const vector_a_u8_128 m2 = {
            0xe0, 0x80, 0x80,
            0xe0, 0x80, 0x80,
            0xe0, 0x80, 0x80,
            0xe0, 0x80, 0x80,
            0, 0, 0, 0};
    vector_a_u32_128 x1 = rshift_u32_128(x, 6);
    vector_a_u32_128 x2 = rshift_u32_128(x, 12);
    vector_a_u8_128 x3 = shuffle_128(x, t1);
    vector_a_u8_128 x4 = shuffle_128(x1, t2);
    vector_a_u8_128 x5 = shuffle_128(x2, t3);
    vector_a_u8_128 x6 = ((x3 | x4 | x5) & m1) | m2;
    *(vector_u_u8_128 *)writer = x6;
}

#endif // PYYJSON_SIMD_SSSE3_UTF8_H
