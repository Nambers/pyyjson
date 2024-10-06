#ifndef PYYJSON_ENCODE_SSE2_H
#define PYYJSON_ENCODE_SSE2_H
#include "encode_shared.hpp"
#include <immintrin.h>

/*==============================================================================
 * SSE2
 *============================================================================*/

/*==============================================================================
 * SSE2 Check 128 Bits
 *============================================================================*/

force_inline void _check_escape_u8_128_sse2_get_mask(__m128i &u, __m128i &m1, __m128i &m2, __m128i &m3) {
    __m128i t1 = _mm_load_si128((__m128i *) _Quote_i8);      // movdqa, SSE2
    __m128i t2 = _mm_load_si128((__m128i *) _Slash_i8);      // movdqa, SSE2
    __m128i t3 = _mm_load_si128((__m128i *) _MinusOne_i8);   // movdqa, SSE2
    __m128i t4 = _mm_load_si128((__m128i *) _ControlMax_i8); // movdqa, SSE2
    m1 = _mm_cmpeq_epi8(u, t1);                              // pcmpeqb, SSE2
    m2 = _mm_cmpeq_epi8(u, t2);                              // pcmpeqb, SSE2
    __m128i r1 = _mm_cmpgt_epi8(u, t3);                      // u > -1, pcmpgtb, SSE2
    __m128i r2 = _mm_cmplt_epi8(u, t4);                      // u < 32, pcmpgtb, SSE2
    m3 = _mm_and_si128(r1, r2);                              // pand, SSE2
}

force_inline __m128i _check_escape_u8_128_sse2_impl(__m128i &u) {
    __m128i m1, m2, m3;
    _check_escape_u8_128_sse2_get_mask(u, m1, m2, m3); // SSE2
    return _mm_or_si128(_mm_or_si128(m1, m2), m3);     // por, SSE2
}

force_inline bool check_escape_u8_128_sse2(__m128i &u) {
    __m128i r = _check_escape_u8_128_sse2_impl(u); // SSE2
    return 0 == _mm_movemask_epi8(r);              // pmovmskb, SSE2
}

force_inline bool check_escape_tail_u8_128_sse2(__m128i &u, size_t count) {
    assert(count > 0 && count < 128 / 8);
    __m128i r = _check_escape_u8_128_sse2_impl(u); // SSE2
    int ri = _mm_movemask_epi8(r);                 // pmovmskb, SSE2
    return (((1ULL << count) - 1) & ri) == 0;
}

force_inline void _check_escape_u16_128_sse2_get_mask(__m128i &u, __m128i &m1, __m128i &m2, __m128i &m3) {
    __m128i t1 = _mm_load_si128((__m128i *) _Quote_i16);      // movdqa, SSE2
    __m128i t2 = _mm_load_si128((__m128i *) _Slash_i16);      // movdqa, SSE2
    __m128i t3 = _mm_load_si128((__m128i *) _MinusOne_i16);   // movdqa, SSE2
    __m128i t4 = _mm_load_si128((__m128i *) _ControlMax_i16); // movdqa, SSE2
    m1 = _mm_cmpeq_epi16(u, t1);                              // pcmpeqw, SSE2
    m2 = _mm_cmpeq_epi16(u, t2);                              // pcmpeqw, SSE2
    __m128i r1 = _mm_cmpgt_epi16(u, t3);                      // u > -1, pcmpgtw, SSE2
    __m128i r2 = _mm_cmplt_epi16(u, t4);                      // u < 32, pcmpgtw, SSE2
    m3 = _mm_and_si128(r1, r2);                               // pand, SSE2
}

force_inline __m128i _check_escape_u16_128_sse2_impl(__m128i &u) {
    __m128i m1, m2, m3;
    _check_escape_u16_128_sse2_get_mask(u, m1, m2, m3); // SSE2
    return _mm_or_si128(_mm_or_si128(m1, m2), m3);      // por, SSE2
}

force_inline bool check_escape_u16_128_sse2(__m128i &u) {
    __m128i r = _check_escape_u16_128_sse2_impl(u); // SSE2
    return 0 == _mm_movemask_epi8(r);               // pmovmskb, SSE2
}

