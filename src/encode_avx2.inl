#ifndef PYYJSON_ENCODE_AVX2_H
#define PYYJSON_ENCODE_AVX2_H
#include "encode_shared.hpp"
#include "encode_sse4.inl"
#include <immintrin.h>

/*==============================================================================
 * AVX2
 *============================================================================*/

force_inline Py_ssize_t reduce_add_epi32_256(__m256i y) {
    __m256i sum1 = _mm256_hadd_epi32(y, y);           // vphaddd, AVX2
    __m256i sum2 = _mm256_hadd_epi32(sum1, sum1);     // vphaddd, AVX2
    __m128i low = _mm256_extracti128_si256(sum2, 0);  // vextracti128, AVX2
    __m128i high = _mm256_extracti128_si256(sum2, 1); // vextracti128, AVX2
    __m128i total = _mm_add_epi32(low, high);         // paddd, SSE2
    return _mm_cvtsi128_si32(total);                  // movd, SSE2
}

/*==============================================================================
 * AVX2 Check 256 Bits
 *============================================================================*/

force_inline void _check_escape_u8_256_avx2_get_mask(__m256i &u, __m256i &m1, __m256i &m2, __m256i &m3) {
    __m256i t1 = _mm256_load_si256((__m256i *) _Quote_i8);      // vmovdqa, AVX
    __m256i t2 = _mm256_load_si256((__m256i *) _Slash_i8);      // vmovdqa, AVX
    __m256i t3 = _mm256_load_si256((__m256i *) _MinusOne_i8);   // vmovdqa, AVX
    __m256i t4 = _mm256_load_si256((__m256i *) _ControlMax_i8); // vmovdqa, AVX
    m1 = _mm256_cmpeq_epi8(u, t1);                              // vpcmpeqb, AVX2
    m2 = _mm256_cmpeq_epi8(u, t2);                              // vpcmpeqb, AVX2
    __m256i r1 = _mm256_cmpgt_epi8(u, t3);                      // u > -1, vpcmpgtb, AVX2
    __m256i r2 = _mm256_cmpgt_epi8(t4, u);                      // 32 > u, vpcmpgtb, AVX2
    m3 = _mm256_and_si256(r1, r2);                              // vpand, AVX2
}

force_inline __m256i _check_escape_u8_256_avx2_impl(__m256i &u) {
    __m256i m1, m2, m3;
    _check_escape_u8_256_avx2_get_mask(u, m1, m2, m3);   // AVX2
    return _mm256_or_si256(_mm256_or_si256(m1, m2), m3); // vpor, AVX2
}

force_inline bool check_escape_u8_256_avx2(__m256i &u) {
    __m256i r = _check_escape_u8_256_avx2_impl(u); // AVX2
    return _mm256_testz_si256(r, r);               // vptest, AVX
}

force_inline bool check_escape_tail_u8_256_avx2(__m256i &u, size_t count) {
    assert(count && count < 256 / 8);
    __m256i r = _check_escape_u8_256_avx2_impl(u);                        // AVX2
    __m256i m = _mm256_load_si256((__m256i *) &_MaskArray<u8>[count][0]); // vmovdqa, AVX
    __m256i t = _mm256_and_si256(r, m);                                   // vpand, AVX2
    return _mm256_testz_si256(t, t);                                      // vptest, AVX
}

force_inline __m256i _check_escape_u16_256_avx2_get_mask(__m256i &u, __m256i &m1, __m256i &m2, __m256i &m3) {
    __m256i t1 = _mm256_load_si256((__m256i *) _Quote_i16);      // vmovdqa, AVX
    __m256i t2 = _mm256_load_si256((__m256i *) _Slash_i16);      // vmovdqa, AVX
    __m256i t3 = _mm256_load_si256((__m256i *) _MinusOne_i16);   // vmovdqa, AVX
    __m256i t4 = _mm256_load_si256((__m256i *) _ControlMax_i16); // vmovdqa, AVX
    m1 = _mm256_cmpeq_epi16(u, t1);                              // vpcmpeqw, AVX2
    m2 = _mm256_cmpeq_epi16(u, t2);                              // vpcmpeqw, AVX2
    __m256i r1 = _mm256_cmpgt_epi16(u, t3);                      // u > -1, vpcmpgtw, AVX2
    __m256i r2 = _mm256_cmpgt_epi16(t4, u);                      // 32 > u, vpcmpgtw, AVX2
    m3 = _mm256_and_si256(r1, r2);                               // vpand, AVX2
}

