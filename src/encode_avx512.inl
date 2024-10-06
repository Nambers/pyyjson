#ifndef PYYJSON_ENCODE_AVX512_H
#define PYYJSON_ENCODE_AVX512_H
#include "encode_avx2.inl"
#include "encode_shared.hpp"
#include <immintrin.h>

/*==============================================================================
 * AVX512
 *============================================================================*/

/*==============================================================================
 * AVX512 Check 128 Bits
 * These should be faster than SSE4 and AVX2 implementations
 * since the instructions are less.
 *============================================================================*/

force_inline bool check_escape_u8_128_avx512(__m128i &u) {
    __m128i t1 = _mm_load_si128((__m128i *) _Quote_i8);      // movdqa, sse2
    __mmask16 r1 = _mm_cmpeq_epi8_mask(u, t1);               // vpcmpb, AVX512BW + AVX512VL
    __m128i t2 = _mm_load_si128((__m128i *) _Slash_i8);      // movdqa, sse2
    __mmask16 r2 = _mm_cmpeq_epi8_mask(u, t2);               // vpcmpb, AVX512BW + AVX512VL
    __m128i t3 = _mm_load_si128((__m128i *) _ControlMax_i8); // movdqa, sse2
    __mmask16 r3 = _mm_cmplt_epu8_mask(u, t3);               // vpcmpub, AVX512BW + AVX512VL
    return (r1 | r2 | r3) == 0;
}

force_inline bool check_escape_tail_u8_128_avx512(__m128i &u, size_t count) {
    assert(count && count < 128 / 8);
    __m128i t1 = _mm_load_si128((__m128i *) _Quote_i8);      // movdqa, sse2
    __mmask16 r1 = _mm_cmpeq_epi8_mask(u, t1);               // vpcmpb, AVX512BW + AVX512VL
    __m128i t2 = _mm_load_si128((__m128i *) _Slash_i8);      // movdqa, sse2
    __mmask16 r2 = _mm_cmpeq_epi8_mask(u, t2);               // vpcmpb, AVX512BW + AVX512VL
    __m128i t3 = _mm_load_si128((__m128i *) _ControlMax_i8); // movdqa, sse2
    __mmask16 r3 = _mm_cmplt_epu8_mask(u, t3);               // vpcmpub, AVX512BW + AVX512VL
    return ((r1 | r2 | r3) & ((1ULL << count) - 1)) == 0;
}

force_inline bool check_escape_u16_128_avx512(__m128i &u) {
    __m128i t1 = _mm_load_si128((__m128i *) _Quote_i16);      // movdqa, sse2
    __mmask8 r1 = _mm_cmpeq_epi16_mask(u, t1);                // vpcmpw, AVX512BW + AVX512VL
    __m128i t2 = _mm_load_si128((__m128i *) _Slash_i16);      // movdqa, sse2
    __mmask8 r2 = _mm_cmpeq_epi16_mask(u, t2);                // vpcmpw, AVX512BW + AVX512VL
    __m128i t3 = _mm_load_si128((__m128i *) _ControlMax_i16); // movdqa, sse2
    __mmask8 r3 = _mm_cmplt_epu16_mask(u, t3);                // vpcmpuw, AVX512BW + AVX512VL
    return (r1 | r2 | r3) == 0;
}

force_inline bool check_escape_tail_u16_128_avx512(__m128i &u, size_t count) {
    assert(count && count < 128 / 16);
    __m128i t1 = _mm_load_si128((__m128i *) _Quote_i16);      // movdqa, sse2
    __mmask8 r1 = _mm_cmpeq_epi16_mask(u, t1);                // vpcmpw, AVX512BW + AVX512VL
    __m128i t2 = _mm_load_si128((__m128i *) _Slash_i16);      // movdqa, sse2
    __mmask8 r2 = _mm_cmpeq_epi16_mask(u, t2);                // vpcmpw, AVX512BW + AVX512VL
    __m128i t3 = _mm_load_si128((__m128i *) _ControlMax_i16); // movdqa, sse2
    __mmask8 r3 = _mm_cmplt_epu16_mask(u, t3);                // vpcmpuw, AVX512BW + AVX512VL
    return ((r1 | r2 | r3) & ((1ULL << count) - 1)) == 0;
}

