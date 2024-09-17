#ifndef PYYJSON_ENCODE_SSE4_H
#define PYYJSON_ENCODE_SSE4_H
#include "encode_shared.hpp"
#include <immintrin.h>


/*==============================================================================
 * SSE4.1
 *============================================================================*/

/*==============================================================================
 * SSE4.1 Check 128 Bits
 *============================================================================*/

force_inline bool check_escape_u8_128_sse4_1(__m128i &u) {
    __m128i t1 = _mm_load_si128((__m128i *) _Quote_i8); // movdqa, sse2
    __m128i r1 = _mm_cmpeq_epi8(u, t1);                 // pcmpeqb, sse2
    // if (unlikely(!_mm_testz_si128(r1, r1))) {           // ptest, sse4.1
    //     return false;
    // }
    __m128i t2 = _mm_load_si128((__m128i *) _Slash_i8); // movdqa, sse2
    __m128i r2 = _mm_cmpeq_epi8(u, t2);                 // pcmpeqb, sse2
    // if (unlikely(!_mm_testz_si128(r2, r2))) {           // ptest, sse4.1
    //     return false;
    // }
    __m128i t3 = _mm_load_si128((__m128i *) _MinusOne_i8);   // movdqa, sse2
    __m128i t4 = _mm_load_si128((__m128i *) _ControlMax_i8); // movdqa, sse2
    __m128i r3 = _mm_cmpgt_epi8(u, t3);                      // u > -1, pcmpgtb, sse2
    __m128i r4 = _mm_cmplt_epi8(u, t4);                      // u < 32, pcmpgtb, sse2
    __m128i r5 = _mm_and_si128(r3, r4);                      // pand, sse2
    __m128i r = _mm_or_si128(_mm_or_si128(r1, r2), r5);      // por, sse2
    return likely(!_mm_testz_si128(r, r));                   // ptest, sse4.1
}

force_inline bool check_escape_u16_128_sse4_1(__m128i &u) {
    __m128i t1 = _mm_load_si128((__m128i *) _Quote_i16); // movdqa, sse2
    __m128i r1 = _mm_cmpeq_epi16(u, t1);                 // pcmpeqw, sse2
    // if (unlikely(!_mm_testz_si128(r1, r1))) {            // ptest, sse4.1
    //     return false;
    // }
    __m128i t2 = _mm_load_si128((__m128i *) _Slash_i16); // movdqa, sse2
    __m128i r2 = _mm_cmpeq_epi16(u, t2);                 // pcmpeqw, sse2
    // if (unlikely(!_mm_testz_si128(r2, r2))) {            // ptest, sse4.1
    //     return false;
    // }
    __m128i t3 = _mm_load_si128((__m128i *) _MinusOne_i16);   // movdqa, sse2
    __m128i t4 = _mm_load_si128((__m128i *) _ControlMax_i16); // movdqa, sse2
    __m128i r3 = _mm_cmpgt_epi16(u, t3);                      // u > -1, pcmpgtw, sse2
    __m128i r4 = _mm_cmplt_epi16(u, t4);                      // u < 32, pcmpgtw, sse2
    __m128i r5 = _mm_and_si128(r3, r4);                       // pand, sse2
    __m128i r = _mm_or_si128(_mm_or_si128(r1, r2), r5);       // por, sse2
    return likely(!_mm_testz_si128(r, r));                    // ptest, sse4.1
}

force_inline bool check_escape_u32_128_sse4_1(__m128i &u) {
    __m128i t1 = _mm_load_si128((__m128i *) _Quote_i32); // movdqa, sse2
    __m128i r1 = _mm_cmpeq_epi32(u, t1);                 // pcmpeqd, sse2
    // if (unlikely(!_mm_testz_si128(r1, r1))) {            // ptest, sse4.1
    //     return false;
    // }
    __m128i t2 = _mm_load_si128((__m128i *) _Slash_i32); // movdqa, sse2
    __m128i r2 = _mm_cmpeq_epi32(u, t2);                 // pcmpeqd, sse2
    // if (unlikely(!_mm_testz_si128(r2, r2))) {            // ptest, sse4.1
    //     return false;
    // }
    __m128i t3 = _mm_load_si128((__m128i *) _MinusOne_i32);   // movdqa, sse2
    __m128i t4 = _mm_load_si128((__m128i *) _ControlMax_i32); // movdqa, sse2
    __m128i r3 = _mm_cmpgt_epi32(u, t3);                      // u > -1, pcmpgtd, sse2
    __m128i r4 = _mm_cmplt_epi32(u, t4);                      // u < 32, pcmpgtd, sse2
    __m128i r5 = _mm_and_si128(r3, r4);                       // pand, sse2
    __m128i r = _mm_or_si128(_mm_or_si128(r1, r2), r5);       // por, sse2
    return likely(!_mm_testz_si128(r, r));                    // ptest, sse4.1
}

/*==============================================================================
 * SSE4.1 cvtepu
 *============================================================================*/

template<UCSKind __from, UCSKind __to>
force_inline void cvtepu_128_sse4_1(__m128i &u, UCSType_t<__from> *&src, UCSType_t<__to> *&dst) { // sse4.1
    CVTEPU_COMMON_DEF(128, 128);
    // 128 -> 128
    constexpr size_t _CvtepuLoopCount = _SizeRatio * _SrcReadBits / _DstWriteBits;
    size_t i = _CvtepuLoopCount;
    __m128i s;
    while (i--) {
        if constexpr (__from == UCSKind::UCS2) {
            static_assert(__to == UCSKind::UCS4);
            s = _mm_cvtepu16_epi32(u); // pmovzxwd, sse4.1
        } else if constexpr (__to == UCSKind::UCS2) {
            static_assert(__from == UCSKind::UCS1);
            s = _mm_cvtepu8_epi16(u); // pmovzxbw, sse4.1
        } else {
            static_assert(__from == UCSKind::UCS1);
            static_assert(__to == UCSKind::UCS4);
            s = _mm_cvtepu8_epi32(u); // pmovzxbd, sse4.1
        }
        _mm_storeu_si128((__m128i_u *) dst, s); // movdqu, sse2
        src += _MoveDst;
        dst += _MoveDst;
        if (i) {
            u = _mm_lddqu_si128((__m128i *) src); // lddqu, sse3
        }
    }
}

#endif