force_inline __m256i _check_escape_u16_256_avx2_impl(__m256i &u) {
    __m256i m1, m2, m3;
    _check_escape_u16_256_avx2_get_mask(u, m1, m2, m3);  // AVX2
    return _mm256_or_si256(_mm256_or_si256(m1, m2), m3); // vpor, AVX2
}

force_inline bool check_escape_u16_256_avx2(__m256i &u) {
    __m256i r = _check_escape_u16_256_avx2_impl(u); // AVX2
    return _mm256_testz_si256(r, r);                // vptest, AVX
}

force_inline bool check_escape_tail_u16_256_avx2(__m256i &u, size_t count) {
    assert(count && count < 256 / 16);
    __m256i r = _check_escape_u16_256_avx2_impl(u);                        // AVX2
    __m256i m = _mm256_load_si256((__m256i *) &_MaskArray<u16>[count][0]); // vmovdqa, AVX
    __m256i t = _mm256_and_si256(r, m);                                    // vpand, AVX2
    return _mm256_testz_si256(t, t);                                       // vptest, AVX
}

force_inline __m256i _check_escape_u32_256_avx2_get_mask(__m256i &u, __m256i &m1, __m256i &m2, __m256i &m3) {
    __m256i t1 = _mm256_load_si256((__m256i *) _Quote_i16);      // vmovdqa, AVX
    __m256i t2 = _mm256_load_si256((__m256i *) _Slash_i16);      // vmovdqa, AVX
    __m256i t3 = _mm256_load_si256((__m256i *) _MinusOne_i16);   // vmovdqa, AVX
    __m256i t4 = _mm256_load_si256((__m256i *) _ControlMax_i16); // vmovdqa, AVX
    m1 = _mm256_cmpeq_epi32(u, t1);                              // vpcmpeqd, AVX2
    m2 = _mm256_cmpeq_epi32(u, t2);                              // vpcmpeqd, AVX2
    __m256i r1 = _mm256_cmpgt_epi32(u, t3);                      // u > -1, vpcmpgtd, AVX2
    __m256i r2 = _mm256_cmpgt_epi32(t4, u);                      // 32 > u, vpcmpgtd, AVX2
    m3 = _mm256_and_si256(r1, r2);                               // vpand, AVX2
}

force_inline __m256i _check_escape_u32_256_avx2_impl(__m256i &u) {
    __m256i m1, m2, m3;
    _check_escape_u32_256_avx2_get_mask(u, m1, m2, m3);  // AVX2
    return _mm256_or_si256(_mm256_or_si256(m1, m2), m3); // vpor, AVX2
}

force_inline bool check_escape_u32_256_avx2(__m256i &u) {
    __m256i r = _check_escape_u32_256_avx2_impl(u); // vpor, AVX2
    return _mm256_testz_si256(r, r);                // vptest, AVX
}

force_inline bool check_escape_tail_u32_256_avx2(__m256i &u, size_t count) {
    assert(count && count < 256 / 32);
    __m256i r = _check_escape_u32_256_avx2_impl(u);                        // AVX2
    __m256i m = _mm256_load_si256((__m256i *) &_MaskArray<u32>[count][0]); // vmovdqa, AVX
    __m256i t = _mm256_and_si256(r, m);                                    // vpand, AVX2
    return _mm256_testz_si256(t, t);                                       // vptest, AVX
}

/*==============================================================================
 * AVX512 Count
 *============================================================================*/