force_inline bool check_escape_tail_u16_128_sse2(__m128i &u, size_t count) {
    assert(count > 0 && count < 128 / 16);
    __m128i r = _check_escape_u16_128_sse2_impl(u); // SSE2
    int ri = _mm_movemask_epi8(r);                  // pmovmskb, SSE2
    return ((((int) 1 << (count << 1)) - 1) & ri) == 0;
}

force_inline void _check_escape_u32_128_sse2_get_mask(__m128i &u, __m128i &m1, __m128i &m2, __m128i &m3) {
    __m128i t1 = _mm_load_si128((__m128i *) _Quote_i32);      // movdqa, SSE2
    __m128i t2 = _mm_load_si128((__m128i *) _Slash_i32);      // movdqa, SSE2
    __m128i t3 = _mm_load_si128((__m128i *) _MinusOne_i32);   // movdqa, SSE2
    __m128i t4 = _mm_load_si128((__m128i *) _ControlMax_i32); // movdqa, SSE2
    m1 = _mm_cmpeq_epi32(u, t1);                              // pcmpeqd, SSE2
    m2 = _mm_cmpeq_epi32(u, t2);                              // pcmpeqd, SSE2
    __m128i r1 = _mm_cmpgt_epi32(u, t3);                      // u > -1, pcmpgtd, SSE2
    __m128i r2 = _mm_cmplt_epi32(u, t4);                      // u < 32, pcmpgtd, SSE2
    m3 = _mm_and_si128(r1, r2);                               // pand, SSE2
}

force_inline __m128i _check_escape_u32_128_sse2_impl(__m128i &u) {
    __m128i m1, m2, m3;
    _check_escape_u32_128_sse2_get_mask(u, m1, m2, m3); // SSE2
    return _mm_or_si128(_mm_or_si128(m1, m2), m3);      // por, SSE2
}

force_inline bool check_escape_u32_128_sse2(__m128i &u) {
    __m128i r = _check_escape_u32_128_sse2_impl(u); // por, SSE2
    return 0 == _mm_movemask_epi8(r);               // pmovmskb, SSE2
}

force_inline bool check_escape_tail_u32_128_sse2(__m128i &u, size_t count) {
    assert(count > 0 && count < 128 / 32);
    __m128i r = _check_escape_u32_128_sse2_impl(u); // por, SSE2
    int ri = _mm_movemask_epi8(r);                  // pmovmskb, SSE2
    return ((((int) 1 << (count << 2)) - 1) & ri) == 0;
}

/*==============================================================================
 * SSE2 Count
 * We don't have any *gather* API in SSE2, so this is a bit tricky and may be slower.
 *============================================================================*/

int bit_count_32(u32 u) {
    u32 u_count = u - ((u >> 1) & 033333333333) - ((u >> 2) & 011111111111);
    return ((u_count + (u_count >> 3)) & 030707070707) % 63;
}

force_inline void _count_escape_u8_128_sse2_get_final_mask_impl(__m128i &u, __m128i &m1or2, __m128i &m3, __m128i &m_final_1, __m128i &m_final_2) {
    __m128i m4;
    {
        __m128i t1 = _mm_load_si128((__m128i *) _Seven_i8);    // movdqa, SSE2
        __m128i t2 = _mm_load_si128((__m128i *) _Fourteen_i8); // movdqa, SSE2
        __m128i t3 = _mm_load_si128((__m128i *) _Eleven_i8);   // movdqa, SSE2
        __m128i t4 = _mm_load_si128((__m128i *) _All_0XFF);    // movdqa, SSE2
        __m128i tmp1 = _mm_cmpgt_epi8(u, t1);                  // u > 7, pcmpgtb, SSE2
        __m128i tmp2 = _mm_cmplt_epi8(u, t2);                  // u < 14, pcmpgtb, SSE2
        __m128i tmp3 = _mm_cmpeq_epi8(u, t3);                  // u == 11, pcmpeqb, SSE2
        __m128i tmp4 = _mm_xor_si128(tmp3, t4);                // u != 11, pxor, SSE2
        m4 = _mm_and_si128(_mm_and_si128(tmp1, tmp2), tmp4);   // 7 < u < 14 && u != 11, pand, SSE2
    }
    m_final_1 = _mm_or_si128(m1or2, m4); // escape + 1, por, SSE2
    // 7 < u < 14 && u != 11 also implies -1 < u < 32
    m_final_2 = _mm_xor_si128(m3, m4); // escape + 5, pxor, SSE2
}

