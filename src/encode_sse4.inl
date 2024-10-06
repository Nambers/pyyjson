#ifndef PYYJSON_ENCODE_SSE4_H
#define PYYJSON_ENCODE_SSE4_H
#include "encode_shared.hpp"
#include "encode_sse2.inl"
#include <immintrin.h>

/*==============================================================================
 * SSE4.1
 *============================================================================*/

/*==============================================================================
 * SSE4.1 Check 128 Bits
 *============================================================================*/

force_inline bool check_escape_u8_128_sse4_1(__m128i &u) {
    __m128i r = _check_escape_u8_128_sse2_impl(u); // sse2
    return _mm_testz_si128(r, r);                  // ptest, sse4.1
}

force_inline bool check_escape_tail_u8_128_sse4_1(__m128i &u, size_t count) {
    assert(count > 0 && count < 128 / 8);
    __m128i r = _check_escape_u8_128_sse2_impl(u);                     // sse2
    __m128i m = _mm_load_si128((__m128i *) &_MaskArray<u8>[count][0]); // movdqa, sse2
    __m128i t = _mm_and_si128(r, m);                                   // pand, sse2
    return _mm_testz_si128(t, t);                                      // ptest, sse4.1
}

force_inline bool check_escape_u16_128_sse4_1(__m128i &u) {
    __m128i r = _check_escape_u16_128_sse2_impl(u); // sse2
    return _mm_testz_si128(r, r);                   // ptest, sse4.1
}

force_inline bool check_escape_tail_u16_128_sse4_1(__m128i &u, size_t count) {
    assert(count > 0 && count < 128 / 16);
    __m128i r = _check_escape_u16_128_sse2_impl(u);                     // sse2
    __m128i m = _mm_load_si128((__m128i *) &_MaskArray<u16>[count][0]); // movdqa, sse2
    __m128i t = _mm_and_si128(r, m);                                    // pand, sse2
    return _mm_testz_si128(t, t);                                       // ptest, sse4.1
}

force_inline bool check_escape_u32_128_sse4_1(__m128i &u) {
    __m128i r = _check_escape_u32_128_sse2_impl(u); // por, sse2
    return _mm_testz_si128(r, r);                   // ptest, sse4.1
}

force_inline bool check_escape_tail_u32_128_sse4_1(__m128i &u, size_t count) {
    assert(count > 0 && count < 128 / 32);
    __m128i r = _check_escape_u32_128_sse2_impl(u);                     // por, sse2
    __m128i m = _mm_load_si128((__m128i *) &_MaskArray<u32>[count][0]); // movdqa, sse2
    __m128i t = _mm_and_si128(r, m);                                    // pand, sse2
    return _mm_testz_si128(t, t);                                       // ptest, sse4.1
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

/*==============================================================================
 * SSE4.2
 *============================================================================*/

/*==============================================================================
 * SSE4.2 Count
 *============================================================================*/

force_inline Py_ssize_t count_escape_u8_128_sse4_2(__m128i &u) {
    constexpr Py_ssize_t _BaseRet = 128 / 8;
    Py_ssize_t ret = _BaseRet;
    __m128i m_final_1, m_final_2;
    bool _c = _count_escape_u8_128_sse2_get_mask(u, m_final_1, m_final_2);
    if (likely(_c)) return ret;
    ret += _mm_popcnt_u32((u32) _mm_movemask_epi8(m_final_1));     // pmovmskb, SSE2
    ret += 5 * _mm_popcnt_u32((u32) _mm_movemask_epi8(m_final_2)); // pmovmskb, SSE2
    return ret;
}

force_inline Py_ssize_t count_escape_tail_u8_128_sse4_2(__m128i &u, usize count) {
    constexpr Py_ssize_t _BaseRet = 128 / 8;
    assert(count && count < _BaseRet);
    const int tail_mask = ((int) 1 << count) - 1;
    Py_ssize_t ret = _BaseRet;
    __m128i m_final_1, m_final_2;
    bool _c = _count_escape_tail_u8_128_sse2_get_mask(u, m_final_1, m_final_2, tail_mask);
    if (likely(_c)) return ret;
    ret += _mm_popcnt_u32((u32) (tail_mask & _mm_movemask_epi8(m_final_1)));     // pmovmskb, SSE2
    ret += 5 * _mm_popcnt_u32((u32) (tail_mask & _mm_movemask_epi8(m_final_2))); // pmovmskb, SSE2
    return ret;
}

force_inline Py_ssize_t count_escape_u16_128_sse4_2(__m128i &u) {
    constexpr Py_ssize_t _BaseRet = 128 / 16;
    Py_ssize_t ret = _BaseRet;
    __m128i m_final_1, m_final_2;
    bool _c = _count_escape_u16_128_sse2_get_mask(u, m_final_1, m_final_2);
    if (likely(_c)) return ret;
    ret += (_mm_popcnt_u32((u32) _mm_movemask_epi8(m_final_1)) >> 1);     // pmovmskb, SSE2
    ret += 5 * (_mm_popcnt_u32((u32) _mm_movemask_epi8(m_final_2)) >> 1); // pmovmskb, SSE2
    return ret;
}

force_inline Py_ssize_t count_escape_tail_u16_128_sse4_2(__m128i &u, usize count) {
    constexpr Py_ssize_t _BaseRet = 128 / 16;
    assert(count && count < _BaseRet);
    const int tail_mask = ((int) 1 << (count << 1)) - 1;
    Py_ssize_t ret = _BaseRet;
    __m128i m_final_1, m_final_2;
    bool _c = _count_escape_tail_u16_128_sse2_get_mask(u, m_final_1, m_final_2, tail_mask);
    if (likely(_c)) return ret;
    ret += (_mm_popcnt_u32((u32) (tail_mask & _mm_movemask_epi8(m_final_1))) >> 1);     // pmovmskb, SSE2
    ret += 5 * (_mm_popcnt_u32((u32) (tail_mask & _mm_movemask_epi8(m_final_2))) >> 1); // pmovmskb, SSE2
    return ret;
}

force_inline Py_ssize_t count_escape_u32_128_sse4_2(__m128i &u) {
    constexpr Py_ssize_t _BaseRet = 128 / 32;
    Py_ssize_t ret = _BaseRet;
    __m128i m_final_1, m_final_2;
    bool _c = _count_escape_u32_128_sse2_get_mask(u, m_final_1, m_final_2);
    if (likely(_c)) return ret;
    ret += (_mm_popcnt_u32((u32) _mm_movemask_epi8(m_final_1)) >> 2);     // pmovmskb, SSE2
    ret += 5 * (_mm_popcnt_u32((u32) _mm_movemask_epi8(m_final_2)) >> 2); // pmovmskb, SSE2
    return ret;
}

force_inline Py_ssize_t count_escape_tail_u32_128_sse4_2(__m128i &u, usize count) {
    constexpr Py_ssize_t _BaseRet = 128 / 32;
    assert(count && count < _BaseRet);
    const int tail_mask = ((int) 1 << (count << 1)) - 1;
    Py_ssize_t ret = _BaseRet;
    __m128i m_final_1, m_final_2;
    bool _c = _count_escape_tail_u32_128_sse2_get_mask(u, m_final_1, m_final_2, tail_mask);
    if (likely(_c)) return ret;
    ret += (_mm_popcnt_u32((u32) (tail_mask & _mm_movemask_epi8(m_final_1))) >> 2);     // pmovmskb, SSE2
    ret += 5 * (_mm_popcnt_u32((u32) (tail_mask & _mm_movemask_epi8(m_final_2))) >> 2); // pmovmskb, SSE2
    return ret;
}

#endif