force_inline Py_ssize_t count_escape_u8_256_avx2(__m256i &u) {
    constexpr Py_ssize_t _BaseRet = 256 / 8;
    Py_ssize_t ret = _BaseRet;
    __m256i m1, m2, m3;
    _check_escape_u8_256_avx2_get_mask(u, m1, m2, m3); // AVX2
    __m256i m1or2 = _mm256_or_si256(m1, m2);           // vpor, AVX2
    {
        __m256i tmp = _mm256_or_si256(m1or2, m3);             // vpor, AVX2
        if (likely(_mm256_testz_si256(tmp, tmp))) return ret; // vptest, AVX
    }
    // expand to i32, 64 -> 256 (4 times)
    __m256i zero_256 = _mm256_setzero_si256(); // vpxor, AVX
    {
        i64 data = _mm256_extract_epi64(u, 0);                                                        // Sequence, AVX
        i64 mask64 = _mm256_extract_epi64(m3, 0);                                                     // Sequence, AVX
        __m128i x_data = _mm_cvtsi64_si128(data);                                                     // movq, SSE2
        __m256i vidx = _mm256_cvtepu8_epi32(x_data);                                                  // vpmovzxbd, AVX2
        __m128i x_mask = _mm_cvtsi64_si128(mask64);                                                   // movq, SSE2
        __m256i mask = _mm256_cvtepi8_epi32(x_mask);                                                  // vpmovsxbd, AVX2
        __m256i to_sum = _mm256_mask_i32gather_epi32(zero_256, &_ControlLengthAdd[0], vidx, mask, 4); // vpgatherdd, AVX2
        ret += reduce_add_epi32_256(to_sum);                                                          // AVX2
    }
    {
        i64 data = _mm256_extract_epi64(u, 1);                                                        // Sequence, AVX
        i64 mask64 = _mm256_extract_epi64(m3, 1);                                                     // Sequence, AVX
        __m128i x_data = _mm_cvtsi64_si128(data);                                                     // movq, SSE2
        __m256i vidx = _mm256_cvtepu8_epi32(x_data);                                                  // vpmovzxbd, AVX2
        __m128i x_mask = _mm_cvtsi64_si128(mask64);                                                   // movq, SSE2
        __m256i mask = _mm256_cvtepi8_epi32(x_mask);                                                  // vpmovsxbd, AVX2
        __m256i to_sum = _mm256_mask_i32gather_epi32(zero_256, &_ControlLengthAdd[0], vidx, mask, 4); // vpgatherdd, AVX2
        ret += reduce_add_epi32_256(to_sum);                                                          // AVX2
    }
    {
        i64 data = _mm256_extract_epi64(u, 2);                                                        // Sequence, AVX
        i64 mask64 = _mm256_extract_epi64(m3, 2);                                                     // Sequence, AVX
        __m128i x_data = _mm_cvtsi64_si128(data);                                                     // movq, SSE2
        __m256i vidx = _mm256_cvtepu8_epi32(x_data);                                                  // vpmovzxbd, AVX2
        __m128i x_mask = _mm_cvtsi64_si128(mask64);                                                   // movq, SSE2
        __m256i mask = _mm256_cvtepi8_epi32(x_mask);                                                  // vpmovsxbd, AVX2
        __m256i to_sum = _mm256_mask_i32gather_epi32(zero_256, &_ControlLengthAdd[0], vidx, mask, 4); // vpgatherdd, AVX2
        ret += reduce_add_epi32_256(to_sum);                                                          // AVX2
    }
    {
        i64 data = _mm256_extract_epi64(u, 3);                                                        // Sequence, AVX
        i64 mask64 = _mm256_extract_epi64(m3, 3);                                                     // Sequence, AVX
        __m128i x_data = _mm_cvtsi64_si128(data);                                                     // movq, SSE2
        __m256i vidx = _mm256_cvtepu8_epi32(x_data);                                                  // vpmovzxbd, AVX2
        __m128i x_mask = _mm_cvtsi64_si128(mask64);                                                   // movq, SSE2
        __m256i mask = _mm256_cvtepi8_epi32(x_mask);                                                  // vpmovsxbd, AVX2
        __m256i to_sum = _mm256_mask_i32gather_epi32(zero_256, &_ControlLengthAdd[0], vidx, mask, 4); // vpgatherdd, AVX2
        ret += reduce_add_epi32_256(to_sum);                                                          // AVX2
    }
    i32 mask32 = _mm256_movemask_epi8(m1or2); // vpmovmskb, AVX2
    return ret + _mm_popcnt_u32(mask32);      // popcnt, implies SSE4.2
}

