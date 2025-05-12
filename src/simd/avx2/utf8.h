#ifndef PYYJSON_SIMD_AVX2_UTF8_H
#define PYYJSON_SIMD_AVX2_UTF8_H

#ifndef __AVX2__
#    error "AVX2 is required for this file"
#endif
#include "simd/simd_detect.h"
#include "simd/vector_types.h"
//
#include "cvt.h"
#include "simd/avx2/common.h"
#include "simd/sse2/common.h"
#include "simd/ssse3/common.h"

force_inline void ucs2_encode_3bytes_utf8_avx2(vector_a_u16_256 y, u8 *writer) {
    /* abcdefgh|12345678|00000000|00000000 -> 5678[mmmm]|gh1234[mm]|abcdef[mm] */
    vector_a_u8_256 t1 = {
            0x80, 0x80, 0x80, 0x80,
            0x80, 0x80, 0,
            0x80, 0x80, 4,
            0x80, 0x80, 8,
            0x80, 0x80, 12,
            //
            0x80, 0x80, 0,
            0x80, 0x80, 4,
            0x80, 0x80, 8,
            0x80, 0x80, 12,
            0x80, 0x80, 0x80, 0x80};
    vector_a_u8_256 t2 = {
            0x80, 0x80, 0x80, 0x80,
            0x80, 0, 0x80,
            0x80, 4, 0x80,
            0x80, 8, 0x80,
            0x80, 12, 0x80,
            //
            0x80, 0, 0x80,
            0x80, 4, 0x80,
            0x80, 8, 0x80,
            0x80, 12, 0x80,
            0x80, 0x80, 0x80, 0x80};
    vector_a_u8_256 t3 = {
            0x80, 0x80, 0x80, 0x80,
            0, 0x80, 0x80,
            4, 0x80, 0x80,
            8, 0x80, 0x80,
            12, 0x80, 0x80,
            //
            0, 0x80, 0x80,
            4, 0x80, 0x80,
            8, 0x80, 0x80,
            12, 0x80, 0x80,
            0x80, 0x80, 0x80, 0x80};
    vector_a_u8_256 m1 = {
            0xff, 0xff, 0xff, 0xff,
            0xff, 0x3f, 0x3f,
            0xff, 0x3f, 0x3f,
            0xff, 0x3f, 0x3f,
            0xff, 0x3f, 0x3f,
            //
            0xff, 0x3f, 0x3f,
            0xff, 0x3f, 0x3f,
            0xff, 0x3f, 0x3f,
            0xff, 0x3f, 0x3f,
            0xff, 0xff, 0xff, 0xff};
    vector_a_u8_256 m2 = {
            0, 0, 0, 0,
            0xe0, 0x80, 0x80,
            0xe0, 0x80, 0x80,
            0xe0, 0x80, 0x80,
            0xe0, 0x80, 0x80,
            //
            0xe0, 0x80, 0x80,
            0xe0, 0x80, 0x80,
            0xe0, 0x80, 0x80,
            0xe0, 0x80, 0x80,
            0, 0, 0, 0};

    vector_a_u32_256 y1, y2;
    y1 = cvt_u16_to_u32_256(extract_128_from_256(y, 0));
    y2 = cvt_u16_to_u32_256(extract_128_from_256(y, 1));
    /* y3,y4 = gh123456|78000000|00000000|00000000 */
    vector_a_u32_256 y3 = rshift_u32_256(y1, 6);
    vector_a_u32_256 y4 = rshift_u32_256(y2, 6);
    /* y5,y6 = 56780000|00000000|00000000|00000000 */
    vector_a_u32_256 y5 = rshift_u32_256(y1, 12);
    vector_a_u32_256 y6 = rshift_u32_256(y2, 12);
    /* y7,y8 = 00000000|00000000|abcdefgh */
    vector_a_u8_256 y7 = shuffle_256(y1, t1);
    vector_a_u8_256 y8 = shuffle_256(y2, t1);
    /* y9,y10 = 00000000|gh123456|00000000 */
    vector_a_u8_256 y9 = shuffle_256(y3, t2);
    vector_a_u8_256 y10 = shuffle_256(y4, t2);
    /* y11,y12 = 56780000|00000000|00000000 */
    vector_a_u8_256 y11 = shuffle_256(y5, t3);
    vector_a_u8_256 y12 = shuffle_256(y6, t3);
    //
    vector_a_u8_256 y13 = ((y7 | y9 | y11) & m1) | m2;
    vector_a_u8_256 y14 = ((y8 | y10 | y12) & m1) | m2;
    //
    vector_a_u8_128 x1 = extract_128_from_256(y13, 0);
    vector_a_u8_128 x2 = extract_128_from_256(y13, 1);
    vector_a_u8_128 x3 = _mm_alignr_epi8(x2, x1, 4);
    vector_a_u8_128 x4 = byte_rshift_128(x2, 4);
    *(vector_u_u8_128 *)(writer + 0) = x3;
    // optimized to vpshufd + vmovq
    memcpy(writer + 16, &x4, 8);
    vector_a_u8_128 x5 = extract_128_from_256(y14, 0);
    vector_a_u8_128 x6 = extract_128_from_256(y14, 1);
    vector_a_u8_128 x7 = _mm_alignr_epi8(x6, x5, 4);
    vector_a_u8_128 x8 = byte_rshift_128(x6, 4);
    *(vector_u_u8_128 *)(writer + 24) = x7;
    // optimized to vpshufd + vmovq
    memcpy(writer + 40, &x8, 8);
}