force_inline bool check_escape_u32_128_avx512(__m128i &u) {
    __m128i t1 = _mm_load_si128((__m128i *) _Quote_i32);      // movdqa, sse2
    __mmask8 r1 = _mm_cmpeq_epi32_mask(u, t1);                // vpcmpeqd, AVX512F + AVX512VL
    __m128i t2 = _mm_load_si128((__m128i *) _Slash_i32);      // movdqa, sse2
    __mmask8 r2 = _mm_cmpeq_epi32_mask(u, t2);                // vpcmpeqd, AVX512F + AVX512VL
    __m128i t3 = _mm_load_si128((__m128i *) _ControlMax_i32); // movdqa, sse2
    __mmask8 r3 = _mm_cmplt_epu32_mask(u, t3);                // vpcmpud, AVX512F + AVX512VL
    return (r1 | r2 | r3) == 0;
}

force_inline bool check_escape_tail_u32_128_avx512(__m128i &u, size_t count) {
    assert(count && count < 128 / 32);
    __m128i t1 = _mm_load_si128((__m128i *) _Quote_i32);      // movdqa, sse2
    __mmask8 r1 = _mm_cmpeq_epi32_mask(u, t1);                // vpcmpeqd, AVX512F + AVX512VL
    __m128i t2 = _mm_load_si128((__m128i *) _Slash_i32);      // movdqa, sse2
    __mmask8 r2 = _mm_cmpeq_epi32_mask(u, t2);                // vpcmpeqd, AVX512F + AVX512VL
    __m128i t3 = _mm_load_si128((__m128i *) _ControlMax_i32); // movdqa, sse2
    __mmask8 r3 = _mm_cmplt_epu32_mask(u, t3);                // vpcmpud, AVX512F + AVX512VL
    return ((r1 | r2 | r3) & ((1ULL << count) - 1)) == 0;
}

/*==============================================================================
 * AVX512 Check 256 Bits
 * These should be faster than AVX2 implementations
 * since the instructions are less.
 *============================================================================*/

force_inline bool check_escape_u8_256_avx512(__m256i &u) {
    __m256i t1 = _mm256_load_si256((__m256i *) _Quote_i8);      // vmovdqa, AVX
    __mmask32 r1 = _mm256_cmpeq_epi8_mask(u, t1);               // vpcmpb, AVX512BW + AVX512VL
    __m256i t2 = _mm256_load_si256((__m256i *) _Slash_i8);      // vmovdqa, AVX
    __mmask32 r2 = _mm256_cmpeq_epi8_mask(u, t2);               // vpcmpb, AVX512BW + AVX512VL
    __m256i t3 = _mm256_load_si256((__m256i *) _ControlMax_i8); // vmovdqa, AVX
    __mmask32 r3 = _mm256_cmplt_epu8_mask(u, t3);               // vpcmpub, AVX512BW + AVX512VL
    return (r1 | r2 | r3) == 0;
}

force_inline bool check_escape_tail_u8_256_avx512(__m256i &u, size_t count) {
    assert(count && count < 256 / 8);
    __m256i t1 = _mm256_load_si256((__m256i *) _Quote_i8);      // vmovdqa, AVX
    __mmask32 r1 = _mm256_cmpeq_epi8_mask(u, t1);               // vpcmpb, AVX512BW + AVX512VL
    __m256i t2 = _mm256_load_si256((__m256i *) _Slash_i8);      // vmovdqa, AVX
    __mmask32 r2 = _mm256_cmpeq_epi8_mask(u, t2);               // vpcmpb, AVX512BW + AVX512VL
    __m256i t3 = _mm256_load_si256((__m256i *) _ControlMax_i8); // vmovdqa, AVX
    __mmask32 r3 = _mm256_cmplt_epu8_mask(u, t3);               // vpcmpub, AVX512BW + AVX512VL
    return ((r1 | r2 | r3) & ((1ULL << count) - 1)) == 0;
}