force_inline Py_ssize_t count_escape_tail_u8_256_avx2(__m256i &u, usize count) {
    constexpr Py_ssize_t _BaseRet = 256 / 8;
    assert(count && count < _BaseRet);
    Py_ssize_t ret = _BaseRet;
    __m256i m1, m2, m3;
    _check_escape_u8_256_avx2_get_mask(u, m1, m2, m3);                            // AVX2
    __m256i m1or2 = _mm256_or_si256(m1, m2);                                      // vpor, AVX2
    __m256i tail_mask = _mm256_load_si256((__m256i *) &_MaskArray<u8>[count][0]); // vmovdqa, AVX
    {
        __m256i tmp = _mm256_and_si256(_mm256_or_si256(m1or2, m3), tail_mask); // vpand, vpor, AVX2
        if (likely(_mm256_testz_si256(tmp, tmp))) return ret;                  // vptest, AVX
    }
    // expand to i32, 64 -> 256 (4 times)
    m3 = _mm256_and_si256(m3, tail_mask);      // vpand, AVX2
    __m256i zero_256 = _mm256_setzero_si256(); // vpxor, AVX
    {
        i64 data = _mm256_extract_epi64(u, 0);                                                        // Sequence, AVX
        i64 mask64 = _mm256_extract_epi64(m3, 0);                                                     // Sequence, AVX
        __m128i x_data = _mm_cvtsi64_si128(data);                                                     // movq, SSE2
        __m256i vidx = _mm256_cvtepu8_epi32(x_data);                                                  // vpmovzxbd, AVX2
        __m128i x_mask = _mm_cvtsi64_si128(mask64);                                                   // movq, SSE2
        __m256i mask = _mm256_cvtepi8_epi32(x_mask);                                                  // vpmovsxbd, AVX2
        __m256i to_sum = _mm256_mask_i32gather_epi32(zero_256, &_ControlLengthAdd[0], vidx, mask, 4); // vpgatherdd, AVX2
        ret += reduce_add_epi32_256(to_sum);                                                          // AVX2
    }
    {
        i64 data = _mm256_extract_epi64(u, 1);                                                        // Sequence, AVX
        i64 mask64 = _mm256_extract_epi64(m3, 1);                                                     // Sequence, AVX
        __m128i x_data = _mm_cvtsi64_si128(data);                                                     // movq, SSE2
        __m256i vidx = _mm256_cvtepu8_epi32(x_data);                                                  // vpmovzxbd, AVX2
        __m128i x_mask = _mm_cvtsi64_si128(mask64);                                                   // movq, SSE2
        __m256i mask = _mm256_cvtepi8_epi32(x_mask);                                                  // vpmovsxbd, AVX2
        __m256i to_sum = _mm256_mask_i32gather_epi32(zero_256, &_ControlLengthAdd[0], vidx, mask, 4); // vpgatherdd, AVX2
        ret += reduce_add_epi32_256(to_sum);                                                          // AVX2
    }
    {
        i64 data = _mm256_extract_epi64(u, 2);                                                        // Sequence, AVX
        i64 mask64 = _mm256_extract_epi64(m3, 2);                                                     // Sequence, AVX
        __m128i x_data = _mm_cvtsi64_si128(data);                                                     // movq, SSE2
        __m256i vidx = _mm256_cvtepu8_epi32(x_data);                                                  // vpmovzxbd, AVX2
        __m128i x_mask = _mm_cvtsi64_si128(mask64);                                                   // movq, SSE2
        __m256i mask = _mm256_cvtepi8_epi32(x_mask);                                                  // vpmovsxbd, AVX2
        __m256i to_sum = _mm256_mask_i32gather_epi32(zero_256, &_ControlLengthAdd[0], vidx, mask, 4); // vpgatherdd, AVX2
        ret += reduce_add_epi32_256(to_sum);                                                          // AVX2
    }
    {
        i64 data = _mm256_extract_epi64(u, 3);                                                        // Sequence, AVX
        i64 mask64 = _mm256_extract_epi64(m3, 3);                                                     // Sequence, AVX
        __m128i x_data = _mm_cvtsi64_si128(data);                                                     // movq, SSE2
        __m256i vidx = _mm256_cvtepu8_epi32(x_data);                                                  // vpmovzxbd, AVX2
        __m128i x_mask = _mm_cvtsi64_si128(mask64);                                                   // movq, SSE2
        __m256i mask = _mm256_cvtepi8_epi32(x_mask);                                                  // vpmovsxbd, AVX2
        __m256i to_sum = _mm256_mask_i32gather_epi32(zero_256, &_ControlLengthAdd[0], vidx, mask, 4); // vpgatherdd, AVX2
        ret += reduce_add_epi32_256(to_sum);                                                          // AVX2
    }
    i32 mask32 = _mm256_movemask_epi8(_mm256_and_si256(m1or2, tail_mask)); // vpmovmskb, vpand, AVX2
    return ret + _mm_popcnt_u32(mask32);                                   // popcnt, implies SSE4.2
}

