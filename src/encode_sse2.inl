#ifndef PYYJSON_ENCODE_SSE2_H
#define PYYJSON_ENCODE_SSE2_H
#include "encode_shared.hpp"
#include <immintrin.h>

/*==============================================================================
 * SSE2
 *============================================================================*/

force_inline __m128i _check_escape_u8_128_sse2_impl(__m128i &u) {
    __m128i t1 = _mm_load_si128((__m128i *) _Quote_i8);      // movdqa, sse2
    __m128i r1 = _mm_cmpeq_epi8(u, t1);                      // pcmpeqb, sse2
    __m128i t2 = _mm_load_si128((__m128i *) _Slash_i8);      // movdqa, sse2
    __m128i r2 = _mm_cmpeq_epi8(u, t2);                      // pcmpeqb, sse2
    __m128i t3 = _mm_load_si128((__m128i *) _MinusOne_i8);   // movdqa, sse2
    __m128i t4 = _mm_load_si128((__m128i *) _ControlMax_i8); // movdqa, sse2
    __m128i r3 = _mm_cmpgt_epi8(u, t3);                      // u > -1, pcmpgtb, sse2
    __m128i r4 = _mm_cmplt_epi8(u, t4);                      // u < 32, pcmpgtb, sse2
    __m128i r5 = _mm_and_si128(r3, r4);                      // pand, sse2
    return _mm_or_si128(_mm_or_si128(r1, r2), r5);           // por, sse2
}

force_inline bool check_escape_u8_128_sse2(__m128i &u) {
    __m128i r = _check_escape_u8_128_sse2_impl(u); // sse2
    return 0 == _mm_movemask_epi8(r);              // pmovmskb, sse2
}

force_inline bool check_escape_tail_u8_128_sse2(__m128i &u, size_t count) {
    assert(count > 0 && count < 128 / 8);
    __m128i r = _check_escape_u8_128_sse2_impl(u); // sse2
    int ri = _mm_movemask_epi8(r);                 // pmovmskb, sse2
    return (((1ULL << count) - 1) & ri) == 0;
}

force_inline __m128i _check_escape_u16_128_sse2_impl(__m128i &u) {
    __m128i t1 = _mm_load_si128((__m128i *) _Quote_i16);      // movdqa, sse2
    __m128i r1 = _mm_cmpeq_epi16(u, t1);                      // pcmpeqw, sse2
    __m128i t2 = _mm_load_si128((__m128i *) _Slash_i16);      // movdqa, sse2
    __m128i r2 = _mm_cmpeq_epi16(u, t2);                      // pcmpeqw, sse2
    __m128i t3 = _mm_load_si128((__m128i *) _MinusOne_i16);   // movdqa, sse2
    __m128i t4 = _mm_load_si128((__m128i *) _ControlMax_i16); // movdqa, sse2
    __m128i r3 = _mm_cmpgt_epi16(u, t3);                      // u > -1, pcmpgtw, sse2
    __m128i r4 = _mm_cmplt_epi16(u, t4);                      // u < 32, pcmpgtw, sse2
    __m128i r5 = _mm_and_si128(r3, r4);                       // pand, sse2
    return _mm_or_si128(_mm_or_si128(r1, r2), r5);            // por, sse2
}

force_inline bool check_escape_u16_128_sse2(__m128i &u) {
    __m128i r = _check_escape_u16_128_sse2_impl(u); // sse2
    return 0 == _mm_movemask_epi8(r);               // pmovmskb, sse2
}

force_inline bool check_escape_tail_u16_128_sse2(__m128i &u, size_t count) {
    assert(count > 0 && count < 128 / 16);
    __m128i r = _check_escape_u16_128_sse2_impl(u); // sse2
    int ri = _mm_movemask_epi8(r);                  // pmovmskb, sse2
    return (((1ULL << (count * 2)) - 1) & ri) == 0;
}

force_inline __m128i _check_escape_u32_128_sse2_impl(__m128i &u) {
    __m128i t1 = _mm_load_si128((__m128i *) _Quote_i32);      // movdqa, sse2
    __m128i r1 = _mm_cmpeq_epi32(u, t1);                      // pcmpeqd, sse2
    __m128i t2 = _mm_load_si128((__m128i *) _Slash_i32);      // movdqa, sse2
    __m128i r2 = _mm_cmpeq_epi32(u, t2);                      // pcmpeqd, sse2
    __m128i t3 = _mm_load_si128((__m128i *) _MinusOne_i32);   // movdqa, sse2
    __m128i t4 = _mm_load_si128((__m128i *) _ControlMax_i32); // movdqa, sse2
    __m128i r3 = _mm_cmpgt_epi32(u, t3);                      // u > -1, pcmpgtd, sse2
    __m128i r4 = _mm_cmplt_epi32(u, t4);                      // u < 32, pcmpgtd, sse2
    __m128i r5 = _mm_and_si128(r3, r4);                       // pand, sse2
    return _mm_or_si128(_mm_or_si128(r1, r2), r5);            // por, sse2
}

force_inline bool check_escape_u32_128_sse2(__m128i &u) {
    __m128i r = _check_escape_u32_128_sse2_impl(u); // por, sse2
    return 0 == _mm_movemask_epi8(r);               // pmovmskb, sse2
}

force_inline bool check_escape_tail_u32_128_sse2(__m128i &u, size_t count) {
    assert(count > 0 && count < 128 / 32);
    __m128i r = _check_escape_u32_128_sse2_impl(u); // por, sse2
    int ri = _mm_movemask_epi8(r);                  // pmovmskb, sse2
    return (((1ULL << (count * 4)) - 1) & ri) == 0;
}

/* SSE2 does not support cvtepu. */

#endif // PYYJSON_ENCODE_SSE2_H