force_inline bool check_escape_u16_256_avx512(__m256i &u) {
    __m256i t1 = _mm256_load_si256((__m256i *) _Quote_i16);      // vmovdqa, AVX
    __mmask16 r1 = _mm256_cmpeq_epi16_mask(u, t1);               // vpcmpw, AVX512BW + AVX512VL
    __m256i t2 = _mm256_load_si256((__m256i *) _Slash_i16);      // vmovdqa, AVX
    __mmask16 r2 = _mm256_cmpeq_epi16_mask(u, t2);               // vpcmpw, AVX512BW + AVX512VL
    __m256i t3 = _mm256_load_si256((__m256i *) _ControlMax_i16); // vmovdqa, AVX
    __mmask16 r3 = _mm256_cmplt_epu16_mask(u, t3);               // vpcmpuw, AVX512BW + AVX512VL
    return (r1 | r2 | r3) == 0;
}

force_inline bool check_escape_tail_u16_256_avx512(__m256i &u, size_t count) {
    assert(count && count < 256 / 16);
    __m256i t1 = _mm256_load_si256((__m256i *) _Quote_i16);      // vmovdqa, AVX
    __mmask16 r1 = _mm256_cmpeq_epi16_mask(u, t1);               // vpcmpw, AVX512BW + AVX512VL
    __m256i t2 = _mm256_load_si256((__m256i *) _Slash_i16);      // vmovdqa, AVX
    __mmask16 r2 = _mm256_cmpeq_epi16_mask(u, t2);               // vpcmpw, AVX512BW + AVX512VL
    __m256i t3 = _mm256_load_si256((__m256i *) _ControlMax_i16); // vmovdqa, AVX
    __mmask16 r3 = _mm256_cmplt_epu16_mask(u, t3);               // vpcmpuw, AVX512BW + AVX512VL
    return ((r1 | r2 | r3) & ((1ULL << count) - 1)) == 0;
}

force_inline bool check_escape_u32_256_avx512(__m256i &u) {
    __m256i t1 = _mm256_load_si256((__m256i *) _Quote_i32);      // vmovdqa, AVX
    __mmask8 r1 = _mm256_cmpeq_epi32_mask(u, t1);                // vpcmpd, AVX512F + AVX512VL
    __m256i t2 = _mm256_load_si256((__m256i *) _Slash_i32);      // vmovdqa, AVX
    __mmask8 r2 = _mm256_cmpeq_epi32_mask(u, t2);                // vpcmpd, AVX512F + AVX512VL
    __m256i t3 = _mm256_load_si256((__m256i *) _ControlMax_i32); // vmovdqa, AVX
    __mmask8 r3 = _mm256_cmplt_epu32_mask(u, t3);                // vpcmpud, AVX512F + AVX512VL
    return (r1 | r2 | r3) == 0;
}

force_inline bool check_escape_tail_u32_256_avx512(__m256i &u, size_t count) {
    assert(count && count < 256 / 32);
    __m256i t1 = _mm256_load_si256((__m256i *) _Quote_i32);      // vmovdqa, AVX
    __mmask8 r1 = _mm256_cmpeq_epi32_mask(u, t1);                // vpcmpd, AVX512F + AVX512VL
    __m256i t2 = _mm256_load_si256((__m256i *) _Slash_i32);      // vmovdqa, AVX
    __mmask8 r2 = _mm256_cmpeq_epi32_mask(u, t2);                // vpcmpd, AVX512F + AVX512VL
    __m256i t3 = _mm256_load_si256((__m256i *) _ControlMax_i32); // vmovdqa, AVX
    __mmask8 r3 = _mm256_cmplt_epu32_mask(u, t3);                // vpcmpud, AVX512F + AVX512VL
    return ((r1 | r2 | r3) & ((1ULL << count) - 1)) == 0;
}

/*==============================================================================
 * AVX512 Check 512 Bits
 *============================================================================*/