force_inline Py_ssize_t count_escape_u16_256_avx2(__m256i &u) {
    constexpr Py_ssize_t _BaseRet = 256 / 16;
    Py_ssize_t ret = _BaseRet;
    __m256i m1, m2, m3;
    _check_escape_u16_256_avx2_get_mask(u, m1, m2, m3); // AVX2
    __m256i m1or2 = _mm256_or_si256(m1, m2);            // vpor, AVX2
    {
        __m256i tmp = _mm256_or_si256(m1or2, m3);             // vpor, AVX2
        if (likely(_mm256_testz_si256(tmp, tmp))) return ret; // vptest, AVX
    }
    // expand to i32, 128 -> 256 (2 times)
    __m256i zero_256 = _mm256_setzero_si256(); // vpxor, AVX
    {
        __m128i data = _mm256_extracti128_si256(u, 0);                                                // vextracti128, AVX2
        __m128i mask16 = _mm256_extracti128_si256(m3, 0);                                             // vextracti128, AVX2
        __m256i vidx = _mm256_cvtepu16_epi32(data);                                                   // vpmovzxwd, AVX2
        __m256i mask = _mm256_cvtepi16_epi32(mask16);                                                 // vpmovsxwd, AVX2
        __m256i to_sum = _mm256_mask_i32gather_epi32(zero_256, &_ControlLengthAdd[0], vidx, mask, 4); // vpgatherdd, AVX2
        ret += reduce_add_epi32_256(to_sum);                                                          // AVX2
    }
    {
        __m128i data = _mm256_extracti128_si256(u, 1);                                                // vextracti128, AVX2
        __m128i mask16 = _mm256_extracti128_si256(m3, 1);                                             // vextracti128, AVX2
        __m256i vidx = _mm256_cvtepu16_epi32(data);                                                   // vpmovzxwd, AVX2
        __m256i mask = _mm256_cvtepi16_epi32(mask16);                                                 // vpmovsxwd, AVX2
        __m256i to_sum = _mm256_mask_i32gather_epi32(zero_256, &_ControlLengthAdd[0], vidx, mask, 4); // vpgatherdd, AVX2
        ret += reduce_add_epi32_256(to_sum);                                                          // AVX2
    }
    i32 mask32 = _mm256_movemask_epi8(m1or2);   // vpmovmskb, AVX2
    return ret + (_mm_popcnt_u32(mask32) >> 1); // popcnt, implies SSE4.2
}