force_inline void ucs2_encode_2bytes_utf8_avx2(vector_a_u16_256 y, u8 *writer) {
    /* abcdefgh|12300000 -> gh123[mmm]|abcdef[mm] */
    vector_a_u8_256 t1 = {
            0x80, 0,
            0x80, 2,
            0x80, 4,
            0x80, 6,
            0x80, 8,
            0x80, 10,
            0x80, 12,
            0x80, 14,
            //
            0x80, 0,
            0x80, 2,
            0x80, 4,
            0x80, 6,
            0x80, 8,
            0x80, 10,
            0x80, 12,
            0x80, 14};
    /*y1 = gh123000|00000000 */
    vector_a_u8_256 y1 = rshift_u16_256(y, 6);
    /*y2 = 00000000|abcdefgh */
    vector_a_u8_256 y2 = shuffle_256(y, t1);
    /*y = gh123000|abcdefgh */
    y = y1 | y2;
    /*y = gh123000|abcdef00 */
    y = y & broadcast_u16_256(0x3fff);
    /*y = gh123[mmm]|abcdef[mm] */
    y = y | broadcast_u16_256(0x80c0);
    *(vector_u_u16_256 *)writer = y;
}

force_inline void ucs4_encode_3bytes_utf8_avx2(vector_a_u32_256 y, u8 *writer) {
    /* abcdefgh|12345678|00000000|00000000 -> 5678[mmmm]|gh1234[mm]|abcdef[mm] */
    vector_a_u8_256 t1 = {
            0x80, 0x80, 0x80, 0x80,
            0x80, 0x80, 0,
            0x80, 0x80, 4,
            0x80, 0x80, 8,
            0x80, 0x80, 12,
            //
            0x80, 0x80, 0,
            0x80, 0x80, 4,
            0x80, 0x80, 8,
            0x80, 0x80, 12,
            0x80, 0x80, 0x80, 0x80};
    vector_a_u8_256 t2 = {
            0x80, 0x80, 0x80, 0x80,
            0x80, 0, 0x80,
            0x80, 4, 0x80,
            0x80, 8, 0x80,
            0x80, 12, 0x80,
            //
            0x80, 0, 0x80,
            0x80, 4, 0x80,
            0x80, 8, 0x80,
            0x80, 12, 0x80,
            0x80, 0x80, 0x80, 0x80};
    vector_a_u8_256 t3 = {
            0x80, 0x80, 0x80, 0x80,
            0, 0x80, 0x80,
            4, 0x80, 0x80,
            8, 0x80, 0x80,
            12, 0x80, 0x80,
            //
            0, 0x80, 0x80,
            4, 0x80, 0x80,
            8, 0x80, 0x80,
            12, 0x80, 0x80,
            0x80, 0x80, 0x80, 0x80};
    vector_a_u8_256 m1 = {
            0xff, 0xff, 0xff, 0xff,
            0xff, 0x3f, 0x3f,
            0xff, 0x3f, 0x3f,
            0xff, 0x3f, 0x3f,
            0xff, 0x3f, 0x3f,
            //
            0xff, 0x3f, 0x3f,
            0xff, 0x3f, 0x3f,
            0xff, 0x3f, 0x3f,
            0xff, 0x3f, 0x3f,
            0xff, 0xff, 0xff, 0xff};
    vector_a_u8_256 m2 = {
            0, 0, 0, 0,
            0xe0, 0x80, 0x80,
            0xe0, 0x80, 0x80,
            0xe0, 0x80, 0x80,
            0xe0, 0x80, 0x80,
            //
            0xe0, 0x80, 0x80,
            0xe0, 0x80, 0x80,
            0xe0, 0x80, 0x80,
            0xe0, 0x80, 0x80,
            0, 0, 0, 0};
    /* y1 = gh123456|78000000|00000000|00000000 */
    vector_a_u32_256 y1 = rshift_u32_256(y, 6);
    /* y2 = 56780000|00000000|00000000|00000000 */
    vector_a_u32_256 y2 = rshift_u32_256(y, 12);
    /* y3 = 00000000|00000000|abcdefgh */
    vector_a_u8_256 y3 = shuffle_256(y, t1);
    /* y4 = 00000000|gh123456|00000000 */
    vector_a_u8_256 y4 = shuffle_256(y1, t2);
    /* y5 = 56780000|00000000|00000000 */
    vector_a_u8_256 y5 = shuffle_256(y2, t3);
    vector_a_u8_256 y6 = ((y3 | y4 | y5) & m1) | m2;
    //
    vector_a_u8_128 x1 = extract_128_from_256(y6, 0);
    vector_a_u8_128 x2 = extract_128_from_256(y6, 1);
    //
    vector_a_u8_128 x3 = _mm_alignr_epi8(x2, x1, 4);
    vector_a_u8_128 x4 = byte_rshift_128(x2, 4);
    *(vector_u_u8_128 *)(writer + 0) = x3;
    // optimized to vpshufd + vmovq
    memcpy(writer + 16, &x4, 8);
}

force_inline void ucs4_encode_2bytes_utf8_avx2(vector_a_u32_256 y, u8 *writer) {
    /* abcdefgh|12300000|00000000|00000000 -> gh123[mmm]|abcdef[mm] */
    /* x = abcdefgh|12300000 */
    vector_a_u16_128 x = cvt_u32_to_u16_256(y);
    vector_a_u8_128 t1 = {
            0x80, 0,
            0x80, 2,
            0x80, 4,
            0x80, 6,
            0x80, 8,
            0x80, 10,
            0x80, 12,
            0x80, 14};
    vector_a_u8_128 m1 = broadcast_u16_128(0x3fff);
    vector_a_u8_128 m2 = broadcast_u16_128(0x80c0);
    /*x1 = gh123000|00000000 */
    vector_a_u8_128 x1 = rshift_u16_128(x, 6);
    /*x2 = 00000000|abcdefgh */
    vector_a_u8_128 x2 = shuffle_128(x, t1);
    /*x3 = gh123000|abcdefgh */
    vector_a_u8_128 x3 = ((x1 | x2) & m1) | m2;
    *(vector_u_u8_128 *)writer = x3;
}

#endif