force_inline bool check_escape_u8_512_avx512(__m512i &u) {      // AVX512F + AVX512BW
    __m512i t1 = _mm512_load_si512((__m512i *) _Quote_i8);      // vmovdqa32, AVX512F
    __mmask64 r1 = _mm512_cmpeq_epi8_mask(u, t1);               // vpcmpb, AVX512BW
    __m512i t2 = _mm512_load_si512((__m512i *) _Slash_i8);      // vmovdqa32, AVX512F
    __mmask64 r2 = _mm512_cmpeq_epi8_mask(u, t2);               // vpcmpb, AVX512BW
    __m512i t3 = _mm512_load_si512((__m512i *) _ControlMax_i8); // vmovdqa32, AVX512F
    __mmask64 r3 = _mm512_cmplt_epu8_mask(u, t3);               // vpcmpub, AVX512BW
    return (r1 | r2 | r3) == 0;
}

force_inline bool check_escape_tail_u8_512_avx512(__m512i &u, size_t count) { // AVX512F + AVX512BW
    assert(count && count < 512 / 8);
    __m512i t1 = _mm512_load_si512((__m512i *) _Quote_i8);      // vmovdqa32, AVX512F
    __mmask64 r1 = _mm512_cmpeq_epi8_mask(u, t1);               // vpcmpb, AVX512BW
    __m512i t2 = _mm512_load_si512((__m512i *) _Slash_i8);      // vmovdqa32, AVX512F
    __mmask64 r2 = _mm512_cmpeq_epi8_mask(u, t2);               // vpcmpb, AVX512BW
    __m512i t3 = _mm512_load_si512((__m512i *) _ControlMax_i8); // vmovdqa32, AVX512F
    __mmask64 r3 = _mm512_cmplt_epu8_mask(u, t3);               // vpcmpub, AVX512BW
    return ((r1 | r2 | r3) & (((__mmask64) 1 << count) - 1)) == 0;
}

force_inline bool check_escape_u16_512_avx512(__m512i &u) {      // AVX512F + AVX512BW
    __m512i t1 = _mm512_load_si512((__m512i *) _Quote_i16);      // vmovdqa32, AVX512F
    __mmask32 r1 = _mm512_cmpeq_epi16_mask(u, t1);               // vpcmpw, AVX512BW
    __m512i t2 = _mm512_load_si512((__m512i *) _Slash_i16);      // vmovdqa32, AVX512F
    __mmask32 r2 = _mm512_cmpeq_epi16_mask(u, t2);               // vpcmpw, AVX512BW
    __m512i t3 = _mm512_load_si512((__m512i *) _ControlMax_i16); // vmovdqa32, AVX512F
    __mmask32 r3 = _mm512_cmplt_epu16_mask(u, t3);               // vpcmpuw, AVX512BW
    return (r1 | r2 | r3) == 0;
}

force_inline bool check_escape_tail_u16_512_avx512(__m512i &u, size_t count) { // AVX512F + AVX512BW
    assert(count && count < 512 / 16);
    __m512i t1 = _mm512_load_si512((__m512i *) _Quote_i16);      // vmovdqa32, AVX512F
    __mmask32 r1 = _mm512_cmpeq_epi16_mask(u, t1);               // vpcmpw, AVX512BW
    __m512i t2 = _mm512_load_si512((__m512i *) _Slash_i16);      // vmovdqa32, AVX512F
    __mmask32 r2 = _mm512_cmpeq_epi16_mask(u, t2);               // vpcmpw, AVX512BW
    __m512i t3 = _mm512_load_si512((__m512i *) _ControlMax_i16); // vmovdqa32, AVX512F
    __mmask32 r3 = _mm512_cmplt_epu16_mask(u, t3);               // vpcmpuw, AVX512BW
    return ((r1 | r2 | r3) & (((__mmask32) 1 << count) - 1)) == 0;
}