force_inline bool _count_escape_u8_128_sse2_get_mask(__m128i &u, __m128i &m_final_1, __m128i &m_final_2) {
    __m128i m1, m2, m3;
    _check_escape_u8_128_sse2_get_mask(u, m1, m2, m3); // SSE2
    __m128i m1or2 = _mm_or_si128(m1, m2);
    __m128i r = _mm_or_si128(m1or2, m3); // por, SSE2
    {
        if (likely(0 == _mm_movemask_epi8(r))) return true; // pmovmskb, SSE2
    }
    _count_escape_u8_128_sse2_get_final_mask_impl(u, m1or2, m3, m_final_1, m_final_2); // SSE2
    return false;
}

force_inline bool _count_escape_tail_u8_128_sse2_get_mask(__m128i &u, __m128i &m_final_1, __m128i &m_final_2, int tail_mask) {
    __m128i m1, m2, m3;
    _check_escape_u8_128_sse2_get_mask(u, m1, m2, m3); // SSE2
    __m128i m1or2 = _mm_or_si128(m1, m2);
    __m128i r = _mm_or_si128(m1or2, m3); // por, SSE2
    {
        if (likely(0 == (tail_mask & _mm_movemask_epi8(r)))) return true; // pmovmskb, SSE2
    }
    _count_escape_u8_128_sse2_get_final_mask_impl(u, m1or2, m3, m_final_1, m_final_2); // SSE2
    return false;
}

force_inline Py_ssize_t count_escape_u8_128_sse2(__m128i &u) {
    constexpr Py_ssize_t _BaseRet = 128 / 8;
    Py_ssize_t ret = _BaseRet;
    __m128i m_final_1, m_final_2;
    bool _c = _count_escape_u8_128_sse2_get_mask(u, m_final_1, m_final_2);
    if (likely(_c)) return ret;
    // SSE2 does not have popcnt
    ret += bit_count_32((u32) _mm_movemask_epi8(m_final_1));     // pmovmskb, SSE2
    ret += 5 * bit_count_32((u32) _mm_movemask_epi8(m_final_2)); // pmovmskb, SSE2
    return ret;
}

force_inline Py_ssize_t count_escape_tail_u8_128_sse2(__m128i &u, usize count) {
    constexpr Py_ssize_t _BaseRet = 128 / 8;
    assert(count && count < _BaseRet);
    const int tail_mask = ((int) 1 << count) - 1;
    Py_ssize_t ret = _BaseRet;
    __m128i m_final_1, m_final_2;
    bool _c = _count_escape_tail_u8_128_sse2_get_mask(u, m_final_1, m_final_2, tail_mask);
    if (likely(_c)) return ret;
    // SSE2 does not have popcnt
    ret += bit_count_32((u32) (tail_mask & _mm_movemask_epi8(m_final_1)));     // pmovmskb, SSE2
    ret += 5 * bit_count_32((u32) (tail_mask & _mm_movemask_epi8(m_final_2))); // pmovmskb, SSE2
    return ret;
}

force_inline void _count_escape_u16_128_sse2_get_final_mask_impl(__m128i &u, __m128i &m1or2, __m128i &m3, __m128i &m_final_1, __m128i &m_final_2) {
    __m128i m4;
    {
        __m128i t1 = _mm_load_si128((__m128i *) _Seven_i16);    // movdqa, SSE2
        __m128i t2 = _mm_load_si128((__m128i *) _Fourteen_i16); // movdqa, SSE2
        __m128i t3 = _mm_load_si128((__m128i *) _Eleven_i16);   // movdqa, SSE2
        __m128i t4 = _mm_load_si128((__m128i *) _All_0XFF);     // movdqa, SSE2
        __m128i tmp1 = _mm_cmpgt_epi16(u, t1);                  // u > 7, pcmpgtb, SSE2
        __m128i tmp2 = _mm_cmplt_epi16(u, t2);                  // u < 14, pcmpgtb, SSE2
        __m128i tmp3 = _mm_cmpeq_epi16(u, t3);                  // u == 11, pcmpeqb, SSE2
        __m128i tmp4 = _mm_xor_si128(tmp3, t4);                 // u != 11, pxor, SSE2
        m4 = _mm_and_si128(_mm_and_si128(tmp1, tmp2), tmp4);    // 7 < u < 14 && u != 11, pand, SSE2
    }
    m_final_1 = _mm_or_si128(m1or2, m4); // escape + 1, por, SSE2
    // 7 < u < 14 && u != 11 also implies -1 < u < 32
    m_final_2 = _mm_xor_si128(m3, m4); // escape + 5, pxor, SSE2
}