force_inline Py_ssize_t count_escape_tail_u16_256_avx2(__m256i &u, usize count) {
    constexpr Py_ssize_t _BaseRet = 256 / 16;
    assert(count && count < _BaseRet);
    Py_ssize_t ret = _BaseRet;
    __m256i m1, m2, m3;
    _check_escape_u16_256_avx2_get_mask(u, m1, m2, m3);                            // AVX2
    __m256i m1or2 = _mm256_or_si256(m1, m2);                                       // vpor, AVX2
    __m256i tail_mask = _mm256_load_si256((__m256i *) &_MaskArray<u16>[count][0]); // vmovdqa, AVX
    {
        __m256i tmp = _mm256_and_si256(_mm256_or_si256(m1or2, m3), tail_mask); // vpor, vpand, AVX2
        if (likely(_mm256_testz_si256(tmp, tmp))) return ret;                  // vptest, AVX
    }
    // expand to i32, 128 -> 256 (2 times)
    m3 = _mm256_and_si256(m3, tail_mask);      // vpand, AVX2
    __m256i zero_256 = _mm256_setzero_si256(); // vpxor, AVX
    {
        __m128i data = _mm256_extracti128_si256(u, 0);                                                // vextracti128, AVX2
        __m128i mask16 = _mm256_extracti128_si256(m3, 0);                                             // vextracti128, AVX2
        __m256i vidx = _mm256_cvtepu16_epi32(data);                                                   // vpmovzxwd, AVX2
        __m256i mask = _mm256_cvtepi16_epi32(mask16);                                                 // vpmovsxwd, AVX2
        __m256i to_sum = _mm256_mask_i32gather_epi32(zero_256, &_ControlLengthAdd[0], vidx, mask, 4); // vpgatherdd, AVX2
        ret += reduce_add_epi32_256(to_sum);                                                          // AVX2
    }
    {
        __m128i data = _mm256_extracti128_si256(u, 1);                                                // vextracti128, AVX2
        __m128i mask16 = _mm256_extracti128_si256(m3, 1);                                             // vextracti128, AVX2
        __m256i vidx = _mm256_cvtepu16_epi32(data);                                                   // vpmovzxwd, AVX2
        __m256i mask = _mm256_cvtepi16_epi32(mask16);                                                 // vpmovsxwd, AVX2
        __m256i to_sum = _mm256_mask_i32gather_epi32(zero_256, &_ControlLengthAdd[0], vidx, mask, 4); // vpgatherdd, AVX2
        ret += reduce_add_epi32_256(to_sum);                                                          // AVX2
    }
    i32 mask32 = _mm256_movemask_epi8(_mm256_and_si256(m1or2, tail_mask)); // vpmovmskb, vpand, AVX2
    return ret + (_mm_popcnt_u32(mask32) >> 1);                            // popcnt, implies SSE4.2
}

force_inline Py_ssize_t count_escape_u32_256_avx2(__m256i &u) {
    constexpr Py_ssize_t _BaseRet = 256 / 32;
    Py_ssize_t ret = _BaseRet;
    __m256i m1, m2, m3;
    _check_escape_u32_256_avx2_get_mask(u, m1, m2, m3); // AVX2
    __m256i m1or2 = _mm256_or_si256(m1, m2);            // vpor, AVX2
    {
        __m256i tmp = _mm256_or_si256(m1or2, m3);             // vpor, AVX2
        if (likely(_mm256_testz_si256(tmp, tmp))) return ret; // vptest, AVX
    }
    // it is already i32
    __m256i zero_256 = _mm256_setzero_si256();                                               // vpxor, AVX
    __m256i to_sum = _mm256_mask_i32gather_epi32(zero_256, &_ControlLengthAdd[0], u, m3, 4); // vpgatherdd, AVX2
    ret += reduce_add_epi32_256(to_sum);                                                     // AVX2
    i32 mask32 = _mm256_movemask_epi8(m1or2);                                                // vpmovmskb, AVX2
    return ret + (_mm_popcnt_u32(mask32) >> 2);                                              // popcnt, implies SSE4.2
}

force_inline Py_ssize_t count_escape_tail_u32_256_avx2(__m256i &u, usize count) {
    constexpr Py_ssize_t _BaseRet = 256 / 32;
    Py_ssize_t ret = _BaseRet;
    __m256i m1, m2, m3;
    _check_escape_u32_256_avx2_get_mask(u, m1, m2, m3);                            // AVX2
    __m256i m1or2 = _mm256_or_si256(m1, m2);                                       // vpor, AVX2
    __m256i tail_mask = _mm256_load_si256((__m256i *) &_MaskArray<u32>[count][0]); // vmovdqa, AVX
    {
        __m256i tmp = _mm256_and_si256(_mm256_or_si256(m1or2, m3), tail_mask); // vpor, vpand, AVX2
        if (likely(_mm256_testz_si256(tmp, tmp))) return ret;                  // vptest, AVX
    }
    // it is already i32
    m3 = _mm256_and_si256(m3, tail_mask);                                                    // vpand, AVX2
    __m256i zero_256 = _mm256_setzero_si256();                                               // vpxor, AVX
    __m256i to_sum = _mm256_mask_i32gather_epi32(zero_256, &_ControlLengthAdd[0], u, m3, 4); // vpgatherdd, AVX2
    ret += reduce_add_epi32_256(to_sum);                                                     // AVX2
    i32 mask32 = _mm256_movemask_epi8(_mm256_and_si256(m1or2, tail_mask));                   // vpmovmskb, vpand, AVX2
    return ret + (_mm_popcnt_u32(mask32) >> 2);                                              // popcnt, implies SSE4.2
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