force_inline bool check_escape_u32_512_avx512(__m512i &u) {      // AVX512F
    __m512i t1 = _mm512_load_si512((__m512i *) _Quote_i32);      // vmovdqa32, AVX512F
    auto r1 = _mm512_cmpeq_epi32_mask(u, t1);                    // vpcmpeqd, AVX512F
    __m512i t2 = _mm512_load_si512((__m512i *) _Slash_i32);      // vmovdqa32, AVX512F
    auto r2 = _mm512_cmpeq_epi32_mask(u, t2);                    // vpcmpeqd, AVX512F
    __m512i t3 = _mm512_load_si512((__m512i *) _ControlMax_i32); // vmovdqa32, AVX512F
    auto r3 = _mm512_cmplt_epu32_mask(u, t3);                    // vpcmpud, AVX512F
    return (r1 | r2 | r3) == 0;
}

force_inline bool check_escape_tail_u32_512_avx512(__m512i &u, size_t count) { // AVX512F
    assert(count && count < 512 / 32);
    __m512i t1 = _mm512_load_si512((__m512i *) _Quote_i32);      // vmovdqa32, AVX512F
    auto r1 = _mm512_cmpeq_epi32_mask(u, t1);                    // vpcmpeqd, AVX512F
    __m512i t2 = _mm512_load_si512((__m512i *) _Slash_i32);      // vmovdqa32, AVX512F
    auto r2 = _mm512_cmpeq_epi32_mask(u, t2);                    // vpcmpeqd, AVX512F
    __m512i t3 = _mm512_load_si512((__m512i *) _ControlMax_i32); // vmovdqa32, AVX512F
    auto r3 = _mm512_cmplt_epu32_mask(u, t3);                    // vpcmpud, AVX512F
    return ((r1 | r2 | r3) & ((1ULL << count) - 1)) == 0;
}

/*==============================================================================
 * AVX512 Count
 *============================================================================*/

force_inline Py_ssize_t count_escape_u8_512_avx512(__m512i &u) { // AVX512F + AVX512BW
    constexpr Py_ssize_t _BaseRet = 512 / 8;
    Py_ssize_t ret = _BaseRet;
    __m512i t1 = _mm512_load_si512((__m512i *) _Quote_i8);      // vmovdqa32, AVX512F
    __mmask64 r1 = _mm512_cmpeq_epi8_mask(u, t1);               // vpcmpb, AVX512BW
    __m512i t2 = _mm512_load_si512((__m512i *) _Slash_i8);      // vmovdqa32, AVX512F
    __mmask64 r2 = _mm512_cmpeq_epi8_mask(u, t2);               // vpcmpb, AVX512BW
    __m512i t3 = _mm512_load_si512((__m512i *) _ControlMax_i8); // vmovdqa32, AVX512F
    __mmask64 r3 = _mm512_cmplt_epu8_mask(u, t3);               // vpcmpub, AVX512BW
    if (likely((r1 | r2 | r3) == 0)) return _BaseRet;
    __m512i zero_512 = _mm512_setzero_si512(); // vpxorq, AVX512F
    // expand to i32, 128 -> 512 (4 times)
    __m128i tmp_128;
    __m512i vidx, to_sum_512;
    __mmask16 mask;
    // 1st
    for (usize i = 0; i < 4; ++i) {
        tmp_128 = _mm512_extracti32x4_epi32(u, i); // vextracti32x4, AVX512F
        mask = (r3 >> (16 * i)) & 0xFFFF;
        vidx = _mm512_cvtepu8_epi32(tmp_128);                                                     // vpmovzxbd, AVX512F
        to_sum_512 = _mm512_mask_i32gather_epi32(zero_512, mask, vidx, &_ControlLengthAdd[0], 4); // vpgatherdd, AVX512F
        ret += (Py_ssize_t) _mm512_reduce_add_epi32(to_sum_512);                                  // Sequence, AVX512F
    }
    return ret + (Py_ssize_t) _mm_popcnt_u64(r1 | r2); // popcnt, implies SSE4.2
}

