#ifndef PYYJSON_ENCODE_AVX2_H
#define PYYJSON_ENCODE_AVX2_H
#include "encode_shared.hpp"
#include "encode_sse4.inl"
#include <immintrin.h>

/*==============================================================================
 * AVX2
 *============================================================================*/

/*==============================================================================
 * AVX2 Check 256 Bits
 *============================================================================*/

force_inline bool check_escape_u8_256_avx2(__m256i &u) {
    __m256i t1 = _mm256_load_si256((__m256i *) _Quote_i8); // vmovdqa, AVX
    __m256i r1 = _mm256_cmpeq_epi8(u, t1);                 // vpcmpeqb, AVX2
    // if (unlikely(!_mm256_testz_si256(r1, r1))) {           // vptest, AVX
    //     return false;
    // }
    __m256i t2 = _mm256_load_si256((__m256i *) _Slash_i8); // vmovdqa, AVX
    __m256i r2 = _mm256_cmpeq_epi8(u, t2);                 // vpcmpeqb, AVX2
    // if (unlikely(!_mm256_testz_si256(r2, r2))) {           // vptest, AVX
    //     return false;
    // }
    __m256i t3 = _mm256_load_si256((__m256i *) _MinusOne_i8);   // vmovdqa, AVX
    __m256i t4 = _mm256_load_si256((__m256i *) _ControlMax_i8); // vmovdqa, AVX
    __m256i r3 = _mm256_cmpgt_epi8(u, t3);                      // u > -1, vpcmpgtb, AVX2
    __m256i r4 = _mm256_cmpgt_epi8(t4, u);                      // 32 > u, vpcmpgtb, AVX2
    __m256i r5 = _mm256_and_si256(r3, r4);                      // vpand, AVX2
    __m256i r = _mm256_or_si256(_mm256_or_si256(r1, r2), r5);   // vpor, AVX2
    return likely(!_mm256_testz_si256(r, r));                   // vptest, AVX
}


force_inline bool check_escape_u16_256_avx2(__m256i &u) {
    __m256i t1 = _mm256_load_si256((__m256i *) _Quote_i16); // vmovdqa, AVX
    __m256i r1 = _mm256_cmpeq_epi16(u, t1);                 // vpcmpeqw, AVX2
    // if (unlikely(!_mm256_testz_si256(r1, r1))) {            // vptest, AVX
    //     return false;
    // }
    __m256i t2 = _mm256_load_si256((__m256i *) _Slash_i16); // vmovdqa, AVX
    __m256i r2 = _mm256_cmpeq_epi16(u, t2);                 // vpcmpeqw, AVX2
    // if (unlikely(!_mm256_testz_si256(r2, r2))) {            // vptest, AVX
    //     return false;
    // }
    __m256i t3 = _mm256_load_si256((__m256i *) _MinusOne_i16);   // vmovdqa, AVX
    __m256i t4 = _mm256_load_si256((__m256i *) _ControlMax_i16); // vmovdqa, AVX
    __m256i r3 = _mm256_cmpgt_epi16(u, t3);                      // u > -1, vpcmpgtw, AVX2
    __m256i r4 = _mm256_cmpgt_epi16(t4, u);                      // 32 > u, vpcmpgtw, AVX2
    __m256i r5 = _mm256_and_si256(r3, r4);                       // vpand, AVX2
    __m256i r = _mm256_or_si256(_mm256_or_si256(r1, r2), r5);    // vpor, AVX2
    return likely(!_mm256_testz_si256(r, r));                    // vptest, AVX
}


force_inline bool check_escape_u32_256_avx2(__m256i &u) {
    __m256i t1 = _mm256_load_si256((__m256i *) _Quote_i32); // vmovdqa, AVX
    __m256i r1 = _mm256_cmpeq_epi32(u, t1);                 // vpcmpeqd, AVX2
    // if (unlikely(!_mm256_testz_si256(r1, r1))) {            // vptest, AVX
    //     return false;
    // }
    __m256i t2 = _mm256_load_si256((__m256i *) _Slash_i32); // vmovdqa, AVX
    __m256i r2 = _mm256_cmpeq_epi32(u, t2);                 // vpcmpeqd, AVX2
    // if (unlikely(!_mm256_testz_si256(r2, r2))) {            // vptest, AVX
    //     return false;
    // }
    __m256i t3 = _mm256_load_si256((__m256i *) _MinusOne_i32);   // vmovdqa, AVX
    __m256i t4 = _mm256_load_si256((__m256i *) _ControlMax_i32); // vmovdqa, AVX
    __m256i r3 = _mm256_cmpgt_epi32(u, t3);                      // u > -1, vpcmpgtd, AVX2
    __m256i r4 = _mm256_cmpgt_epi32(t4, u);                      // 32 > u, vpcmpgtd, AVX2
    __m256i r5 = _mm256_and_si256(r3, r4);                       // vpand, AVX2
    __m256i r = _mm256_or_si256(_mm256_or_si256(r1, r2), r5);    // vpor, AVX2
    return likely(!_mm256_testz_si256(r, r));                    // vptest, AVX
}

/*==============================================================================
 * AVX2 cvtepu
 *============================================================================*/

template<UCSKind __from, UCSKind __to>
force_inline void cvtepu_128_avx2(__m128i &u, UCSType_t<__from> *&src, UCSType_t<__to> *&dst) { // AVX2
    CVTEPU_COMMON_DEF(128, 256);
    // 128 -> 256
    constexpr size_t _CvtepuLoopCount = _SizeRatio * _SrcReadBits / _DstWriteBits;
    size_t i = _CvtepuLoopCount;
    __m256i s;

    while (i--) {
        if constexpr (__from == UCSKind::UCS2) {
            static_assert(__to == UCSKind::UCS4);
            s = _mm256_cvtepu16_epi32(u); // vpmovzxwd, AVX2
        } else if constexpr (__to == UCSKind::UCS2) {
            static_assert(__from == UCSKind::UCS1);
            s = _mm256_cvtepu8_epi16(u); // vpmovzxbw, AVX2
        } else {
            static_assert(__from == UCSKind::UCS1);
            static_assert(__to == UCSKind::UCS4);
            s = _mm256_cvtepu8_epi32(u); // vpmovzxbd, AVX2
        }
        _mm256_storeu_si256((__m256i_u *) dst, s); // vmovdqu, AVX2
        src += _MoveDst;
        dst += _MoveDst;
        if (i) {
            u = _mm_lddqu_si128((__m128i *) src); // lddqu, sse3
        }
    }
}


#endif // PYYJSON_ENCODE_AVX2_H
