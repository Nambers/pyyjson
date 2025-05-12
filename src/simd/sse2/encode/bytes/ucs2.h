#ifndef PYYJSON_SIMD_SSE2_ENCODE_BYTES_UCS2_H
#define PYYJSON_SIMD_SSE2_ENCODE_BYTES_UCS2_H

#include "simd/simd_detect.h"
#include "simd/vector_types.h"
//
#include "simd/sse2/common.h"

force_inline void ucs2_encode_2bytes_utf8_sse2(vector_a_u16_128 x, u8 *writer) {
    /* abcdefgh|12300000 -> gh123[mmm]|abcdef[mm] */
    /* x1 = gh123000|00000000 */
    vector_a_u16_128 x1 = rshift_u16_128(x, 6);
    /* x2 = ????????|abcdefgh */
    vector_a_u16_128 x2 = _mm_bslli_si128(x, 1);
    /* x2 = 00000000|abcdef00 */
    x2 = x2 & broadcast_u16_128(0x3f00);
    /* y = gh123000|abcdef00 */
    x = x1 | x2;
    /* y = gh123[mmm]|abcdef[mm] */
    x = x | broadcast_u16_128(0x80c0);
    *(vector_u_u16_128 *)writer = x;
}

#endif // PYYJSON_SIMD_SSE2_ENCODE_BYTES_UCS2_H