force_inline Py_ssize_t count_escape_tail_u8_512_avx512(__m512i &u, size_t count) { // AVX512F + AVX512BW
    constexpr Py_ssize_t _BaseRet = 512 / 8;
    assert(count && count < _BaseRet);
    Py_ssize_t ret = _BaseRet;
    __m512i t1 = _mm512_load_si512((__m512i *) _Quote_i8);      // vmovdqa32, AVX512F
    __mmask64 r1 = _mm512_cmpeq_epi8_mask(u, t1);               // vpcmpb, AVX512BW
    __m512i t2 = _mm512_load_si512((__m512i *) _Slash_i8);      // vmovdqa32, AVX512F
    __mmask64 r2 = _mm512_cmpeq_epi8_mask(u, t2);               // vpcmpb, AVX512BW
    __m512i t3 = _mm512_load_si512((__m512i *) _ControlMax_i8); // vmovdqa32, AVX512F
    __mmask64 r3 = _mm512_cmplt_epu8_mask(u, t3);               // vpcmpub, AVX512BW
    __mmask64 tail_mask = (((__mmask64) 1 << count) - 1);
    if (likely(((r1 | r2 | r3) & tail_mask) == 0)) return _BaseRet;
    r3 = r3 & tail_mask;
    __m512i zero_512 = _mm512_setzero_si512(); // vpxorq, AVX512F
    // expand to i32, 128 -> 512 (4 times)
    __m128i tmp_128;
    __m512i vidx, to_sum_512;
    __mmask16 mask;
    // 1st
    for (usize i = 0; i < 4; ++i) {
        tmp_128 = _mm512_extracti32x4_epi32(u, i); // vextracti32x4, AVX512F
        mask = (r3 >> (16 * i)) & 0xFFFF;
        vidx = _mm512_cvtepu8_epi32(tmp_128);                                                     // vpmovzxbd, AVX512F
        to_sum_512 = _mm512_mask_i32gather_epi32(zero_512, mask, vidx, &_ControlLengthAdd[0], 4); // vpgatherdd, AVX512F
        ret += (Py_ssize_t) _mm512_reduce_add_epi32(to_sum_512);                                  // Sequence, AVX512F
    }
    return ret + (Py_ssize_t) _mm_popcnt_u64((r1 | r2) & tail_mask); // popcnt, implies SSE4.2
}

force_inline Py_ssize_t count_escape_u16_512_avx512(__m512i &u) { // AVX512F + AVX512BW
    constexpr Py_ssize_t _BaseRet = 512 / 16;
    Py_ssize_t ret = _BaseRet;
    __m512i t1 = _mm512_load_si512((__m512i *) _Quote_i8);      // vmovdqa32, AVX512F
    __mmask32 r1 = _mm512_cmpeq_epi16_mask(u, t1);              // vpcmpw, AVX512BW
    __m512i t2 = _mm512_load_si512((__m512i *) _Slash_i8);      // vmovdqa32, AVX512F
    __mmask32 r2 = _mm512_cmpeq_epi16_mask(u, t2);              // vpcmpw, AVX512BW
    __m512i t3 = _mm512_load_si512((__m512i *) _ControlMax_i8); // vmovdqa32, AVX512F
    __mmask32 r3 = _mm512_cmplt_epu16_mask(u, t3);              // vpcmpuw, AVX512BW
    if (likely((r1 | r2 | r3) == 0)) return _BaseRet;
    __m512i zero_512 = _mm512_setzero_si512(); // vpxorq, AVX512F
    // expand to i32, 256 -> 512 (2 times)
    __m256i tmp_256;
    __m512i vidx, to_sum_512;
    __mmask16 mask;
    // 1st
    for (usize i = 0; i < 2; ++i) {
        tmp_256 = _mm512_extracti32x8_epi32(u, i); // vextracti32x8, AVX512F
        mask = (r3 >> (16 * i)) & 0xFFFF;
        vidx = _mm512_cvtepu16_epi32(tmp_256);                                                    // vpmovzxwd, AVX512F
        to_sum_512 = _mm512_mask_i32gather_epi32(zero_512, mask, vidx, &_ControlLengthAdd[0], 4); // vpgatherdd, AVX512F
        ret += (Py_ssize_t) _mm512_reduce_add_epi32(to_sum_512);                                  // Sequence, AVX512F
    }
    return ret + (Py_ssize_t) _mm_popcnt_u32(r1 | r2); // popcnt, implies SSE4.2
}