force_inline bool _count_escape_u16_128_sse2_get_mask(__m128i &u, __m128i &m_final_1, __m128i &m_final_2) {
    __m128i m1, m2, m3;
    _check_escape_u16_128_sse2_get_mask(u, m1, m2, m3); // SSE2
    __m128i m1or2 = _mm_or_si128(m1, m2);
    __m128i r = _mm_or_si128(m1or2, m3); // por, SSE2
    {
        if (likely(0 == _mm_movemask_epi8(r))) return true; // pmovmskb, SSE2
    }
    _count_escape_u16_128_sse2_get_final_mask_impl(u, m1or2, m3, m_final_1, m_final_2); // SSE2
    return false;
}

force_inline bool _count_escape_tail_u16_128_sse2_get_mask(__m128i &u, __m128i &m_final_1, __m128i &m_final_2, int tail_mask) {
    __m128i m1, m2, m3;
    _check_escape_u16_128_sse2_get_mask(u, m1, m2, m3); // SSE2
    __m128i m1or2 = _mm_or_si128(m1, m2);
    __m128i r = _mm_or_si128(m1or2, m3); // por, SSE2
    {
        if (likely(0 == (tail_mask & _mm_movemask_epi8(r)))) return true; // pmovmskb, SSE2
    }
    _count_escape_u16_128_sse2_get_final_mask_impl(u, m1or2, m3, m_final_1, m_final_2); // SSE2
    return false;
}

force_inline Py_ssize_t count_escape_u16_128_sse2(__m128i &u) {
    constexpr Py_ssize_t _BaseRet = 128 / 16;
    Py_ssize_t ret = _BaseRet;
    __m128i m_final_1, m_final_2;
    bool _c = _count_escape_u16_128_sse2_get_mask(u, m_final_1, m_final_2);
    if (likely(_c)) return ret;
    // SSE2 does not have popcnt
    ret += (bit_count_32((u32) _mm_movemask_epi8(m_final_1)) >> 1);     // pmovmskb, SSE2
    ret += 5 * (bit_count_32((u32) _mm_movemask_epi8(m_final_2)) >> 1); // pmovmskb, SSE2
    return ret;
}

force_inline Py_ssize_t count_escape_tail_u16_128_sse2(__m128i &u, usize count) {
    constexpr Py_ssize_t _BaseRet = 128 / 16;
    assert(count && count < _BaseRet);
    const int tail_mask = ((int) 1 << (count << 1)) - 1;
    Py_ssize_t ret = _BaseRet;
    __m128i m_final_1, m_final_2;
    bool _c = _count_escape_tail_u16_128_sse2_get_mask(u, m_final_1, m_final_2, tail_mask);
    if (likely(_c)) return ret;
    // SSE2 does not have popcnt
    ret += (bit_count_32((u32) (tail_mask & _mm_movemask_epi8(m_final_1))) >> 1);     // pmovmskb, SSE2
    ret += 5 * (bit_count_32((u32) (tail_mask & _mm_movemask_epi8(m_final_2))) >> 1); // pmovmskb, SSE2
    return ret;
}

