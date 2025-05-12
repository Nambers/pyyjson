#ifndef PYYJSON_SIMD_SSE2_ENCODE_BYTES_UCS4_H
#define PYYJSON_SIMD_SSE2_ENCODE_BYTES_UCS4_H

#include "simd/simd_detect.h"
#include "simd/vector_types.h"
//
#include "simd/sse2/common.h"

force_inline void ucs4_encode_2bytes_utf8_sse2(vector_a_u32_128 x, u8 *writer) {
    /* abcdefgh|12300000|00000000|00000000 -> gh123[mmm]|abcdef[mm] */
    vector_a_u8_128 m1 = broadcast_u32_128(0xfff83f00);
    vector_a_u16_64 m2 = {0x80c0, 0x80c0, 0x80c0, 0x80c0};
    /* x1 = gh123000|00000000|00000000|00000000 */
    vector_a_u32_128 x1 = rshift_u32_128(x, 6);
    /* x2 = ????????|abcdefgh|12300000|00000000 */
    vector_a_u32_128 x2 = _mm_bslli_si128(x, 1);
    /* x3 = 00000000|abcdef00|00000000|00000000 */
    vector_a_u32_128 x3 = x2 & m1;
    /* x4 = gh123000|abcdef00|00000000|00000000 */
    vector_a_u32_128 x4 = x1 | x3;
    /* u = gh123000|abcdef00 */
    vector_a_u16_64 u = cvt_u32_to_u16_128(x4);
    u = u | m2;
    *(vector_u_u16_64 *)writer = u;
    // memcpy(writer, &u, 8);
}

#endif // PYYJSON_SIMD_SSE2_ENCODE_BYTES_UCS4_H