force_inline Py_ssize_t count_escape_tail_u16_512_avx512(__m512i &u, usize count) { // AVX512F + AVX512BW
    constexpr Py_ssize_t _BaseRet = 512 / 16;
    assert(count && count < _BaseRet);
    Py_ssize_t ret = _BaseRet;
    __m512i t1 = _mm512_load_si512((__m512i *) _Quote_i8);      // vmovdqa32, AVX512F
    __mmask32 r1 = _mm512_cmpeq_epi16_mask(u, t1);              // vpcmpw, AVX512BW
    __m512i t2 = _mm512_load_si512((__m512i *) _Slash_i8);      // vmovdqa32, AVX512F
    __mmask32 r2 = _mm512_cmpeq_epi16_mask(u, t2);              // vpcmpw, AVX512BW
    __m512i t3 = _mm512_load_si512((__m512i *) _ControlMax_i8); // vmovdqa32, AVX512F
    __mmask32 r3 = _mm512_cmplt_epu16_mask(u, t3);              // vpcmpuw, AVX512BW
    __mmask32 tail_mask = ((__mmask32) 1 << count) - 1;
    if (likely(((r1 | r2 | r3) & tail_mask) == 0)) return _BaseRet;
    r3 = r3 & tail_mask;
    __m512i zero_512 = _mm512_setzero_si512(); // vpxorq, AVX512F
    // expand to i32, 256 -> 512 (2 times)
    __m256i tmp_256;
    __m512i vidx, to_sum_512;
    __mmask16 mask;
    // 1st
    for (usize i = 0; i < 2; ++i) {
        tmp_256 = _mm512_extracti32x8_epi32(u, i); // vextracti32x8, AVX512F
        mask = (r3 >> (16 * i)) & 0xFFFF;
        vidx = _mm512_cvtepu16_epi32(tmp_256);                                                    // vpmovzxwd, AVX512F
        to_sum_512 = _mm512_mask_i32gather_epi32(zero_512, mask, vidx, &_ControlLengthAdd[0], 4); // vpgatherdd, AVX512F
        ret += (Py_ssize_t) _mm512_reduce_add_epi32(to_sum_512);                                  // Sequence, AVX512F
    }
    return ret + (Py_ssize_t) _mm_popcnt_u32((r1 | r2) & tail_mask); // popcnt, implies SSE4.2
}

force_inline Py_ssize_t count_escape_u32_512_avx512(__m512i &u) { // AVX512F + AVX512BW
    constexpr Py_ssize_t _BaseRet = 512 / 32;
    Py_ssize_t ret = _BaseRet;
    __m512i t1 = _mm512_load_si512((__m512i *) _Quote_i8);      // vmovdqa32, AVX512F
    __mmask16 r1 = _mm512_cmpeq_epi32_mask(u, t1);              // vpcmpeqd, AVX512F
    __m512i t2 = _mm512_load_si512((__m512i *) _Slash_i8);      // vmovdqa32, AVX512F
    __mmask16 r2 = _mm512_cmpeq_epi32_mask(u, t2);              // vpcmpeqd, AVX512F
    __m512i t3 = _mm512_load_si512((__m512i *) _ControlMax_i8); // vmovdqa32, AVX512F
    __mmask16 r3 = _mm512_cmplt_epu32_mask(u, t3);              // vpcmpud, AVX512F
    if (likely((r1 | r2 | r3) == 0)) return _BaseRet;
    __m512i zero_512 = _mm512_setzero_si512(); // vpxorq, AVX512F
    // it is already i32
    __m512i to_sum_512 = _mm512_mask_i32gather_epi32(zero_512, r3, u, &_ControlLengthAdd[0], 4); // vpgatherdd, AVX512F
    ret += (Py_ssize_t) _mm512_reduce_add_epi32(to_sum_512);                                     // Sequence, AVX512F
    return ret + (Py_ssize_t) _mm_popcnt_u32((unsigned int) (r1 | r2));                          // popcnt, implies SSE4.2
}