force_inline void _count_escape_u32_128_sse2_get_final_mask_impl(__m128i &u, __m128i &m1or2, __m128i &m3, __m128i &m_final_1, __m128i &m_final_2) {
    // TODO I don't think this will be faster than just loop over the 4 * 32 bits...
    __m128i m4;
    {
        __m128i t1 = _mm_load_si128((__m128i *) _Seven_i32);    // movdqa, SSE2
        __m128i t2 = _mm_load_si128((__m128i *) _Fourteen_i32); // movdqa, SSE2
        __m128i t3 = _mm_load_si128((__m128i *) _Eleven_i32);   // movdqa, SSE2
        __m128i t4 = _mm_load_si128((__m128i *) _All_0XFF);     // movdqa, SSE2
        __m128i tmp1 = _mm_cmpgt_epi32(u, t1);                  // u > 7, pcmpgtd, SSE2
        __m128i tmp2 = _mm_cmplt_epi32(u, t2);                  // u < 14, pcmpgtd, SSE2
        __m128i tmp3 = _mm_cmpeq_epi32(u, t3);                  // u == 11, pcmpeqd, SSE2
        __m128i tmp4 = _mm_xor_si128(tmp3, t4);                 // u != 11, pxor, SSE2
        m4 = _mm_and_si128(_mm_and_si128(tmp1, tmp2), tmp4);    // 7 < u < 14 && u != 11, pand, SSE2
    }
    m_final_1 = _mm_or_si128(m1or2, m4); // escape + 1, por, SSE2
    // 7 < u < 14 && u != 11 also implies -1 < u < 32
    m_final_2 = _mm_xor_si128(m3, m4); // escape + 5, pxor, SSE2
}

force_inline bool _count_escape_u32_128_sse2_get_mask(__m128i &u, __m128i &m_final_1, __m128i &m_final_2) {
    __m128i m1, m2, m3;
    _check_escape_u32_128_sse2_get_mask(u, m1, m2, m3); // SSE2
    __m128i m1or2 = _mm_or_si128(m1, m2);
    __m128i r = _mm_or_si128(m1or2, m3); // por, SSE2
    {
        if (likely(0 == _mm_movemask_epi8(r))) return true; // pmovmskb, SSE2
    }
    _count_escape_u32_128_sse2_get_final_mask_impl(u, m1or2, m3, m_final_1, m_final_2); // SSE2
    return false;
}

force_inline bool _count_escape_tail_u32_128_sse2_get_mask(__m128i &u, __m128i &m_final_1, __m128i &m_final_2, int tail_mask) {
    __m128i m1, m2, m3;
    _check_escape_u32_128_sse2_get_mask(u, m1, m2, m3); // SSE2
    __m128i m1or2 = _mm_or_si128(m1, m2);
    __m128i r = _mm_or_si128(m1or2, m3); // por, SSE2
    {
        if (likely(0 == (tail_mask & _mm_movemask_epi8(r)))) return true; // pmovmskb, SSE2
    }
    _count_escape_u32_128_sse2_get_final_mask_impl(u, m1or2, m3, m_final_1, m_final_2); // SSE2
    return false;
}

force_inline Py_ssize_t count_escape_u32_128_sse2(__m128i &u) {
    constexpr Py_ssize_t _BaseRet = 128 / 32;
    Py_ssize_t ret = _BaseRet;
    __m128i m_final_1, m_final_2;
    bool _c = _count_escape_u32_128_sse2_get_mask(u, m_final_1, m_final_2);
    if (likely(_c)) return ret;
    // SSE2 does not have popcnt
    ret += (bit_count_32((u32) _mm_movemask_epi8(m_final_1)) >> 2);     // pmovmskb, SSE2
    ret += 5 * (bit_count_32((u32) _mm_movemask_epi8(m_final_2)) >> 2); // pmovmskb, SSE2
    return ret;
}

force_inline Py_ssize_t count_escape_tail_u32_128_sse2(__m128i &u, usize count) {
    constexpr Py_ssize_t _BaseRet = 128 / 32;
    assert(count && count < _BaseRet);
    const int tail_mask = ((int) 1 << (count << 2)) - 1;
    Py_ssize_t ret = _BaseRet;
    __m128i m_final_1, m_final_2;
    bool _c = _count_escape_tail_u32_128_sse2_get_mask(u, m_final_1, m_final_2, tail_mask);
    if (likely(_c)) return ret;
    // SSE2 does not have popcnt
    ret += (bit_count_32((u32) (tail_mask & _mm_movemask_epi8(m_final_1))) >> 2);     // pmovmskb, SSE2
    ret += 5 * (bit_count_32((u32) (tail_mask & _mm_movemask_epi8(m_final_2))) >> 2); // pmovmskb, SSE2
    return ret;
}

/* SSE2 does not support cvtepu. */

#endif // PYYJSON_ENCODE_SSE2_H