force_inline Py_ssize_t count_escape_tail_u32_512_avx512(__m512i &u, usize count) { // AVX512F
    constexpr Py_ssize_t _BaseRet = 512 / 32;
    assert(count && count < _BaseRet);
    Py_ssize_t ret = _BaseRet;
    __m512i t1 = _mm512_load_si512((__m512i *) _Quote_i8);      // vmovdqa32, AVX512F
    __mmask16 r1 = _mm512_cmpeq_epi32_mask(u, t1);              // vpcmpeqd, AVX512F
    __m512i t2 = _mm512_load_si512((__m512i *) _Slash_i8);      // vmovdqa32, AVX512F
    __mmask16 r2 = _mm512_cmpeq_epi32_mask(u, t2);              // vpcmpeqd, AVX512F
    __m512i t3 = _mm512_load_si512((__m512i *) _ControlMax_i8); // vmovdqa32, AVX512F
    __mmask16 r3 = _mm512_cmplt_epu32_mask(u, t3);              // vpcmpud, AVX512F
    __mmask16 tail_mask = ((__mmask16) 1 << count) - 1;
    if (likely(((r1 | r2 | r3) & tail_mask) == 0)) return _BaseRet;
    r3 = r3 & tail_mask;
    __m512i zero_512 = _mm512_setzero_si512(); // vpxorq, AVX512F
    // it is already i32
    __m512i to_sum_512 = _mm512_mask_i32gather_epi32(zero_512, r3, u, &_ControlLengthAdd[0], 4); // vpgatherdd, AVX512F
    ret += (Py_ssize_t) _mm512_reduce_add_epi32(to_sum_512);                                     // Sequence, AVX512F
    return ret + (Py_ssize_t) _mm_popcnt_u32((unsigned int) ((r1 | r2) & tail_mask));            // popcnt, implies SSE4.2
}

/*==============================================================================
 * AVX512 cvtepu
 *============================================================================*/

template<UCSKind __from, UCSKind __to>
force_inline void cvtepu_128_avx512(__m128i &u, UCSType_t<__from> *&src, UCSType_t<__to> *&dst) { // AVX512
    // We can only handle UCS1 -> UCS4 in this case.
    if constexpr (__from != UCSKind::UCS1 || __to != UCSKind::UCS4) {
        return cvtepu_128_avx2<__from, __to>(u, src, dst);
    }
    CVTEPU_COMMON_DEF(128, 512);
    // 128 -> 512
    __m512i s;
    // Loop only once
    static_assert(__from == UCSKind::UCS1);
    static_assert(__to == UCSKind::UCS4);
    s = _mm512_cvtepu8_epi32(u);             // vpmovzxbd, AVX512F
    _mm512_storeu_si512((__m512i *) dst, s); // vmovdqu32, AVX512F
    src += _MoveDst;
    dst += _MoveDst;
    ptr_move_bits<__from, _SrcReadBits>(src);
    ptr_move_bits<__to, _DstWriteBits>(dst);
}

template<UCSKind __from, UCSKind __to>
force_inline void cvtepu_256_avx512(__m256i &u, UCSType_t<__from> *&src, UCSType_t<__to> *&dst) { // AVX512
    // We can't handle UCS1 -> UCS4 in this case.
    if constexpr (__from == UCSKind::UCS1 && __to == UCSKind::UCS4) {
        static_assert(false, "Can't do this, use cvtepu_128_avx512 instead.");
    }
    CVTEPU_COMMON_DEF(256, 512);
    // 256 -> 512
    __m512i s;
    if constexpr (__from == UCSKind::UCS2) {
        static_assert(__to == UCSKind::UCS4);
        s = _mm512_cvtepu16_epi32(u); // vpmovzxwd, AVX512F
    } else {
        static_assert(__to == UCSKind::UCS2);
        static_assert(__from == UCSKind::UCS1);
        s = _mm512_cvtepu8_epi16(u); // vpmovzxbw, AVX512BW
    }
    _mm512_storeu_si512((void *) dst, s); // vmovdqu32, AVX512F
    ptr_move_bits<__from>(src, _SrcReadBits);
    ptr_move_bits<__to>(dst, _DstWriteBits);
}

#endif // PYYJSON_ENCODE_AVX512_H
