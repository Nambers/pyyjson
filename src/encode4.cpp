#include "encode.hpp"
#include "encode_avx2.inl"
#include "encode_avx512.inl"
#include "encode_float.inl"
#include "encode_sse2.inl"
#include "encode_sse4.inl"
#include "pyyjson_config.h"
#include "yyjson.h"
#include <algorithm>
#include <array>
#include <stddef.h>

#include "unicode_utils.hpp"

/*==============================================================================
 * Macros
 *============================================================================*/

#define RETURN_ON_UNLIKELY_ERR(x) \
    do {                          \
        if (unlikely((x))) {      \
            return false;         \
        }                         \
    } while (0)

#define RETURN_ON_UNLIKELY_RESIZE_FAIL(_u_kind, _dst, _buffer_info, _required_len_u8)     \
    do {                                                                                  \
        if (unlikely((_required_len_u8) > (_buffer_info).m_size)) {                       \
            if (unlikely(!(_buffer_info).resizer<_u_kind>((_dst), (_required_len_u8)))) { \
                return false;                                                             \
            }                                                                             \
        }                                                                                 \
    } while (0)

#define NEED_INDENT(__indent) (static_cast<int>(__indent) != 0)

#define ENCODE_MOVE_SRC_DST(src, src_count, dst, dst_left, move_count) \
    do {                                                               \
        Py_ssize_t _temp__ = (move_count);                             \
        src += _temp__;                                                \
        dst += _temp__;                                                \
        src_count -= _temp__;                                          \
        dst_left -= _temp__;                                           \
    } while (0)

template<UCSKind __kind>
struct SrcInfo {
    UCSType_t<__kind> *src_start;
    Py_ssize_t src_size;
};


//
template<X86SIMDLevel _SIMDLevel>
__m128i load_unaligned_si128(const __m128i_u *__p) {
    if constexpr (_SIMDLevel >= X86SIMDLevel::SSE3) {
        return _mm_lddqu_si128(__p);
    } else {
        return _mm_loadu_si128(__p);
    }
}

/*==============================================================================
 * Constexpr Utils
 *============================================================================*/

static_inline constexpr bool size_is_pow2(usize size) {
    return (size & (size - 1)) == 0;
}

static_inline constexpr usize size_align_up(usize size, usize align) {
    if (size_is_pow2(align)) {
        return (size + (align - 1)) & ~(align - 1);
    } else {
        return size + align - (size + align - 1) % align - 1;
    }
}

force_inline constexpr bool check_support_512_copy(X86SIMDLevel _SIMDLevel) {
    return _SIMDLevel >= X86SIMDLevel::AVX512;
}

force_inline constexpr bool check_support_256_copy(X86SIMDLevel _SIMDLevel) {
    return _SIMDLevel >= X86SIMDLevel::AVX512;
}

/*==============================================================================
 * Wrappers for `cvtepu`
 *============================================================================*/

template<UCSKind __from, UCSKind __to, X86SIMDLevel __simd_level>
force_inline void cvtepu_128(__m128i &x, UCSType_t<__from> *&src, UCSType_t<__to> *&dst) {
    if constexpr (__simd_level >= X86SIMDLevel::AVX512) {
        cvtepu_128_avx512<__from, __to>(x, src, dst);
    } else if constexpr (__simd_level >= X86SIMDLevel::AVX2) {
        cvtepu_128_avx2<__from, __to>(x, src, dst);
    } else if constexpr (__simd_level >= X86SIMDLevel::SSE4) {
        cvtepu_128_sse4_1<__from, __to>(x, src, dst);
    } else {
        static_assert(false, "No cvtepu support");
    }
}

template<UCSKind __from, UCSKind __to, X86SIMDLevel __simd_level>
force_inline void cvtepu_256(__m256i &y, UCSType_t<__from> *&src, UCSType_t<__to> *&dst) {
    static_assert(__simd_level >= X86SIMDLevel::AVX2);
    if constexpr (__simd_level >= X86SIMDLevel::AVX512) {
        if constexpr (__from == UCSKind::UCS1 && __to == UCSKind::UCS4) {
            __m128i x;
            x = _mm256_castsi256_si128(y);
            cvtepu_128_avx512<__from, __to>(x, src, dst);
            x = _mm256_extracti128_si256(y, 1);
            cvtepu_128_avx512<__from, __to>(x, src, dst);
        } else {
            return cvtepu_256_avx512<__from, __to>(y, src, dst);
        }
    } else {
        __m128i x;
        x = _mm256_castsi256_si128(y);
        cvtepu_128_avx2<__from, __to>(x, src, dst);
        x = _mm256_extracti128_si256(y, 1);
        cvtepu_128_avx2<__from, __to>(x, src, dst);
    }
}

template<UCSKind __from, UCSKind __to, X86SIMDLevel __simd_level>
force_inline void cvtepu_512(__m512i &z, UCSType_t<__from> *&src, UCSType_t<__to> *&dst) {
    static_assert(__simd_level >= X86SIMDLevel::AVX512);
    if constexpr (__from == UCSKind::UCS1 && __to == UCSKind::UCS4) {
        __m128i x;
        x = _mm512_castsi512_si128(z);
        cvtepu_128_avx512<__from, __to>(x, src, dst);
        x = _mm512_extracti32x4_epi32(z, 1);
        cvtepu_128_avx512<__from, __to>(x, src, dst);
        x = _mm512_extracti32x4_epi32(z, 2);
        cvtepu_128_avx512<__from, __to>(x, src, dst);
        x = _mm512_extracti32x4_epi32(z, 3);
        cvtepu_128_avx512<__from, __to>(x, src, dst);
    } else {
        __m256i y;
        y = _mm512_castsi512_si256(z);
        cvtepu_256_avx512<__from, __to>(y, src, dst);
        y = _mm512_extracti64x4_epi64(z, 1);
        cvtepu_256_avx512<__from, __to>(y, src, dst);
    }
}

template<UCSKind __from, UCSKind __to>
force_inline bool copy_ucs_128_elevate_sse4_1(UCSType_t<__from> *&src, UCSType_t<__to> *&dst, BufferInfo &buffer_info) {
    SIMD_COMMON_DEF(128);

    static_assert(sizeof(usrc) < sizeof(udst), "sizeof(usrc) must be less sizeof(udst)");
    __m128i u;
    // TODO ABTEST with _mm_loadu_si128
    u = _mm_lddqu_si128((__m128i *) src); // lddqu, sse3

    if constexpr (__from == UCSKind::UCS2) {
        static_assert(__to == UCSKind::UCS4);
        if (likely(check_escape_u16_128_sse4_1(u))) {                                  // sse4.1
            cvtepu_128<UCSKind::UCS2, UCSKind::UCS4, X86SIMDLevel::SSE4>(u, src, dst); // sse4.1
        } else {
            // run a escape logic
            bool _c = copy_escape_fixsize<UCSKind::UCS2, UCSKind::UCS4, 128>(src, dst);
            RETURN_ON_UNLIKELY_ERR(!_c);
        }
    } else {
        static_assert(__from == UCSKind::UCS1);
        static_assert(_MoveSrc == 16);
        if (likely(check_escape_u8_128_sse4_1(u))) { // sse4.1
            if constexpr (__to == UCSKind::UCS2) {
                cvtepu_128<UCSKind::UCS1, UCSKind::UCS2, X86SIMDLevel::SSE4>(u, src, dst); // sse4.1
            } else {
                static_assert(__to == UCSKind::UCS4);
                cvtepu_128<UCSKind::UCS1, UCSKind::UCS4, X86SIMDLevel::SSE4>(u, src, dst); // sse4.1
            }
        } else {
            bool _c = copy_escape_fixsize<UCSKind::UCS1, __to, 128>(src, dst);
            RETURN_ON_UNLIKELY_ERR(!_c);
        }
    }

    return true;
}

/*==============================================================================
 * Wrappers for `check_escape`
 *============================================================================*/

template<UCSKind __kind, X86SIMDLevel __simd_level>
force_inline bool check_escape_512bits(__m512i &u) {
    static_assert(__simd_level >= X86SIMDLevel::AVX512);
    if constexpr (__kind == UCSKind::UCS1) {
        return check_escape_u8_512_avx512(u);
    } else if constexpr (__kind == UCSKind::UCS2) {
        return check_escape_u16_512_avx512(u);
    } else {
        static_assert(__kind == UCSKind::UCS4);
        return check_escape_u32_512_avx512(u);
    }
}

template<UCSKind __kind, X86SIMDLevel __simd_level>
force_inline bool check_escape_tail_512bits(__m512i &z, size_t count) {
    static_assert(__simd_level >= X86SIMDLevel::AVX512, "AVX2 and lower not supported");
    if constexpr (__kind == UCSKind::UCS1) {
        return check_escape_tail_u8_512_avx512(z, count);
    } else if constexpr (__kind == UCSKind::UCS2) {
        return check_escape_tail_u16_512_avx512(z, count);
    } else {
        return check_escape_tail_u32_512_avx512(z, count);
    }
}

template<UCSKind __kind, X86SIMDLevel __simd_level>
force_inline bool check_escape_256bits(__m256i &u) {
    if constexpr (__simd_level >= X86SIMDLevel::AVX512) {
        if constexpr (__kind == UCSKind::UCS1) {
            return check_escape_u8_256_avx512(u);
        } else if constexpr (__kind == UCSKind::UCS2) {
            return check_escape_u16_256_avx512(u);
        } else {
            static_assert(__kind == UCSKind::UCS4);
            return check_escape_u32_256_avx512(u);
        }
    } else {
        static_assert(__simd_level >= X86SIMDLevel::AVX2, "SSE4 and lower not supported");
        if constexpr (__kind == UCSKind::UCS1) {
            return check_escape_u8_256_avx2(u);
        } else if constexpr (__kind == UCSKind::UCS2) {
            return check_escape_u16_256_avx2(u);
        } else {
            static_assert(__kind == UCSKind::UCS4);
            return check_escape_u32_256_avx2(u);
        }
    }
}

template<UCSKind __kind, X86SIMDLevel __simd_level>
force_inline bool check_escape_tail_256bits(__m256i &u, size_t count) {
    if constexpr (__simd_level >= X86SIMDLevel::AVX512) {
        if constexpr (__kind == UCSKind::UCS1) {
            return check_escape_tail_u8_256_avx512(u, count);
        } else if constexpr (__kind == UCSKind::UCS2) {
            return check_escape_tail_u16_256_avx512(u, count);
        } else {
            static_assert(__kind == UCSKind::UCS4);
            return check_escape_tail_u32_256_avx512(u, count);
        }
    } else {
        static_assert(__simd_level >= X86SIMDLevel::AVX2, "SSE4 and lower not supported");
        if constexpr (__kind == UCSKind::UCS1) {
            return check_escape_tail_u8_256_avx2(u, count);
        } else if constexpr (__kind == UCSKind::UCS2) {
            return check_escape_tail_u16_256_avx2(u, count);
        } else {
            static_assert(__kind == UCSKind::UCS4);
            return check_escape_tail_u32_256_avx2(u, count);
        }
    }
}


template<UCSKind __kind, X86SIMDLevel __simd_level>
force_inline bool check_escape_128bits(__m128i &u) {
    if constexpr (__simd_level >= X86SIMDLevel::AVX512) {
        if constexpr (__kind == UCSKind::UCS1) {
            return check_escape_u8_128_avx512(u);
        } else if constexpr (__kind == UCSKind::UCS2) {
            return check_escape_u16_128_avx512(u);
        } else {
            static_assert(__kind == UCSKind::UCS4);
            return check_escape_u32_128_avx512(u);
        }
    } else if constexpr (__simd_level >= X86SIMDLevel::SSE4) {
        // AVX2 does not have better `check_escape` than SSE4, for 128bits.
        if constexpr (__kind == UCSKind::UCS1) {
            return check_escape_u8_128_sse4_1(u);
        } else if constexpr (__kind == UCSKind::UCS2) {
            return check_escape_u16_128_sse4_1(u);
        } else {
            static_assert(__kind == UCSKind::UCS4);
            return check_escape_u32_128_sse4_1(u);
        }
    } else {
        static_assert(__simd_level == X86SIMDLevel::SSE2);
        if constexpr (__kind == UCSKind::UCS1) {
            return check_escape_u8_128_sse2(u);
        } else if constexpr (__kind == UCSKind::UCS2) {
            return check_escape_u16_128_sse2(u);
        } else {
            static_assert(__kind == UCSKind::UCS4);
            return check_escape_u32_128_sse2(u);
        }
    }
}

template<UCSKind __kind, X86SIMDLevel __simd_level>
force_inline bool check_escape_tail_128bits(__m128i &u, size_t count) {
    if constexpr (__simd_level >= X86SIMDLevel::AVX512) {
        if constexpr (__kind == UCSKind::UCS1) {
            return check_escape_tail_u8_128_avx512(u, count);
        } else if constexpr (__kind == UCSKind::UCS2) {
            return check_escape_tail_u16_128_avx512(u, count);
        } else {
            static_assert(__kind == UCSKind::UCS4);
            return check_escape_tail_u32_128_avx512(u, count);
        }
    } else if constexpr (__simd_level >= X86SIMDLevel::SSE4) {
        // AVX2 does not have better `check_escape` than SSE4, for 128bits.
        if constexpr (__kind == UCSKind::UCS1) {
            return check_escape_tail_u8_128_sse4_1(u, count);
        } else if constexpr (__kind == UCSKind::UCS2) {
            return check_escape_tail_u16_128_sse4_1(u, count);
        } else {
            static_assert(__kind == UCSKind::UCS4);
            return check_escape_tail_u32_128_sse4_1(u, count);
        }
    } else {
        static_assert(__simd_level == X86SIMDLevel::SSE2);
        if constexpr (__kind == UCSKind::UCS1) {
            return check_escape_tail_u8_128_sse2(u, count);
        } else if constexpr (__kind == UCSKind::UCS2) {
            return check_escape_tail_u16_128_sse2(u, count);
        } else {
            static_assert(__kind == UCSKind::UCS4);
            return check_escape_tail_u32_128_sse2(u, count);
        }
    }
}


/*check_and_write*/

template<UCSKind __kind, X86SIMDLevel __simd_level>
force_inline bool check_and_write_128(void *dst, __m128i x) {
    _mm_storeu_si128((__m128i_u *) dst, x); // movdqu, SSE2
    return check_escape_128bits<__kind, __simd_level>(x);
}

template<UCSKind __kind, X86SIMDLevel __simd_level>
force_inline bool check_and_write_256(void *dst, __m256i y) {
    static_assert(__simd_level >= X86SIMDLevel::AVX);
    _mm256_storeu_si256((__m256i_u *) dst, y); // vmovdqu, AVX
    return check_escape_256bits<__kind, __simd_level>(y);
}

template<UCSKind __kind, X86SIMDLevel __simd_level>
force_inline bool check_and_write_512(void *dst, __m512i z) {
    static_assert(__simd_level >= X86SIMDLevel::AVX512);
    _mm512_storeu_si512((void *) dst, z); // vmovdqu32, AVX512F
    return check_escape_512bits<__kind, __simd_level>(z);
}


/*Copy Utils*/

template<X86SIMDLevel __simd_level>
force_inline void cpy_once_512_impl(u8 *&ddst, u8 *&ssrc) {
    static_assert(__simd_level >= X86SIMDLevel::AVX512);
    _mm512_storeu_si512((void *) ddst, _mm512_loadu_si512((void *) ssrc)); // vmovdqu32, AVX512F
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Walign-mismatch"
template<X86SIMDLevel __simd_level>
force_inline void cpy_aligned_once_512_impl(u8 *&ddst, u8 *&ssrc) {
    static_assert(__simd_level >= X86SIMDLevel::AVX512);
    _mm512_store_si512((void *) ddst, _mm512_load_si512((void *) ssrc)); // vmovdqa32, AVX512F
}
#pragma GCC diagnostic pop

template<X86SIMDLevel __simd_level>
force_inline void cpy_fast_512_impl(u8 *&ddst, u8 *&ssrc, Py_ssize_t &count, Py_ssize_t &allow_write_size) {
    static_assert(__simd_level >= X86SIMDLevel::AVX512);
    constexpr size_t _Align = 512 / 8;
    uintptr_t align_mod = (uintptr_t) ddst % _Align;
    if (align_mod == ((uintptr_t) ssrc % _Align)) {
        if (align_mod) {
            uintptr_t align_diff = _Align - align_mod;
            if (allow_write_size >= align_diff && count >= align_diff) {
                memcpy((void *) ddst, (void *) ssrc, align_diff);
                ENCODE_MOVE_SRC_DST(ssrc, count, ddst, allow_write_size, align_diff);
            } else {
                goto copy_unaligned;
            }
        }
        while (allow_write_size >= _Align && count > 0) {
            cpy_aligned_once_512_impl<__simd_level>(ddst, ssrc); // AVX512F
            ENCODE_MOVE_SRC_DST(ssrc, count, ddst, allow_write_size, _Align);
        }
        return;
    }
copy_unaligned:;
    while (allow_write_size >= _Align && count > 0) {
        cpy_once_512_impl<__simd_level>(ddst, ssrc); // AVX512F
        ENCODE_MOVE_SRC_DST(ssrc, count, ddst, allow_write_size, _Align);
    }
}

template<X86SIMDLevel __simd_level>
force_inline void cpy_once_256_impl(u8 *&ddst, u8 *&ssrc) {
    static_assert(__simd_level >= X86SIMDLevel::AVX2);
    _mm256_storeu_si256((__m256i *) ddst, _mm256_lddqu_si256((const __m256i_u *) ssrc)); // vmovdqu, vlddqu, AVX
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Walign-mismatch"
template<X86SIMDLevel __simd_level>
force_inline void cpy_aligned_once_256_impl(u8 *&ddst, u8 *&ssrc) {
    static_assert(__simd_level >= X86SIMDLevel::AVX2);
    _mm256_store_si256((__m256i *) ddst, _mm256_load_si256((const __m256i_u *) ssrc)); // vmovdqa, AVX
}
#pragma GCC diagnostic pop

template<X86SIMDLevel __simd_level>
force_inline void cpy_fast_256_impl(u8 *&ddst, u8 *&ssrc, Py_ssize_t &count, Py_ssize_t &allow_write_size) {
    static_assert(__simd_level >= X86SIMDLevel::AVX2);
    constexpr size_t _Align = 256 / 8;
    uintptr_t align_mod = (uintptr_t) ddst % _Align;
    if (align_mod == ((uintptr_t) ssrc % _Align)) {
        if (align_mod) {
            uintptr_t align_diff = _Align - align_mod;
            if (allow_write_size >= align_diff && count >= align_diff) {
                memcpy((void *) ddst, (void *) ssrc, align_diff);
                ENCODE_MOVE_SRC_DST(ssrc, count, ddst, allow_write_size, align_diff);
            } else {
                goto copy_unaligned;
            }
        }
        while (allow_write_size >= _Align && count > 0) {
            cpy_aligned_once_256_impl<__simd_level>(ddst, ssrc); // AVX
            ENCODE_MOVE_SRC_DST(ssrc, count, ddst, allow_write_size, _Align);
        }
        return;
    }
copy_unaligned:;

    while (allow_write_size >= _Align && count > 0) {
        cpy_once_256_impl<__simd_level>(ddst, ssrc); // AVX
        ENCODE_MOVE_SRC_DST(ssrc, count, ddst, allow_write_size, _Align);
    }
}

template<X86SIMDLevel __simd_level>
force_inline void cpy_once_128_impl(u8 *&ddst, u8 *&ssrc) {
    if constexpr (__simd_level >= X86SIMDLevel::SSE4) {
        _mm_storeu_si128((__m128i *) ddst, _mm_lddqu_si128((const __m128i_u *) ssrc)); // lddqu, SSE3
    } else {
        _mm_storeu_si128((__m128i *) ddst, _mm_loadu_si128((const __m128i_u *) ssrc)); // movdqu, SSE2
    }
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Walign-mismatch"
template<X86SIMDLevel __simd_level>
force_inline void cpy_aligned_once_128_impl(u8 *&ddst, u8 *&ssrc) {
    _mm_store_si128((__m128i *) ddst, _mm_load_si128((const __m128i_u *) ssrc)); // movdqa, SSE2
}
#pragma GCC diagnostic pop

template<X86SIMDLevel __simd_level>
force_inline void cpy_fast_128_impl(u8 *&ddst, u8 *&ssrc, Py_ssize_t &count, Py_ssize_t &allow_write_size) {
    constexpr size_t _Align = 128 / 8;
    uintptr_t align_mod = (uintptr_t) ddst % _Align;
    if (align_mod == ((uintptr_t) ssrc % _Align)) {
        if (align_mod) {
            uintptr_t align_diff = _Align - align_mod;
            if (allow_write_size >= align_diff && count >= align_diff) {
                memcpy((void *) ddst, (void *) ssrc, align_diff);
                ENCODE_MOVE_SRC_DST(ssrc, count, ddst, allow_write_size, align_diff);
            } else {
                goto copy_unaligned;
            }
        }
        while (allow_write_size >= _Align && count > 0) {
            cpy_aligned_once_128_impl<__simd_level>(ddst, ssrc); // SSE
            ENCODE_MOVE_SRC_DST(ssrc, count, ddst, allow_write_size, _Align);
        }
        return;
    }
copy_unaligned:;
    while (allow_write_size >= _Align && count > 0) {
        cpy_once_128_impl<__simd_level>(ddst, ssrc); // SSE2 or SSE3
        ENCODE_MOVE_SRC_DST(ssrc, count, ddst, allow_write_size, _Align);
    }
}

template<X86SIMDLevel __simd_level>
force_inline void cpy_fast(void *dst, void *src, Py_ssize_t count, Py_ssize_t allow_write_size) {
    assert(count <= allow_write_size);
    u8 *ddst = (u8 *) dst;
    u8 *ssrc = (u8 *) src;
    if constexpr (__simd_level >= X86SIMDLevel::AVX512) {
        goto copy_512;
    } else if constexpr (__simd_level >= X86SIMDLevel::AVX2) {
        goto copy_256;
    } else if constexpr (__simd_level >= X86SIMDLevel::SSE2) {
        goto copy_128;
    }
copy_512:
    if constexpr (__simd_level >= X86SIMDLevel::AVX512) {
        cpy_fast_512_impl<__simd_level>(ddst, ssrc, count, allow_write_size);
    } else {
        Py_UNREACHABLE();
    }
    if (likely(count <= 0)) return;
    goto copy_256;
copy_256:
    if constexpr (__simd_level >= X86SIMDLevel::AVX2) {
        cpy_fast_256_impl<__simd_level>(ddst, ssrc, count, allow_write_size);
    } else {
        Py_UNREACHABLE();
    }
    if (likely(count <= 0)) return;
    goto copy_128;
copy_128:
    cpy_fast_128_impl<__simd_level>(ddst, ssrc, count, allow_write_size);
    if (likely(count <= 0)) return;
    goto copy_final;
copy_final:
    memcpy((void *) ddst, (void *) ssrc, (size_t) count);
}


/*==============================================================================
 * Copy UCS impl
 *============================================================================*/

template<UCSKind __from, UCSKind __to, X86SIMDLevel _SIMDLevel>
force_inline void copy_ucs_elevate_avx512_nocheck(UCSType_t<__to> *&write_ptr, UCSType_t<__from> *src, Py_ssize_t len, Py_ssize_t write_count_left) {
    static_assert(__from < __to);
    static_assert(_SIMDLevel >= X86SIMDLevel::AVX512);
    assert(write_count_left >= 0);
    COMMON_TYPE_DEF();
    constexpr usize _ReadWriteOnce = 512 / 8 / sizeof(udst);
    const usize can_write_max_times = (usize) write_count_left / _ReadWriteOnce;
    usize do_write_times;
    if constexpr (__from == UCSKind::UCS1 && __to == UCSKind::UCS4) {
        // 128 -> 512, * 4
        const usize want_write_times = size_align_up((usize) len * sizeof(usrc), 128 / 8) / (128 / 8);
        do_write_times = std::min(can_write_max_times, want_write_times);
        for (Py_ssize_t i = 0; i < do_write_times; ++i) {
            __m128i x = _mm_lddqu_si128((const __m128i_u *) src); // lddqu, SSE3
            __m512i z = _mm512_cvtepu8_epi32(x);                  // vpmovzxbd, AVX512F
            _mm512_storeu_si512((void *) write_ptr, z);           // vmovdqu32, AVX512F
            src += _ReadWriteOnce;
            write_ptr += _ReadWriteOnce;
        }
    } else {
        // 256 -> 512, * 2
        const usize want_write_times = size_align_up((usize) len * sizeof(usrc), 256 / 8) / (256 / 8);
        do_write_times = std::min(can_write_max_times, want_write_times);
        for (Py_ssize_t i = 0; i < do_write_times; ++i) {
            __m256i y = _mm256_lddqu_si256((const __m256i_u *) src); // vlddqu, AVX
            __m512i z;
            if constexpr (__from == UCSKind::UCS1 && __to == UCSKind::UCS2) {
                z = _mm512_cvtepu8_epi16(y); // vpmovzxbw, AVX512BW
            } else if constexpr (__from == UCSKind::UCS2 && __to == UCSKind::UCS4) {
                z = _mm512_cvtepu16_epi32(y); // vpmovzxwd, AVX512F
            } else {
                static_assert(false, "Unreachable");
            }
            _mm512_storeu_si512((void *) write_ptr, z); // vmovdqu32, AVX512F
            src += _ReadWriteOnce;
            write_ptr += _ReadWriteOnce;
        }
    }
    len -= (Py_ssize_t) do_write_times * (Py_ssize_t) _ReadWriteOnce;
    if (likely(len <= 0)) return;
    // manual copy
    for (Py_ssize_t i = 0; i < len; ++i) {
        *write_ptr = (udst) *src;
        ++write_ptr;
        ++src;
    }
}

template<UCSKind __from, UCSKind __to, X86SIMDLevel _SIMDLevel>
force_inline void copy_ucs_elevate_sse4_or_avx2_nocheck(UCSType_t<__to> *&write_ptr, UCSType_t<__from> *src, Py_ssize_t len, Py_ssize_t write_count_left) {
    static_assert(__from < __to);
    static_assert(_SIMDLevel >= X86SIMDLevel::SSE4);
    assert(write_count_left >= 0);
    COMMON_TYPE_DEF();
    constexpr usize _ReadWriteOnce = 128 / 8 / sizeof(usrc);
    const usize can_write_max_times = (usize) write_count_left / _ReadWriteOnce;
    const usize want_write_times = size_align_up((usize) len * sizeof(usrc), 128 / 8) / (128 / 8);
    usize do_write_times = std::min(can_write_max_times, want_write_times);
    for (usize i = 0; i < do_write_times; ++i) {
        __m128i x = _mm_lddqu_si128((const __m128i_u *) src); // lddqu, SSE3
        cvtepu_128<__from, __to, _SIMDLevel>(x, src, write_ptr);
    }
    len -= (Py_ssize_t) do_write_times * (Py_ssize_t) _ReadWriteOnce;
    if (likely(len <= 0)) return;
    for (Py_ssize_t i = 0; i < len; ++i) {
        *write_ptr = (udst) *src;
        ++write_ptr;
        ++src;
    }
}

template<UCSKind __from, UCSKind __to, X86SIMDLevel _SIMDLevel>
force_inline void copy_ucs_elevate_general_nocheck(UCSType_t<__to> *&write_ptr, UCSType_t<__from> *src, Py_ssize_t len, Py_ssize_t write_count_left) {
    using udst = UCSType_t<__to>;
    assert(len >= 0);
    for (Py_ssize_t i = 0; i < len; ++i) {
        *write_ptr = (udst) *src;
        ++write_ptr;
        ++src;
    }
}

template<UCSKind __from, UCSKind __to, X86SIMDLevel _SIMDLevel>
force_inline void copy_elevate_nocheck(UCSType_t<__to> *&write_ptr, UCSType_t<__from> *src, Py_ssize_t len, Py_ssize_t write_count_left) {
    static_assert(__from < __to);
    if constexpr (_SIMDLevel >= X86SIMDLevel::AVX512) {
        return copy_ucs_elevate_avx512_nocheck<__from, __to, _SIMDLevel>(write_ptr, src, len, write_count_left);
    } else if constexpr (_SIMDLevel >= X86SIMDLevel::AVX2) {
        return copy_ucs_elevate_sse4_or_avx2_nocheck<__from, __to, _SIMDLevel>(write_ptr, src, len, write_count_left);
    } else if constexpr (_SIMDLevel >= X86SIMDLevel::SSE4) {
        return copy_ucs_elevate_sse4_or_avx2_nocheck<__from, __to, _SIMDLevel>(write_ptr, src, len, write_count_left);
    } else {
        return copy_ucs_elevate_general_nocheck<__from, __to, _SIMDLevel>(write_ptr, src, len, write_count_left);
    }
}

template<UCSKind __from, UCSKind __to, X86SIMDLevel _SIMDLevel>
force_inline void copy_ucs_elevate_128bits(UCSType_t<__from> *src, Py_ssize_t len, UCSType_t<__to> *&write_ptr, Py_ssize_t escape_len, Py_ssize_t write_count_left) {
    static_assert(__from < __to);
    static_assert(_SIMDLevel >= X86SIMDLevel::SSE4);
    COMMON_TYPE_DEF();
    // check 256 bits once
    constexpr usize _Check128Once = 128 / 8 / sizeof(usrc);

    assert(len < escape_len);
    assert(escape_len <= write_count_left);

    std::ptrdiff_t temp;
    udst *write_old;
    __m128i x;
    bool v;

    while (len >= (Py_ssize_t) _Check128Once) {
        x = load_unaligned_si128<_SIMDLevel>((const __m128i_u *) src); // SSE2
        v = check_escape_128bits<__from, _SIMDLevel>(x);
        if (likely(v)) {
            cvtepu_128<__from, __to, _SIMDLevel>(x, src, write_ptr);
            len -= _Check128Once;
            write_count_left -= _Check128Once;
            escape_len -= _Check128Once;
        } else {
            write_old = write_ptr;
            copy_escape_fixsize<__from, __to, 128>(src, write_ptr);
            temp = write_ptr - write_old;
            len -= _Check128Once;
            write_count_left -= temp;
            escape_len -= temp;
            if (escape_len == len) {
                goto no_escape;
            }
            continue;
        }
    }
    assert(len < (Py_ssize_t) _Check128Once);
    if (len == 0) return;
    if (likely(write_count_left >= _Check128Once)) {
        x = load_unaligned_si128<_SIMDLevel>((const __m128i_u *) src); // SSE2
        v = check_escape_tail_128bits<__from, _SIMDLevel>(x, len);
        if (likely(v)) {
            write_old = write_ptr;
            cvtepu_128<__from, __to, _SIMDLevel>(x, src, write_ptr);
            write_ptr = write_old + len;
            return;
        }
    }
    copy_escape_impl<__from, __to>(src, write_ptr, len);
    return;

no_escape:;
    copy_elevate_nocheck<__from, __to, _SIMDLevel>(write_ptr, src, len, write_count_left);
    return;
}

template<UCSKind __from, UCSKind __to, X86SIMDLevel _SIMDLevel>
force_inline void copy_ucs_elevate_256bits(UCSType_t<__from> *src, Py_ssize_t len, UCSType_t<__to> *&write_ptr, Py_ssize_t escape_len, Py_ssize_t write_count_left) {
    static_assert(__from < __to);
    static_assert(_SIMDLevel >= X86SIMDLevel::AVX2);
    COMMON_TYPE_DEF();
    // check 256 bits once
    constexpr usize _Check256Once = 256 / 8 / sizeof(usrc);
    constexpr usize _Check128Once = 128 / 8 / sizeof(usrc);

    assert(len < escape_len);
    assert(escape_len <= write_count_left);

    std::ptrdiff_t temp;
    udst *write_old;
    __m256i y;
    __m128i x;
    bool v;

    while (len >= (Py_ssize_t) _Check256Once) {
        y = _mm256_lddqu_si256((const __m256i_u *) src); // vlddqu, AVX
        v = check_escape_256bits<__from, _SIMDLevel>(y);
        if (likely(v)) {
            cvtepu_256<__from, __to, _SIMDLevel>(y, src, write_ptr);
            len -= _Check256Once;
            write_count_left -= _Check256Once;
            escape_len -= _Check256Once;
        } else {
            x = _mm256_castsi256_si128(y);
            v = check_escape_128bits<__from, _SIMDLevel>(x);
            if (v) {
                // .x
                cvtepu_128<__from, __to, _SIMDLevel>(x, src, write_ptr);
                len -= _Check128Once;
                write_count_left -= _Check128Once;
                escape_len -= _Check128Once;
            }
            // x
            write_old = write_ptr;
            copy_escape_fixsize<__from, __to, 128>(src, write_ptr);
            temp = write_ptr - write_old;
            len -= _Check128Once;
            write_count_left -= temp;
            escape_len -= temp;
            if (escape_len == len) {
                goto no_escape;
            }
            continue;
        }
    }
    //
    assert(len < (Py_ssize_t) _Check256Once);
    if (len == 0) return;
    if (likely(write_count_left >= _Check256Once)) {
        y = _mm256_loadu_si256((const __m256i_u *) src); // vmovdqu32, AVX512F
        v = check_escape_tail_256bits<__from, _SIMDLevel>(y, len);
        if (likely(v)) {
            write_old = write_ptr;
            cvtepu_256<__from, __to, _SIMDLevel>(y, src, write_ptr);
            write_ptr = write_old + len;
            return;
        }
    }
    copy_ucs_elevate_128bits<__from, __to, _SIMDLevel>(src, len, write_ptr, escape_len, write_count_left);
    return;

no_escape:;
    copy_elevate_nocheck<__from, __to, _SIMDLevel>(write_ptr, src, len, write_count_left);
    return;
}

template<UCSKind __from, UCSKind __to, X86SIMDLevel _SIMDLevel>
force_inline void copy_ucs_elevate_512bits(UCSType_t<__from> *src, Py_ssize_t len, UCSType_t<__to> *&write_ptr, Py_ssize_t escape_len, Py_ssize_t write_count_left) {
    static_assert(__from < __to);
    static_assert(_SIMDLevel >= X86SIMDLevel::AVX512);
    COMMON_TYPE_DEF();
    // check 512 bits once
    constexpr usize _Check512Once = 512 / 8 / sizeof(usrc);
    constexpr usize _Check256Once = 256 / 8 / sizeof(usrc);
    constexpr usize _Check128Once = 128 / 8 / sizeof(usrc);

    assert(len < escape_len);
    assert(escape_len <= write_count_left);

    std::ptrdiff_t temp;
    udst *write_old;
    __m512i z;
    __m256i y;
    __m128i x;
    bool v;

    while (len >= (Py_ssize_t) _Check512Once) {
        z = _mm512_loadu_si512((const void *) src); // vmovdqu32, AVX512F
        v = check_escape_512bits<__from, _SIMDLevel>(z);
        if (likely(v)) {
            cvtepu_512<__from, __to, _SIMDLevel>(z, src, write_ptr);
            len -= _Check512Once;
            write_count_left -= _Check512Once;
            escape_len -= _Check512Once;
        } else {
            y = _mm512_castsi512_si256(z);
            v = check_escape_256bits<__from, _SIMDLevel>(y);
            if (v) {
                // ..??
                cvtepu_256<__from, __to, _SIMDLevel>(y, src, write_ptr);
                len -= _Check256Once;
                write_count_left -= _Check256Once;
                escape_len -= _Check256Once;
                // moved: ??
                x = _mm512_extracti32x4_epi32(z, 2); // vextracti32x4, AVX512F
                v = check_escape_128bits<__from, _SIMDLevel>(x);
                if (v) {
                    // .x
                    cvtepu_128<__from, __to, _SIMDLevel>(x, src, write_ptr);
                    len -= _Check128Once;
                    write_count_left -= _Check128Once;
                    escape_len -= _Check128Once;
                }
                // x
                write_old = write_ptr;
                copy_escape_fixsize<__from, __to, 128>(src, write_ptr);
                temp = write_ptr - write_old;
                len -= _Check128Once;
                write_count_left -= temp;
                escape_len -= temp;
                if (escape_len == len) {
                    goto no_escape;
                }
                continue;
            }
            // ??
            x = _mm256_castsi256_si128(y);
            v = check_escape_128bits<__from, _SIMDLevel>(x);
            if (v) {
                // .x
                cvtepu_128<__from, __to, _SIMDLevel>(x, src, write_ptr);
                len -= _Check128Once;
                write_count_left -= _Check128Once;
                escape_len -= _Check128Once;
            }
            // x
            write_old = write_ptr;
            copy_escape_fixsize<__from, __to, 128>(src, write_ptr);
            temp = write_ptr - write_old;
            len -= _Check128Once;
            write_count_left -= temp;
            escape_len -= temp;
            if (escape_len == len) {
                goto no_escape;
            }
            continue;
        }
    }
    //
    assert(len < (Py_ssize_t) _Check512Once);
    if (len == 0) return;
    if (likely(write_count_left >= _Check512Once)) {
        z = _mm512_loadu_si512((const void *) src); // vmovdqu32, AVX512F
        v = check_escape_tail_512bits<__from, _SIMDLevel>(z, len);
        if (likely(v)) {
            write_old = write_ptr;
            cvtepu_512<__from, __to, _SIMDLevel>(z, src, write_ptr);
            write_ptr = write_old + len;
            return;
        }
    }
    copy_ucs_elevate_256bits<__from, __to, _SIMDLevel>(write_ptr, src, len, write_count_left, escape_len);
    return;

no_escape:;
    copy_elevate_nocheck<__from, __to, _SIMDLevel>(write_ptr, src, len, write_count_left);
    return;
}

template<UCSKind __from, UCSKind __to, X86SIMDLevel _SIMDLevel>
force_inline void copy_ucs_elevate_general(UCSType_t<__from> *src, Py_ssize_t len, UCSType_t<__to> *&write_ptr, Py_ssize_t escape_len, Py_ssize_t write_count_left) {
    COMMON_TYPE_DEF();
    assert(len >= 0);
    while (escape_len > len) {
        assert(len >= 0);
        if (len >= 4) {
            udst *old_dst = write_ptr;
            copy_escape_fixsize<__from, __to, 4 * sizeof(usrc) * 8>(src, write_ptr);
            Py_ssize_t temp = write_ptr - old_dst;
            len -= 4;
            write_count_left -= temp;
            escape_len -= temp;
        } else {
            copy_escape_impl<__from, __to>(src, write_ptr, len);
            return;
        }
    }
    if (len) {
        copy_ucs_elevate_general_nocheck<__from, __to, _SIMDLevel>(write_ptr, src, len, write_count_left);
    }
}

template<UCSKind __from, UCSKind __to, X86SIMDLevel __simd_level>
force_inline void copy_ucs_elevate(UCSType_t<__from> *src, Py_ssize_t src_count, UCSType_t<__to> *&dst, UCSType_t<__to> *const end_ptr, Py_ssize_t escape_len) {
    COMMON_TYPE_DEF();
    static_assert(__from < __to);
    Py_ssize_t write_count_left = end_ptr - dst;

    if constexpr (__simd_level >= X86SIMDLevel::AVX512) {
        copy_ucs_elevate_512bits<__from, __to, __simd_level>(src, src_count, dst, escape_len, write_count_left);
    } else if constexpr (__simd_level >= X86SIMDLevel::AVX2) {
        copy_ucs_elevate_256bits<__from, __to, __simd_level>(src, src_count, dst, escape_len, write_count_left);
    } else if constexpr (__simd_level >= X86SIMDLevel::SSE4) {
        copy_ucs_elevate_128bits<__from, __to, __simd_level>(src, src_count, dst, escape_len, write_count_left);
    } else {
        copy_ucs_elevate_general<__from, __to>(src, src_count, dst, escape_len, write_count_left);
    }
}

/*copy_ucs_same*/


template<UCSKind __kind, X86SIMDLevel __simd_level>
force_inline void copy_ucs_same_128bits(UCSType_t<__kind> *src, Py_ssize_t count, UCSType_t<__kind> *&dst, Py_ssize_t escape_count, Py_ssize_t write_count_left) {
    using uu = UCSType_t<__kind>;

    constexpr std::ptrdiff_t _Process128OnceCount = 128 / 8 / sizeof(uu);

    assert(count <= escape_count);
    assert(escape_count <= write_count_left);

    Py_ssize_t temp;
    uu *old_dst;
    __m128i x;
    bool v;

    if (escape_count == count) {
        goto no_escape;
    }

    while (count >= _Process128OnceCount) {
        x = load_unaligned_si128<__simd_level>((const __m128i_u *) src); // SSE2
        v = check_and_write_128<__kind, __simd_level>((void *) dst, x);
        if (likely(v)) {
            src += _Process128OnceCount;
            count -= _Process128OnceCount;
            dst += _Process128OnceCount;
            write_count_left -= _Process128OnceCount;
            escape_count -= _Process128OnceCount;
        } else {
            old_dst = dst;
            copy_escape_fixsize<__kind, __kind, 128>(src, dst);
            // src, dst moved, update count and write_count_left
            temp = dst - old_dst;
            count -= _Process128OnceCount;
            write_count_left -= temp;
            escape_count -= temp;
            assert(write_count_left >= 0 && escape_count >= 0);
            if (escape_count == count) {
                goto no_escape;
            }
            continue;
        }
    }
    //
    assert(count < _Process128OnceCount);
    if (!count) return;
    if (likely(write_count_left >= _Process128OnceCount)) {
        // try a tail check
        x = load_unaligned_si128<__simd_level>((const __m128i_u *) src); // SSE2
        _mm_storeu_si128((__m128i_u *) dst, x);                          // movdqu, SSE2
        v = check_escape_tail_128bits<__kind, __simd_level>(x, count);
        if (likely(v)) {
            dst += count;
            return;
        }
    }
    copy_escape_impl<__kind, __kind>(src, dst, count);
    return;

no_escape:;
    cpy_fast<__simd_level>((void *) dst, (void *) src, count * sizeof(uu), write_count_left * sizeof(uu));
    dst += count;
    return;
}

template<UCSKind __kind, X86SIMDLevel __simd_level>
force_inline void copy_ucs_same_256bits(UCSType_t<__kind> *src, Py_ssize_t count, UCSType_t<__kind> *&dst, Py_ssize_t escape_count, Py_ssize_t write_count_left) {
    static_assert(__simd_level >= X86SIMDLevel::AVX2, "Only AVX2 or above can use this");

    using uu = UCSType_t<__kind>;

    constexpr std::ptrdiff_t _Process256OnceCount = 256 / 8 / sizeof(uu);
    constexpr std::ptrdiff_t _Process128OnceCount = 128 / 8 / sizeof(uu);

    assert(count <= escape_count);
    assert(escape_count <= write_count_left);

    Py_ssize_t temp;
    uu *old_dst;
    __m256i y;
    __m128i x;
    bool v;

    if (escape_count == count) {
        goto no_escape;
    }

    while (count >= _Process256OnceCount) {
        y = _mm256_lddqu_si256((const __m256i_u *) src); // vlddqu, AVX
        v = check_and_write_256<__kind, __simd_level>((void *) dst, y);
        if (likely(v)) {
            src += _Process256OnceCount;
            count -= _Process256OnceCount;
            dst += _Process256OnceCount;
            write_count_left -= _Process256OnceCount;
            escape_count -= _Process256OnceCount;
        } else {
            x = _mm256_castsi256_si128(y); // no-op, AVX
            v = check_and_write_128<__kind, __simd_level>((void *) dst, x);
            if (v) {
                // .x
                src += _Process128OnceCount;
                count -= _Process128OnceCount;
                dst += _Process128OnceCount;
                write_count_left -= _Process128OnceCount;
                escape_count -= _Process128OnceCount;
                // moved: x
            }

            old_dst = dst;
            copy_escape_fixsize<__kind, __kind, 128>(src, dst);
            temp = dst - old_dst;
            count -= _Process128OnceCount;
            write_count_left -= temp;
            escape_count -= temp;
            if (escape_count == count) {
                goto no_escape;
            }
            continue;
        }
    }
    //
    assert(count < _Process256OnceCount);
    if (!count) return;
    if (likely(write_count_left >= _Process256OnceCount)) {
        // try a tail check
        y = _mm256_lddqu_si256((const __m256i_u *) src); // vlddqu, AVX
        _mm256_storeu_si256((__m256i_u *) dst, y);       // vmovdqu, AVX
        v = check_escape_tail_256bits<__kind, __simd_level>(y, count);
        if (likely(v)) {
            dst += count;
            return;
        }
    }
    copy_ucs_same_128bits<__kind, __simd_level>(src, count, dst, escape_count, write_count_left);
    return;

no_escape:;
    cpy_fast<__simd_level>((void *) dst, (void *) src, count * sizeof(uu), write_count_left * sizeof(uu));
    dst += count;
    return;
}

template<UCSKind __kind, X86SIMDLevel __simd_level>
force_inline void copy_ucs_same_512bits(UCSType_t<__kind> *src, Py_ssize_t count, UCSType_t<__kind> *&dst, Py_ssize_t escape_count, Py_ssize_t write_count_left) {
    static_assert(__simd_level >= X86SIMDLevel::AVX512, "Only AVX512 or above can use this");
    using uu = UCSType_t<__kind>;

    constexpr std::ptrdiff_t _Process512OnceCount = 512 / 8 / sizeof(uu);
    constexpr std::ptrdiff_t _Process256OnceCount = 256 / 8 / sizeof(uu);
    constexpr std::ptrdiff_t _Process128OnceCount = 128 / 8 / sizeof(uu);

    assert(count <= escape_count);
    assert(escape_count <= write_count_left);

    Py_ssize_t temp;
    uu *old_dst;
    __m512i z;
    __m256i y;
    __m128i x;
    bool v;

    if (escape_count == count) {
        goto no_escape;
    }

    while (count >= _Process512OnceCount) {
        z = _mm512_loadu_si512((void *) src); // vmovdqu32, AVX512F
        v = check_and_write_512<__kind, __simd_level>((void *) dst, z);
        if (likely(v)) {
            src += _Process512OnceCount;
            count -= _Process512OnceCount;
            dst += _Process512OnceCount;
            write_count_left -= _Process512OnceCount;
            escape_count -= _Process512OnceCount;
        } else {
            y = _mm512_castsi512_si256(z); // no-op, AVX512F
            v = check_and_write_256<__kind, __simd_level>((void *) dst, y);
            if (v) {
                // ..??
                src += _Process256OnceCount;
                count -= _Process256OnceCount;
                dst += _Process256OnceCount;
                write_count_left -= _Process256OnceCount;
                escape_count -= _Process256OnceCount;
                // moved: ??
                x = _mm512_extracti32x4_epi32(z, 2); // vextracti32x4, AVX512F
                v = check_and_write_128<__kind, __simd_level>((void *) dst, x);
                if (v) {
                    // .x
                    src += _Process128OnceCount;
                    count -= _Process128OnceCount;
                    dst += _Process128OnceCount;
                    write_count_left -= _Process128OnceCount;
                    escape_count -= _Process128OnceCount;
                    // moved: x
                }
                old_dst = dst;
                copy_escape_fixsize<__kind, __kind, 128>(src, dst);
                temp = dst - old_dst;
                count -= _Process128OnceCount;
                write_count_left -= temp;
                escape_count -= temp;
                if (escape_count == count) {
                    goto no_escape;
                }
                continue;
            } else {
                // ??
                x = _mm512_castsi512_si128(z); // no-op, AVX512F
                v = check_and_write_128<__kind, __simd_level>((void *) dst, x);
                if (v) {
                    // .x
                    src += _Process128OnceCount;
                    count -= _Process128OnceCount;
                    dst += _Process128OnceCount;
                    write_count_left -= _Process128OnceCount;
                    escape_count -= _Process128OnceCount;
                    // moved: x
                }
                // x
                old_dst = dst;
                copy_escape_fixsize<__kind, __kind, 128>(src, dst);
                temp = dst - old_dst;
                count -= _Process128OnceCount;
                write_count_left -= temp;
                escape_count -= temp;
                if (escape_count == count) {
                    goto no_escape;
                }
                continue;
            }
        }
    }
    //
    assert(count < _Process512OnceCount);
    if (!count) return;
    if (likely(write_count_left >= _Process512OnceCount)) {
        // try a tail check
        z = _mm512_loadu_si512((void *) src); // vmovdqu32, AVX512F
        _mm512_storeu_si512((void *) dst, z); // vmovdqu32, AVX512F
        v = check_escape_tail_512bits<__kind, __simd_level>(z, count);
        if (likely(v)) {
            dst += count;
            return;
        }
    }
    copy_ucs_same_256bits<__kind, __simd_level>(src, count, dst, write_count_left);
    return;
no_escape:;
    cpy_fast<__simd_level>((void *) dst, (void *) src, count * sizeof(uu), write_count_left * sizeof(uu));
    dst += count;
    return;
}


template<UCSKind __kind, X86SIMDLevel __simd_level>
force_inline void copy_ucs_same(UCSType_t<__kind> *src, Py_ssize_t src_count, UCSType_t<__kind> *&dst, UCSType_t<__kind> *const end_ptr, Py_ssize_t escape_len) {
    Py_ssize_t write_count_left = (Py_ssize_t) (end_ptr - dst);
    if constexpr (__simd_level >= X86SIMDLevel::AVX512) {
        copy_ucs_same_512bits<__kind, __simd_level>(src, src_count, dst, escape_len, write_count_left);
    } else if constexpr (__simd_level >= X86SIMDLevel::AVX2) {
        copy_ucs_same_256bits<__kind, __simd_level>(src, src_count, dst, escape_len, write_count_left);
    } else {
        copy_ucs_same_128bits<__kind, __simd_level>(src, src_count, dst, escape_len, write_count_left);
    }
}

template<UCSKind __from, UCSKind __to, X86SIMDLevel __simd_level>
force_inline void copy_ucs(UCSType_t<__from> *src, Py_ssize_t src_count, UCSType_t<__to> *&dst, UCSType_t<__to> *const end_ptr, Py_ssize_t escape_len) {
    if constexpr (__from != __to) {
        copy_ucs_elevate<__from, __to, __simd_level>(src, src_count, dst, end_ptr, escape_len);
    } else {
        copy_ucs_same<__from, __simd_level>(src, src_count, dst, end_ptr, escape_len);
    }
}

typedef struct CtnType {
    PyObject *ctn;
    Py_ssize_t index;
} CtnType;


thread_local CtnType _CtnStack[PYYJSON_ENCODE_MAX_RECURSION];

template<typename T, size_t L, T _DefaultVal, size_t... Is>
constexpr auto make_filled_array_impl(std::index_sequence<Is...>) {
    return std::array<T, L>{((void) Is, _DefaultVal)...};
}

template<typename T, size_t L, T _DefaultVal>
constexpr std::array<T, L> create_array() {
    return make_filled_array_impl<T, L, _DefaultVal>(std::make_index_sequence<L>{});
}

template<typename uu, Py_ssize_t reserve, bool _is_signed = true>
struct NaN_Template {
    static constexpr uu value[reserve] = {
            (uu) '-',
            (uu) 'n',
            (uu) 'a',
            (uu) 'n',
            (uu) ',',
    };
};

template<typename uu, Py_ssize_t reserve>
struct NaN_Template<uu, reserve, false> {
    static constexpr uu value[reserve] = {
            (uu) 'n',
            (uu) 'a',
            (uu) 'n',
            (uu) ',',
    };
};

template<typename uu, Py_ssize_t reserve, bool _is_signed = true>
struct Inf_Template {
    static constexpr uu value[reserve] = {
            (uu) '-',
            (uu) 'i',
            (uu) 'n',
            (uu) 'f',
            (uu) ',',
    };
};

template<typename uu, Py_ssize_t reserve>
struct Inf_Template<uu, reserve, false> {
    static constexpr uu value[reserve] = {
            (uu) 'i',
            (uu) 'n',
            (uu) 'f',
            (uu) ',',
    };
};


#define TODO() assert(false)


enum class PyFastTypes : u8 {
    T_Unicode,
    T_Long,
    T_False,
    T_True,
    T_None,
    T_Float,
    T_List,
    T_Dict,
    T_Tuple,
    Unknown,
};

PyFastTypes fast_type_check(PyObject *val) {
    PyTypeObject *type = Py_TYPE(val);
    if (type == &PyUnicode_Type) {
        return PyFastTypes::T_Unicode;
    } else if (type == &PyLong_Type) {
        return PyFastTypes::T_Long;
    } else if (type == &PyBool_Type) {
        if (val == Py_False) {
            return PyFastTypes::T_False;
        } else {
            assert(val == Py_True);
            return PyFastTypes::T_True;
        }
    } else if (type == &_PyNone_Type) { // TODO
        return PyFastTypes::T_None;
    } else if (type == &PyFloat_Type) {
        return PyFastTypes::T_Float;
    } else if (type == &PyList_Type) {
        return PyFastTypes::T_List;
    } else if (type == &PyDict_Type) {
        return PyFastTypes::T_Dict;
    } else if (type == &PyTuple_Type) {
        return PyFastTypes::T_Tuple;
    } else {
        return PyFastTypes::Unknown;
    }
}


struct DumpOption {
    IndentLevel indent_level;
};

constexpr X86SIMDLevel check_simd_level() {
    // TODO
    return X86SIMDLevel::AVX2;
}

thread_local u32 _ObjViewFirstBuffer[(usize) PYYJSON_ENCODE_OBJ_VIEW_BUFFER_INIT_SIZE];
thread_local void *_ViewDataFirstBuffer[(usize) PYYJSON_ENCODE_VIEW_DATA_BUFFER_INIT_SIZE];
static_assert((PYYJSON_ENCODE_OBJ_VIEW_BUFFER_INIT_SIZE % (512 / 8 / sizeof(u32))) == 0);
static_assert((PYYJSON_ENCODE_VIEW_DATA_BUFFER_INIT_SIZE % (512 / 8 / sizeof(void *))) == 0);

struct ObjViewer {
    // unicode related data
    Py_ssize_t m_unicode_count;
    UCSKind m_unicode_kind;
    bool m_is_all_ascii;
    // status
    int *const obj_stack = (int *) &_CtnStack[0]; // points to _CtnStack, reuse
    Py_ssize_t m_cur_depth;
    u32 m_view_payload;
    bool m_is_in_obj;
    // view buffer
    Py_ssize_t m_obj_view_buffer_count;
    u32 *m_cur_obj_view_rw_ptr;
    u32 *m_cur_obj_view_end_ptr;
    Py_ssize_t m_obj_view_cur_rw_index;
    // data buffer
    Py_ssize_t m_data_buffer_count;
    void **m_cur_data_rw_ptr;
    void **m_cur_data_end_ptr;
    Py_ssize_t m_data_cur_rw_index;
    // view buffers pointer
    u32 *m_obj_view_buffers[SIZEOF_UINTPTR_T * 8];
    // data buffers pointer
    void **m_data_buffers[SIZEOF_UINTPTR_T * 8];
};

#define INIT_FROM_THREAD_LOCAL_BUFFER(viewer, rw_ptr, end_ptr, buffers, tls_buffer, offset) \
    do {                                                                                    \
        viewer->rw_ptr = (decltype(viewer->rw_ptr)) &tls_buffer[0];                         \
        viewer->end_ptr = viewer->rw_ptr + (offset);                                        \
        viewer->buffers[0] = viewer->rw_ptr;                                                \
    } while (0)

force_inline void init_viewer(ObjViewer *viewer) {
    viewer->m_unicode_count = 0;
    viewer->m_unicode_kind = UCSKind::UCS1;
    viewer->m_is_all_ascii = true;
    viewer->m_cur_depth = 0;
    viewer->m_is_in_obj = true;
    //
    viewer->m_obj_view_buffer_count = 1;
    viewer->m_obj_view_cur_rw_index = 0;
    INIT_FROM_THREAD_LOCAL_BUFFER(viewer, m_cur_obj_view_rw_ptr, m_cur_obj_view_end_ptr, m_obj_view_buffers, _ObjViewFirstBuffer, PYYJSON_ENCODE_OBJ_VIEW_BUFFER_INIT_SIZE);
    //
    viewer->m_data_buffer_count = 1;
    viewer->m_data_cur_rw_index = 0;
    INIT_FROM_THREAD_LOCAL_BUFFER(viewer, m_cur_data_rw_ptr, m_cur_data_end_ptr, m_data_buffers, _ViewDataFirstBuffer, PYYJSON_ENCODE_VIEW_DATA_BUFFER_INIT_SIZE);
}

force_inline void clear_viewer(ObjViewer *viewer) {
    for (Py_ssize_t i = 1; i < viewer->m_obj_view_buffer_count; i++) {
        free(viewer->m_obj_view_buffers[i]);
    }
    for (Py_ssize_t i = 1; i < viewer->m_data_buffer_count; i++) {
        free(viewer->m_data_buffers[i]);
    }
}

force_inline Py_ssize_t get_view_buffer_length(usize index) {
    return PYYJSON_ENCODE_OBJ_VIEW_BUFFER_INIT_SIZE << index;
}

force_inline Py_ssize_t get_data_buffer_length(usize index) {
    return PYYJSON_ENCODE_VIEW_DATA_BUFFER_INIT_SIZE << index;
}

force_inline void reset_view_for_read(ObjViewer *viewer) {
    viewer->m_obj_view_cur_rw_index = 0;
    viewer->m_cur_obj_view_rw_ptr = viewer->m_obj_view_buffers[0];
    viewer->m_cur_obj_view_end_ptr = viewer->m_cur_obj_view_rw_ptr + get_view_buffer_length(0);
    viewer->m_data_cur_rw_index = 0;
    viewer->m_cur_data_rw_ptr = viewer->m_data_buffers[0];
    viewer->m_cur_data_end_ptr = viewer->m_cur_data_rw_ptr + get_data_buffer_length(0);
}

force_inline bool write_obj_view(ObjViewer *viewer, u32 view) {
    if (unlikely(viewer->m_cur_obj_view_rw_ptr >= viewer->m_cur_obj_view_end_ptr)) {
        assert(viewer->m_cur_obj_view_rw_ptr == viewer->m_cur_obj_view_end_ptr);
        usize next_index = (usize) viewer->m_obj_view_buffer_count;
        Py_ssize_t next_buffer_length = get_view_buffer_length(next_index);
        void *new_buffer = aligned_alloc(sizeof(u32), (usize) next_buffer_length * sizeof(u32));
        if (unlikely(!new_buffer)) goto fail_mem;
        viewer->m_obj_view_buffers[next_index] = (u32 *) new_buffer;
        viewer->m_obj_view_buffer_count++;
        viewer->m_cur_obj_view_rw_ptr = (u32 *) new_buffer;
        viewer->m_cur_obj_view_end_ptr = viewer->m_cur_obj_view_rw_ptr + next_buffer_length;
        // DONT modify m_obj_view_cur_rw_index
    }
    assert(viewer->m_cur_obj_view_rw_ptr < viewer->m_cur_obj_view_end_ptr);
    *viewer->m_cur_obj_view_rw_ptr++ = view;
    return true;
fail_mem:;
    PyErr_NoMemory();
    return false;
}

force_inline u32 read_obj_view(ObjViewer *viewer) {
    if (unlikely(viewer->m_cur_obj_view_rw_ptr >= viewer->m_cur_obj_view_end_ptr)) {
        assert(viewer->m_cur_obj_view_rw_ptr == viewer->m_cur_obj_view_end_ptr);
        Py_ssize_t cur_rw_index = ++viewer->m_obj_view_cur_rw_index;
        assert(cur_rw_index < viewer->m_obj_view_buffer_count);
        viewer->m_cur_obj_view_rw_ptr = viewer->m_obj_view_buffers[cur_rw_index];
        Py_ssize_t length = get_view_buffer_length(cur_rw_index);
        viewer->m_cur_obj_view_end_ptr = viewer->m_cur_obj_view_rw_ptr + length;
        // DONT modify m_obj_view_buffer_count
    }
    assert(viewer->m_cur_obj_view_rw_ptr < viewer->m_cur_obj_view_end_ptr);
    return *viewer->m_cur_obj_view_rw_ptr++;
}

template<typename T>
force_inline bool write_view_data(ObjViewer *viewer, T data) {
    static_assert((sizeof(T) % sizeof(void *)) == 0);
    if (unlikely((uintptr_t) viewer->m_cur_data_rw_ptr + sizeof(T) > (uintptr_t) viewer->m_cur_data_end_ptr)) {
        usize next_index = (usize) viewer->m_data_buffer_count;
        Py_ssize_t next_buffer_length = get_data_buffer_length(next_index);
        void *new_buffer = aligned_alloc(512 / 8, (usize) next_buffer_length * sizeof(void *));
        if (unlikely(!new_buffer)) goto fail_mem;
        viewer->m_data_buffers[next_index] = (void **) new_buffer;
        viewer->m_data_buffer_count++;
        viewer->m_cur_data_rw_ptr = (void **) new_buffer;
        viewer->m_cur_data_end_ptr = viewer->m_cur_data_rw_ptr + next_buffer_length;
        // DONT modify m_data_cur_rw_index
    }
    assert((uintptr_t) viewer->m_cur_data_rw_ptr + sizeof(T) <= (uintptr_t) viewer->m_cur_data_end_ptr);
    *(T *) viewer->m_cur_data_rw_ptr = data;
    viewer->m_cur_data_rw_ptr += sizeof(T) / sizeof(void *);
    return true;
fail_mem:;
    PyErr_NoMemory();
    return false;
}

template<typename T>
force_inline T *reserve_view_data_buffer(ObjViewer *viewer) {
    static_assert((sizeof(T) % sizeof(void *)) == 0);
    T *ret;
    if (unlikely((uintptr_t) viewer->m_cur_data_rw_ptr + sizeof(T) > (uintptr_t) viewer->m_cur_data_end_ptr)) {
        usize next_index = (usize) viewer->m_data_buffer_count;
        Py_ssize_t next_buffer_length = get_data_buffer_length(next_index);
        void *new_buffer = aligned_alloc(512 / 8, (usize) next_buffer_length * sizeof(void *));
        if (unlikely(!new_buffer)) goto fail_mem;
        viewer->m_data_buffers[next_index] = (void **) new_buffer;
        viewer->m_data_buffer_count++;
        viewer->m_cur_data_rw_ptr = (void **) new_buffer;
        viewer->m_cur_data_end_ptr = viewer->m_cur_data_rw_ptr + next_buffer_length;
        // DONT modify m_data_cur_rw_index
    }
    assert((uintptr_t) viewer->m_cur_data_rw_ptr + sizeof(T) <= (uintptr_t) viewer->m_cur_data_end_ptr);
    ret = (T *) viewer->m_cur_data_rw_ptr;
    viewer->m_cur_data_rw_ptr += sizeof(T) / sizeof(void *);
    return ret;
fail_mem:;
    PyErr_NoMemory();
    return NULL;
}

template<typename T>
force_inline T read_view_data(ObjViewer *viewer) {
    static_assert((sizeof(T) % sizeof(void *)) == 0);
    if (unlikely((uintptr_t) viewer->m_cur_data_rw_ptr + sizeof(T) > (uintptr_t) viewer->m_cur_data_end_ptr)) {
        Py_ssize_t cur_rw_index = ++viewer->m_data_cur_rw_index;
        assert(cur_rw_index < viewer->m_data_buffer_count);
        viewer->m_cur_data_rw_ptr = viewer->m_data_buffers[cur_rw_index];
        Py_ssize_t length = get_data_buffer_length(cur_rw_index);
        viewer->m_cur_data_end_ptr = viewer->m_cur_data_rw_ptr + length;
        // DONT modify m_data_buffer_count
    }
    assert((uintptr_t) viewer->m_cur_data_rw_ptr + sizeof(T) <= (uintptr_t) viewer->m_cur_data_end_ptr);
    T ret = *(T *) viewer->m_cur_data_rw_ptr;
    viewer->m_cur_data_rw_ptr += sizeof(T) / sizeof(void *);
    return ret;
}

template<typename T>
force_inline T *read_view_data_buffer(ObjViewer *viewer) {
    static_assert((sizeof(T) % sizeof(void *)) == 0);
    if (unlikely((uintptr_t) viewer->m_cur_data_rw_ptr + sizeof(T) > (uintptr_t) viewer->m_cur_data_end_ptr)) {
        Py_ssize_t cur_rw_index = ++viewer->m_data_cur_rw_index;
        assert(cur_rw_index < viewer->m_data_buffer_count);
        viewer->m_cur_data_rw_ptr = viewer->m_data_buffers[cur_rw_index];
        Py_ssize_t length = get_data_buffer_length(cur_rw_index);
        viewer->m_cur_data_end_ptr = viewer->m_cur_data_rw_ptr + length;
        // DONT modify m_data_buffer_count
    }
    assert((uintptr_t) viewer->m_cur_data_rw_ptr + sizeof(T) <= (uintptr_t) viewer->m_cur_data_end_ptr);
    T *ret = (T *) viewer->m_cur_data_rw_ptr;
    viewer->m_cur_data_rw_ptr += sizeof(T) / sizeof(void *);
    return ret;
}

/*Viewer Utils*/

enum class IndentSetting {
    NONE = 0,
    INDENT_2 = 2,
    INDENT_4 = 4,
};

force_inline constexpr bool need_indent(IndentSetting indent_setting) {
    return indent_setting != IndentSetting::NONE;
}

force_inline constexpr Py_ssize_t _get_indent_space_count(Py_ssize_t cur_depth, IndentSetting indent_setting) {
    assert(cur_depth >= 0);
    return cur_depth * (Py_ssize_t) indent_setting;
}

force_inline Py_ssize_t get_indent_space_count(ObjViewer *viewer, IndentSetting indent_setting) {
    return _get_indent_space_count(viewer->m_cur_depth, indent_setting);
}

force_inline UCSKind read_ucs_kind_from_view(ObjViewer *viewer) {
    return viewer->m_unicode_kind;
}

force_inline Py_ssize_t read_size_from_view(ObjViewer *viewer) {
    return viewer->m_unicode_count;
}

force_inline Py_UCS4 get_max_char_from_ucs_kind(UCSKind kind, bool is_all_ascii) {
    switch (kind) {
        case UCSKind::UCS1: {
            return is_all_ascii ? 0x7f : 0xff;
        }
        case UCSKind::UCS2: {
            return 0xffff;
        }
        case UCSKind::UCS4: {
            return 0x10ffff;
        }
        default: {
            Py_UNREACHABLE();
            return 0;
        }
    }
}

force_inline bool read_is_unicode_escaped(ObjViewer *viewer) {
    return (bool) viewer->m_view_payload;
}

force_inline usize read_number_written_size(ObjViewer *viewer) {
    return (usize) viewer->m_view_payload;
}

template<UCSKind _Kind, usize _BitSize>
force_inline void _do_simd_repeat_copy(UCSType_t<_Kind> *dst, UCSType_t<_Kind> *const end_ptr, const void *aligned_src, Py_ssize_t need_copy_count) {
    using uu = UCSType_t<_Kind>;
    static_assert(_BitSize == 512 || _BitSize == 256 || _BitSize == 128);
    assert(end_ptr >= dst);

    constexpr usize _U8CopySize = _BitSize / 8;
    constexpr usize _CopyOnce = _U8CopySize / sizeof(uu);

    const usize can_copy_times_max = ((uintptr_t) end_ptr - (uintptr_t) dst) / _U8CopySize;
    const usize want_copy_times = size_align_up((usize) need_copy_count * sizeof(uu), _U8CopySize) / _U8CopySize;
    usize do_copy_times = std::min(want_copy_times, can_copy_times_max);

    assert(do_copy_times >= 0);
    if (((uintptr_t) dst % _U8CopySize) != 0) {
        for (usize i = 0; i < do_copy_times; ++i) {
            if constexpr (_BitSize == 512) {
                _mm512_storeu_si512((void *) dst, _mm512_load_si512(aligned_src));
            } else if constexpr (_BitSize == 256) {
                _mm256_storeu_si256((__m256i_u *) dst, _mm256_load_si256((const __m256i *) aligned_src));
            } else {
                _mm_storeu_si128((__m128i_u *) dst, _mm_load_si128((const __m128i *) aligned_src));
            }
            dst += _CopyOnce;
        }
    } else {
        for (usize i = 0; i < do_copy_times; ++i) {
            if constexpr (_BitSize == 512) {
                _mm512_store_si512((void *) dst, _mm512_load_si512(aligned_src));
            } else if constexpr (_BitSize == 256) {
                _mm256_store_si256((__m256i_u *) dst, _mm256_load_si256((const __m256i *) aligned_src));
            } else {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Walign-mismatch"
                _mm_store_si128((__m128i_u *) dst, _mm_load_si128((const __m128i *) aligned_src));
#pragma GCC diagnostic pop
            }
            dst += _CopyOnce;
        }
    }
    need_copy_count -= (Py_ssize_t) do_copy_times * (Py_ssize_t) _CopyOnce;
    if (unlikely(need_copy_count > 0)) {
        assert(need_copy_count < (Py_ssize_t) _CopyOnce);
        assert(end_ptr - dst < _CopyOnce);
        memcpy((void *) dst, aligned_src, (size_t) need_copy_count * sizeof(uu));
    }
}

template<IndentSetting _IndentSetting, UCSKind _Kind, X86SIMDLevel _SIMDLevel, bool _IsForceIndent = false>
void write_newline_and_indent_from_view(ObjViewer *viewer, UCSType_t<_Kind> *&write_ptr, UCSType_t<_Kind> *const end_ptr) {
    using uu = UCSType_t<_Kind>;

    if constexpr (!need_indent(_IndentSetting)) {
        return;
    }
    if constexpr (!_IsForceIndent) {
        if (viewer->m_is_in_obj) return;
    }

    Py_ssize_t indent_space_count = get_indent_space_count(viewer, _IndentSetting);

    uu *dst = write_ptr;
    write_ptr += indent_space_count + 1;
    assert(write_ptr <= end_ptr);
    // begin write / copy
    *dst++ = (uu) '\n';

    constexpr uu _FillVal = (uu) ' ';
    constexpr usize _Copy512Once = 512 / 8 / sizeof(uu);
    constexpr usize _Copy256Once = 256 / 8 / sizeof(uu);
    constexpr usize _Copy128Once = 128 / 8 / sizeof(uu);
    constexpr yyjson_align(512 / 8) std::array<uu, _Copy512Once> _Template = create_array<uu, _Copy512Once, _FillVal>();
    //
    if constexpr (check_support_512_copy(_SIMDLevel)) {
        _do_simd_repeat_copy<_Kind, 512>(dst, end_ptr, _Template.data(), indent_space_count);
    } else if constexpr (check_support_256_copy(_SIMDLevel)) {
        _do_simd_repeat_copy<_Kind, 256>(dst, end_ptr, _Template.data(), indent_space_count);
    } else {
        _do_simd_repeat_copy<_Kind, 128>(dst, end_ptr, _Template.data(), indent_space_count);
    }
}

template<bool _IsWriteObj, UCSKind _Kind>
force_inline void write_empty_ctn(ObjViewer *viewer, UCSType_t<_Kind> *&write_ptr, UCSType_t<_Kind> *const end_ptr) {
    using uu = UCSType_t<_Kind>;
    constexpr uu _Template[3] = {
            (uu) ('[' | ((u8) _IsWriteObj << 5)),
            (uu) (']' | ((u8) _IsWriteObj << 5)),
            (uu) ',',
    };
    if (unlikely(write_ptr + 3 > end_ptr)) {
        assert(write_ptr + 2 == end_ptr);
        memcpy((void *) write_ptr, (const void *) &_Template[0], 2 * sizeof(uu));
        write_ptr = end_ptr;
        return;
    }
    memcpy((void *) write_ptr, (const void *) &_Template[0], sizeof(_Template));
    write_ptr += 3;
}

force_inline void view_increase_indent(ObjViewer *viewer, bool is_obj) {
    viewer->obj_stack[viewer->m_cur_depth++] = viewer->m_is_in_obj ? 1 : 0;
    viewer->m_is_in_obj = is_obj;
    assert(viewer->m_cur_depth);
}

force_inline void view_decrease_indent(ObjViewer *viewer) {
    assert(viewer->m_cur_depth);
    viewer->m_cur_depth--;
    viewer->m_is_in_obj = viewer->obj_stack[viewer->m_cur_depth];
}

template<bool _IsWriteObj, UCSKind _Kind>
force_inline void write_ctn_begin(ObjViewer *viewer, UCSType_t<_Kind> *&write_ptr, UCSType_t<_Kind> *const end_ptr) {
    using uu = UCSType_t<_Kind>;
    *write_ptr++ = (uu) '[' | ((uu) _IsWriteObj << 5);
    assert(write_ptr <= end_ptr);
}

template<bool _IsWriteObj, UCSKind _Kind>
force_inline void write_ctn_end(ObjViewer *viewer, UCSType_t<_Kind> *&write_ptr, UCSType_t<_Kind> *const end_ptr) {
    using uu = UCSType_t<_Kind>;

    *write_ptr++ = (uu) ']' | ((uu) _IsWriteObj << 5);
    if (unlikely(write_ptr >= end_ptr)) return;
    *write_ptr++ = (uu) ',';
}

template<UCSKind _Kind, X86SIMDLevel _SIMDLevel>
void copy_raw_unicode_with_escape(ObjViewer *viewer, PyObject *unicode, UCSType_t<_Kind> *&write_ptr, UCSType_t<_Kind> *const end_ptr, Py_ssize_t escape_len) {
    using udst = UCSType_t<_Kind>;
    udst *const start_write_ptr = write_ptr;

    UCSKind src_kind = (UCSKind) PyUnicode_KIND(unicode);
    Py_ssize_t len = PyUnicode_GET_LENGTH(unicode);

    assert(_Kind >= src_kind);
    assert(len <= escape_len);
    assert(len >= 0);

    if constexpr (_Kind == UCSKind::UCS1) {
        assert(src_kind == UCSKind::UCS1);
        copy_ucs<UCSKind::UCS1, _Kind, _SIMDLevel>((u8 *) PyUnicode_DATA(unicode), len, write_ptr, end_ptr, escape_len);
        return;
    } else if constexpr (_Kind == UCSKind::UCS2) {
        switch (src_kind) {
            case UCSKind::UCS1: {
                copy_ucs<UCSKind::UCS1, _Kind, _SIMDLevel>((u8 *) PyUnicode_DATA(unicode), len, write_ptr, end_ptr, escape_len);
                break;
            }
            case UCSKind::UCS2: {
                copy_ucs<UCSKind::UCS2, _Kind, _SIMDLevel>((u16 *) PyUnicode_DATA(unicode), len, write_ptr, end_ptr, escape_len);
                break;
            }
            default: {
                Py_UNREACHABLE();
                assert(false);
            }
        }
    } else {
        switch (src_kind) {
            case UCSKind::UCS1: {
                copy_ucs<UCSKind::UCS1, _Kind, _SIMDLevel>((u8 *) PyUnicode_DATA(unicode), len, write_ptr, end_ptr, escape_len);
                break;
            }
            case UCSKind::UCS2: {
                copy_ucs<UCSKind::UCS2, _Kind, _SIMDLevel>((u16 *) PyUnicode_DATA(unicode), len, write_ptr, end_ptr, escape_len);
                break;
            }
            case UCSKind::UCS4: {
                copy_ucs<UCSKind::UCS4, _Kind, _SIMDLevel>((u32 *) PyUnicode_DATA(unicode), len, write_ptr, end_ptr, escape_len);
                break;
            }
            default: {
                Py_UNREACHABLE();
                assert(false);
            }
        }
    }


    assert(write_ptr - start_write_ptr > len);
    assert(write_ptr <= end_ptr);
}

template<UCSKind _Kind, X86SIMDLevel _SIMDLevel>
void copy_raw_unicode(ObjViewer *viewer, PyObject *unicode, UCSType_t<_Kind> *&write_ptr, UCSType_t<_Kind> *const end_ptr) {
    UCSKind src_kind = (UCSKind) PyUnicode_KIND(unicode);
    Py_ssize_t len = PyUnicode_GET_LENGTH(unicode);
    if constexpr (_Kind == UCSKind::UCS1) {
        assert(src_kind == UCSKind::UCS1);
        cpy_fast<_SIMDLevel>((void *) write_ptr, (void *) PyUnicode_DATA(unicode), len, (Py_ssize_t) end_ptr - (Py_ssize_t) write_ptr);
        write_ptr += len;
    } else if constexpr (_Kind == UCSKind::UCS2) {
        if (src_kind == UCSKind::UCS1) {
            copy_elevate_nocheck<UCSKind::UCS1, UCSKind::UCS2, _SIMDLevel>(write_ptr, (u8 *) PyUnicode_DATA(unicode), len, end_ptr - write_ptr);
        } else {
            assert(src_kind == UCSKind::UCS2);
            cpy_fast<_SIMDLevel>((void *) write_ptr, (void *) PyUnicode_DATA(unicode), len * 2, (Py_ssize_t) end_ptr - (Py_ssize_t) write_ptr);
            write_ptr += len;
        }
    } else {
        static_assert(_Kind == UCSKind::UCS4);
        if (src_kind == UCSKind::UCS1) {
            copy_elevate_nocheck<UCSKind::UCS1, UCSKind::UCS4, _SIMDLevel>(write_ptr, (u8 *) PyUnicode_DATA(unicode), len, end_ptr - write_ptr);
        } else if (src_kind == UCSKind::UCS2) {
            copy_elevate_nocheck<UCSKind::UCS2, UCSKind::UCS4, _SIMDLevel>(write_ptr, (u16 *) PyUnicode_DATA(unicode), len, end_ptr - write_ptr);
        } else {
            assert(src_kind == UCSKind::UCS4);
            cpy_fast<_SIMDLevel>((void *) write_ptr, (void *) PyUnicode_DATA(unicode), len * 4, (Py_ssize_t) end_ptr - (Py_ssize_t) write_ptr);
            write_ptr += len;
        }
    }
}

template<UCSKind _Kind, X86SIMDLevel _SIMDLevel>
Py_ssize_t scan_unicode_escaped_len_impl(UCSType_t<_Kind> *unicode_data, Py_ssize_t unicode_len) {
    using uu = UCSType_t<_Kind>;
    Py_ssize_t escaped_len = 0;

    if constexpr (_SIMDLevel >= X86SIMDLevel::AVX512) {
        // run 512 bits once
        constexpr usize _Count512Once = 512 / 8 / sizeof(uu);
        Py_ssize_t temp_len = unicode_len;
        while (temp_len >= _Count512Once) {
            __m512i z = _mm512_loadu_si512((const void *) unicode_data);
            if constexpr (_Kind == UCSKind::UCS1) {
                escaped_len += count_escape_u8_512_avx512(z);
            } else if constexpr (_Kind == UCSKind::UCS2) {
                escaped_len += count_escape_u16_512_avx512(z);
            } else {
                escaped_len += count_escape_u32_512_avx512(z);
            }
            temp_len -= _Count512Once;
            unicode_data += _Count512Once;
        }
        if (temp_len) {
            __m512i z = _mm512_loadu_si512((const void *) unicode_data);
            if constexpr (_Kind == UCSKind::UCS1) {
                escaped_len += count_escape_tail_u8_512_avx512(z, (usize) temp_len);
            } else if constexpr (_Kind == UCSKind::UCS2) {
                escaped_len += count_escape_tail_u16_512_avx512(z, (usize) temp_len);
            } else {
                escaped_len += count_escape_tail_u32_512_avx512(z, (usize) temp_len);
            }
        }
    } else if constexpr (_SIMDLevel >= X86SIMDLevel::AVX2) {
        // run 256 bits once
        constexpr usize _Count256Once = 256 / 8 / sizeof(uu);
        Py_ssize_t temp_len = unicode_len;
        while (temp_len >= _Count256Once) {
            __m256i y = _mm256_loadu_si256((const __m256i_u *) unicode_data);
            if constexpr (_Kind == UCSKind::UCS1) {
                escaped_len += count_escape_u8_256_avx2(y);
            } else if constexpr (_Kind == UCSKind::UCS2) {
                escaped_len += count_escape_u16_256_avx2(y);
            } else {
                escaped_len += count_escape_u32_256_avx2(y);
            }
            temp_len -= _Count256Once;
            unicode_data += _Count256Once;
        }
        if (temp_len) {
            __m256i y = _mm256_loadu_si256((const __m256i_u *) unicode_data);
            if constexpr (_Kind == UCSKind::UCS1) {
                escaped_len += count_escape_tail_u8_256_avx2(y, (usize) temp_len);
            } else if constexpr (_Kind == UCSKind::UCS2) {
                escaped_len += count_escape_tail_u16_256_avx2(y, (usize) temp_len);
            } else {
                escaped_len += count_escape_tail_u32_256_avx2(y, (usize) temp_len);
            }
        }
    } else if constexpr (_SIMDLevel >= X86SIMDLevel::SSE4) {
        // run 128 bits once
        constexpr usize _Count128Once = 128 / 8 / sizeof(uu);
        Py_ssize_t temp_len = unicode_len;
        while (temp_len >= _Count128Once) {
            __m128i x = _mm_lddqu_si128((const __m128i_u *) unicode_data);
            if constexpr (_Kind == UCSKind::UCS1) {
                escaped_len += count_escape_u8_128_sse4_2(x);
            } else if constexpr (_Kind == UCSKind::UCS2) {
                escaped_len += count_escape_u16_128_sse4_2(x);
            } else {
                escaped_len += count_escape_u32_128_sse4_2(x);
            }
            temp_len -= _Count128Once;
            unicode_data += _Count128Once;
        }
        if (temp_len) {
            __m128i x = _mm_lddqu_si128((const __m128i_u *) unicode_data);
            if constexpr (_Kind == UCSKind::UCS1) {
                escaped_len += count_escape_tail_u8_128_sse4_2(x, (usize) temp_len);
            } else if constexpr (_Kind == UCSKind::UCS2) {
                escaped_len += count_escape_tail_u16_128_sse4_2(x, (usize) temp_len);
            } else {
                escaped_len += count_escape_tail_u32_128_sse4_2(x, (usize) temp_len);
            }
        }
    } else {
        // run 128 bits once
        constexpr usize _Count128Once = 128 / 8 / sizeof(uu);
        Py_ssize_t temp_len = unicode_len;
        while (temp_len >= _Count128Once) {
            __m128i x = _mm_loadu_si128((const __m128i_u *) unicode_data);
            if constexpr (_Kind == UCSKind::UCS1) {
                escaped_len += count_escape_u8_128_sse2(x);
            } else if constexpr (_Kind == UCSKind::UCS2) {
                escaped_len += count_escape_u16_128_sse2(x);
            } else {
                escaped_len += count_escape_u32_128_sse2(x);
            }
            temp_len -= _Count128Once;
            unicode_data += _Count128Once;
        }
        if (temp_len) {
            __m128i x = _mm_loadu_si128((const __m128i_u *) unicode_data);
            if constexpr (_Kind == UCSKind::UCS1) {
                escaped_len += count_escape_tail_u8_128_sse2(x, (usize) temp_len);
            } else if constexpr (_Kind == UCSKind::UCS2) {
                escaped_len += count_escape_tail_u16_128_sse2(x, (usize) temp_len);
            } else {
                escaped_len += count_escape_tail_u32_128_sse2(x, (usize) temp_len);
            }
        }
    }

    return escaped_len;
}

template<X86SIMDLevel _SIMDLevel>
Py_ssize_t scan_unicode_escaped_len(PyObject *str) {
    void *ptr = (void *) PyUnicode_DATA(str);
    Py_ssize_t len = PyUnicode_GET_LENGTH(str);
    switch (PyUnicode_KIND(str)) {
        case PyUnicode_1BYTE_KIND: {
            return scan_unicode_escaped_len_impl<UCSKind::UCS1, _SIMDLevel>((u8 *) ptr, len);
        }
        case PyUnicode_2BYTE_KIND: {
            return scan_unicode_escaped_len_impl<UCSKind::UCS2, _SIMDLevel>((u16 *) ptr, len);
        }
        case PyUnicode_4BYTE_KIND: {
            return scan_unicode_escaped_len_impl<UCSKind::UCS4, _SIMDLevel>((u32 *) ptr, len);
        }
        default: {
            Py_UNREACHABLE();
            break;
        }
    }
    assert(false);
    return 0;
}

template<IndentSetting _IndentSetting, UCSKind _Kind, X86SIMDLevel _SIMDLevel>
force_inline void write_key(ObjViewer *viewer, PyObject *key, UCSType_t<_Kind> *&write_ptr, UCSType_t<_Kind> *const end_ptr) {
    using uu = UCSType_t<_Kind>;
    assert(PyUnicode_CheckExact(key));
    *write_ptr++ = (uu) '"';
    bool has_escape = read_is_unicode_escaped(viewer);
    uu *old_write_ptr = write_ptr;
    if (unlikely(has_escape)) {
        // additional escape data is written in the view data buffer
        Py_ssize_t escape_len = read_view_data<Py_ssize_t>(viewer);
        copy_raw_unicode_with_escape<_Kind, _SIMDLevel>(viewer, key, write_ptr, end_ptr, escape_len);
        assert(write_ptr - old_write_ptr == escape_len);
    } else {
        copy_raw_unicode<_Kind, _SIMDLevel>(viewer, key, write_ptr, end_ptr);
    }
    constexpr uu _Template[3] = {
            (uu) '"',
            (uu) ':',
            (uu) ' ',
    };
    constexpr usize copy_count = need_indent(_IndentSetting) ? 3 : 2;
    memcpy((void *) write_ptr, (const void *) &_Template[0], copy_count * sizeof(uu));
    write_ptr += copy_count;
    assert(write_ptr <= end_ptr);
}

template<IndentSetting _IndentSetting, UCSKind _Kind, X86SIMDLevel _SIMDLevel>
force_inline void write_str(ObjViewer *viewer, PyObject *str, UCSType_t<_Kind> *&write_ptr, UCSType_t<_Kind> *const end_ptr) {
    using uu = UCSType_t<_Kind>;
    assert(PyUnicode_CheckExact(str));
    *write_ptr++ = (uu) '"';
    bool has_escape = read_is_unicode_escaped(viewer);
    uu *old_write_ptr = write_ptr;
    if (unlikely(has_escape)) {
        // additional escape data is written in the view data buffer
        Py_ssize_t escape_len = read_view_data<Py_ssize_t>(viewer);
        copy_raw_unicode_with_escape<_Kind, _SIMDLevel>(viewer, str, write_ptr, end_ptr, escape_len);
        assert(write_ptr - old_write_ptr == escape_len);
    } else {
        copy_raw_unicode<_Kind, _SIMDLevel>(viewer, str, write_ptr, end_ptr);
    }
    constexpr uu _Template[2] = {
            (uu) '"',
            (uu) ',',
    };
    if (unlikely(end_ptr - write_ptr < 2)) {
        *write_ptr++ = (uu) '"';
        assert(write_ptr == end_ptr);
        return;
    }
    memcpy((void *) write_ptr, (const void *) &_Template[0], 2 * sizeof(uu));
    write_ptr += 2;
    assert(write_ptr <= end_ptr);
}

template<UCSKind _Kind>
force_inline void write_zero(ObjViewer *viewer, UCSType_t<_Kind> *&write_ptr, UCSType_t<_Kind> *const end_ptr) {
    using uu = UCSType_t<_Kind>;
    *write_ptr++ = (uu) '0';
    if (unlikely(write_ptr >= end_ptr)) {
        assert(write_ptr == end_ptr);
        return;
    }
    *write_ptr++ = (uu) ',';
}

template<UCSKind _Kind>
force_inline void write_true(ObjViewer *viewer, UCSType_t<_Kind> *&write_ptr, UCSType_t<_Kind> *const end_ptr) {
    using uu = UCSType_t<_Kind>;
    constexpr uu _Template[6] = {
            (uu) 't',
            (uu) 'r',
            (uu) 'u',
            (uu) 'e',
            (uu) ',',
    };
    assert(end_ptr >= write_ptr + 5);
    if (unlikely(write_ptr + 5 >= end_ptr)) {
        memcpy((void *) write_ptr, (const void *) &_Template[0], 5 * sizeof(uu));
        write_ptr = end_ptr;
        return;
    }
    memcpy((void *) write_ptr, (const void *) &_Template[0], sizeof(_Template));
    write_ptr += 5;
}

template<UCSKind _Kind>
force_inline void write_false(ObjViewer *viewer, UCSType_t<_Kind> *&write_ptr, UCSType_t<_Kind> *const end_ptr) {
    using uu = UCSType_t<_Kind>;
    constexpr uu _Template[6] = {
            (uu) 'f',
            (uu) 'a',
            (uu) 'l',
            (uu) 's',
            (uu) 'e',
            (uu) ',',
    };
    assert(end_ptr >= write_ptr + 6);
    memcpy((void *) write_ptr, (const void *) &_Template[0], sizeof(_Template));
    write_ptr += 6;
}

template<UCSKind _Kind>
force_inline void write_null(ObjViewer *viewer, UCSType_t<_Kind> *&write_ptr, UCSType_t<_Kind> *const end_ptr) {
    using uu = UCSType_t<_Kind>;
    constexpr uu _Template[6] = {
            (uu) 'n',
            (uu) 'u',
            (uu) 'l',
            (uu) 'l',
            (uu) ',',
    };
    assert(end_ptr >= write_ptr + 5);
    if (unlikely(write_ptr + 5 >= end_ptr)) {
        memcpy((void *) write_ptr, (const void *) &_Template[0], 5 * sizeof(uu));
        write_ptr = end_ptr;
        return;
    }
    memcpy((void *) write_ptr, (const void *) &_Template[0], sizeof(_Template));
    write_ptr += 5;
}

template<UCSKind _Kind, X86SIMDLevel _SIMDLevel>
force_inline void write_number(ObjViewer *viewer, u8 *buffer, usize buffer_len, UCSType_t<_Kind> *&write_ptr, UCSType_t<_Kind> *const end_ptr) {
    if constexpr (_Kind > UCSKind ::UCS1) {
        copy_elevate_nocheck<UCSKind::UCS1, _Kind, _SIMDLevel>(write_ptr, buffer, buffer_len, end_ptr - write_ptr);
    } else {
        // copy
        cpy_fast<_SIMDLevel>((void *) write_ptr, (void *) buffer, buffer_len, (Py_ssize_t) end_ptr - (Py_ssize_t) write_ptr);
        write_ptr += buffer_len;
    }
    *write_ptr++ = ',';
}


enum EncodeValJumpFlag {
    JumpFlag_Default,
    JumpFlag_ArrValBegin,
    JumpFlag_DictPairBegin,
    JumpFlag_TupleValBegin,
    JumpFlag_Fail,
};


enum ViewObjType {
    ViewObjType_Done,
    ViewObjType_EmptyObj,
    ViewObjType_EmptyArr,
    ViewObjType_ObjBegin,
    ViewObjType_ArrBegin,
    ViewObjType_ObjEnd,
    ViewObjType_ArrEnd,
    ViewObjType_Key,
    ViewObjType_Str,
    ViewObjType_Zero,
    ViewObjType_Number,
    ViewObjType_True,
    ViewObjType_False,
    ViewObjType_Null,
    ViewObjType_PosNaN,
    ViewObjType_NegNaN,
    ViewObjType_PosInf,
    ViewObjType_NegInf,
};


#define JUMP_BY_FLAG(_flag)                \
    do {                                   \
        switch ((_flag)) {                 \
            case JumpFlag_Default: {       \
                break;                     \
            }                              \
            case JumpFlag_ArrValBegin: {   \
                goto arr_val_begin;        \
            }                              \
            case JumpFlag_DictPairBegin: { \
                goto dict_pair_begin;      \
            }                              \
            case JumpFlag_TupleValBegin: { \
                goto tuple_val_begin;      \
            }                              \
            case JumpFlag_Fail: {          \
                goto fail;                 \
            }                              \
            default: {                     \
                Py_UNREACHABLE();          \
            }                              \
        }                                  \
    } while (0)

#define VIEW_PAYLOAD_BIT 16


template<IndentSetting _IndentSetting, bool _IsWriteObj, bool _IsInsideObj>
force_inline bool view_add_empty_ctn(ObjViewer *viewer, Py_ssize_t cur_nested_depth) {
    bool _c = write_obj_view(viewer, (u32) (_IsWriteObj ? ViewObjType_EmptyObj : ViewObjType_EmptyArr));
    if constexpr (need_indent(_IndentSetting) && !_IsInsideObj) {
        viewer->m_unicode_count += 1 + _get_indent_space_count(cur_nested_depth, _IndentSetting) + 3;
    } else {
        viewer->m_unicode_count += 3;
    }
    return _c;
}

template<IndentSetting _IndentSetting, bool _IsWriteObj, bool _IsInsideObj>
force_inline bool view_add_ctn_begin(ObjViewer *viewer, Py_ssize_t cur_nested_depth) {
    bool _c = write_obj_view(viewer, (u32) (_IsWriteObj ? ViewObjType_ObjBegin : ViewObjType_ArrBegin));
    if constexpr (need_indent(_IndentSetting) && !_IsInsideObj) {
        viewer->m_unicode_count += 1 + _get_indent_space_count(cur_nested_depth, _IndentSetting) + 1;
    } else {
        viewer->m_unicode_count += 1;
    }
    return _c;
}

force_inline void view_update_str_info(ObjViewer *viewer, PyObject *str) {
    assert(PyUnicode_CheckExact(str));
    viewer->m_unicode_kind = std::max<UCSKind>(viewer->m_unicode_kind, (UCSKind) PyUnicode_KIND(str));
    assert(viewer->m_unicode_kind <= UCSKind::UCS4);
    viewer->m_is_all_ascii = viewer->m_is_all_ascii && PyUnicode_IS_ASCII(str);
}

template<IndentSetting _IndentSetting, X86SIMDLevel _SIMDLevel>
force_inline bool view_add_key(ObjViewer *viewer, PyObject *str, Py_ssize_t cur_nested_depth) {
    assert(PyUnicode_CheckExact(str));
    Py_ssize_t escaped_len = scan_unicode_escaped_len<_SIMDLevel>(str);
    bool has_escape = escaped_len > PyUnicode_GET_LENGTH(str);
    u32 view = (u32) ViewObjType_Key | ((u32) has_escape << VIEW_PAYLOAD_BIT);
    bool _c = write_obj_view(viewer, view) && write_view_data<PyObject *>(viewer, str);
    if (unlikely(has_escape)) {
        _c = _c && write_view_data<Py_ssize_t>(viewer, escaped_len);
    }
    if constexpr (need_indent(_IndentSetting)) {
        // indent and spaces, two quotes, one colon, one space, and unicode
        viewer->m_unicode_count += 1 + _get_indent_space_count(cur_nested_depth, _IndentSetting) + 4 + escaped_len;
    } else {
        // two quotes, one colon, and unicode
        viewer->m_unicode_count += 3 + escaped_len;
    }
    return _c;
}

template<IndentSetting _IndentSetting, X86SIMDLevel _SIMDLevel, bool _IsInsideObj>
force_inline bool view_add_str(ObjViewer *viewer, PyObject *str, Py_ssize_t cur_nested_depth) {
    assert(PyUnicode_CheckExact(str));
    Py_ssize_t escaped_len = scan_unicode_escaped_len<_SIMDLevel>(str);
    bool has_escape = escaped_len > PyUnicode_GET_LENGTH(str);
    u32 view = (u32) ViewObjType_Str | ((u32) has_escape << VIEW_PAYLOAD_BIT);
    bool _c = write_obj_view(viewer, view) && write_view_data<PyObject *>(viewer, str);
    if (unlikely(has_escape)) {
        _c = _c && write_view_data<Py_ssize_t>(viewer, escaped_len);
    }
    if constexpr (need_indent(_IndentSetting) && !_IsInsideObj) {
        viewer->m_unicode_count += 1 + _get_indent_space_count(cur_nested_depth, _IndentSetting) + 3 + escaped_len;
    } else {
        // two quotes, unicode, and one comma
        viewer->m_unicode_count += 3 + escaped_len;
    }
    return _c;
}

template<IndentSetting _IndentSetting, bool _IsWriteObj>
force_inline bool view_add_ctn_end(ObjViewer *viewer, Py_ssize_t cur_nested_depth) {
    bool _c = write_obj_view(viewer, (u32) (_IsWriteObj ? ViewObjType_ObjEnd : ViewObjType_ArrEnd));
    if constexpr (need_indent(_IndentSetting)) {
        viewer->m_unicode_count += 1 + _get_indent_space_count(cur_nested_depth, _IndentSetting);
    }
    viewer->m_unicode_count += 1;
    return _c;
}

template<IndentSetting _IndentSetting, bool _IsInsideObj>
force_inline bool view_add_pylong(ObjViewer *viewer, PyObject *val, Py_ssize_t cur_nested_depth) {
    assert(PyLong_CheckExact(val));
    if (pylong_is_zero(val)) {
        if (unlikely(!write_obj_view(viewer, (u32) ViewObjType_Zero))) {
            return false;
        }
        if constexpr (need_indent(_IndentSetting) && !_IsInsideObj) {
            // indent and spaces, '0', and one comma
            viewer->m_unicode_count += 1 + _get_indent_space_count(cur_nested_depth, _IndentSetting) + 2;
        } else {
            // '0', and one comma
            viewer->m_unicode_count += 2;
        }
    } else {
        u8 *const buffer = (u8 *) reserve_view_data_buffer<u8[32]>(viewer);
        u64 v;
        usize sign;
        if (pylong_is_unsigned(val)) {
            bool _c = pylong_value_unsigned(val, &v);
            if (unlikely(!buffer || !_c)) {
                return false;
            }
            sign = 0;
        } else {
            i64 v2;
            bool _c = pylong_value_signed(val, &v2);
            if (unlikely(!buffer || !_c)) {
                return false;
            }
            assert(v2 <= 0);
            v = -v2;
            sign = 1;
            *buffer = '-';
        }
        u8 *const write_end = write_u64(v, buffer + sign);
        usize written_len = (usize) (write_end - buffer);
        assert(written_len <= 32);
        u32 view = ViewObjType_Number | (written_len << VIEW_PAYLOAD_BIT);
        if (unlikely(!write_obj_view(viewer, view))) {
            return false;
        }
        usize view_ptr_retreat = (32 - written_len) / SIZEOF_VOID_P;
        viewer->m_cur_data_rw_ptr -= view_ptr_retreat;
        if constexpr (need_indent(_IndentSetting) && !_IsInsideObj) {
            // indent and spaces, number written, and one comma
            viewer->m_unicode_count += 1 + _get_indent_space_count(cur_nested_depth, _IndentSetting) + written_len + 1;
        } else {
            // number written, and one comma
            viewer->m_unicode_count += written_len + 1;
        }
    }
    return true;
}

template<IndentSetting _IndentSetting, bool _IsInsideObj>
force_inline bool view_add_pyfloat(ObjViewer *viewer, PyObject *val, Py_ssize_t cur_nested_depth) {
    double v = PyFloat_AS_DOUBLE(val);
    u8 *const buffer = (u8 *) reserve_view_data_buffer<u8[32]>(viewer);
    u64 *raw = (u64 *) &v;
    u8 *const write_end = write_f64_raw(buffer, *raw);
    usize written_len = (usize) (write_end - buffer);
    assert(written_len <= 32);
    u32 view = ViewObjType_Number | (written_len << VIEW_PAYLOAD_BIT);
    if (unlikely(!write_obj_view(viewer, view))) {
        return false;
    }
    usize view_ptr_retreat = (32 - written_len) / SIZEOF_VOID_P;
    viewer->m_cur_data_rw_ptr -= view_ptr_retreat;
    if constexpr (need_indent(_IndentSetting) && !_IsInsideObj) {
        // indent and spaces, number written, and one comma
        viewer->m_unicode_count += 1 + _get_indent_space_count(cur_nested_depth, _IndentSetting) + written_len + 1;
    } else {
        // number written, and one comma
        viewer->m_unicode_count += written_len + 1;
    }
    return true;
}

template<IndentSetting _IndentSetting, bool _IsInsideObj>
force_inline bool view_add_false(ObjViewer *viewer, Py_ssize_t cur_nested_depth) {
    if (unlikely(!write_obj_view(viewer, (u32) ViewObjType_False))) {
        return false;
    }
    if constexpr (need_indent(_IndentSetting) && !_IsInsideObj) {
        // indent and spaces, 'false', and one comma
        viewer->m_unicode_count += 1 + _get_indent_space_count(cur_nested_depth, _IndentSetting) + 6;
    } else {
        // 'false', and one comma
        viewer->m_unicode_count += 6;
    }
    return true;
}

template<IndentSetting _IndentSetting, bool _IsInsideObj>
force_inline bool view_add_true(ObjViewer *viewer, Py_ssize_t cur_nested_depth) {
    if (unlikely(!write_obj_view(viewer, (u32) ViewObjType_True))) {
        return false;
    }
    if constexpr (need_indent(_IndentSetting) && !_IsInsideObj) {
        // indent and spaces, 'true', and one comma
        viewer->m_unicode_count += 1 + _get_indent_space_count(cur_nested_depth, _IndentSetting) + 5;
    } else {
        // 'true', and one comma
        viewer->m_unicode_count += 5;
    }
    return true;
}

template<IndentSetting _IndentSetting, bool _IsInsideObj>
force_inline bool view_add_null(ObjViewer *viewer, Py_ssize_t cur_nested_depth) {
    if (unlikely(!write_obj_view(viewer, (u32) ViewObjType_Null))) {
        return false;
    }
    if constexpr (need_indent(_IndentSetting) && !_IsInsideObj) {
        // indent and spaces, 'null', and one comma
        viewer->m_unicode_count += 1 + _get_indent_space_count(cur_nested_depth, _IndentSetting) + 5;
    } else {
        // 'null', and one comma
        viewer->m_unicode_count += 5;
    }
    return true;
}

template<IndentSetting _IndentSetting, X86SIMDLevel _SIMDLevel, bool _IsInsideObj>
force_inline EncodeValJumpFlag view_process_val(
        PyObject *val,
        ObjViewer *obj_viewer,
        CtnType *const ctn_stack,
        Py_ssize_t &cur_nested_depth,
        PyObject *&cur_obj,
        Py_ssize_t &cur_pos,
        Py_ssize_t &cur_list_size) {
#define CTN_SIZE_GROW()                                                      \
    do {                                                                     \
        if (unlikely(cur_nested_depth == PYYJSON_ENCODE_MAX_RECURSION)) {    \
            PyErr_SetString(PyExc_ValueError, "Too many nested structures"); \
            return JumpFlag_Fail;                                            \
        }                                                                    \
    } while (0)
#define RETURN_JUMP_FAIL_ON_UNLIKELY_ERR(_condition)    \
    do {                                                \
        if (unlikely(_condition)) return JumpFlag_Fail; \
    } while (0)

    PyFastTypes fast_type = fast_type_check(val);

    switch (fast_type) {
        case PyFastTypes::T_Unicode: {
            view_update_str_info(obj_viewer, val);
            bool _c = view_add_str<_IndentSetting, _SIMDLevel, _IsInsideObj>(obj_viewer, val, cur_nested_depth);
            RETURN_JUMP_FAIL_ON_UNLIKELY_ERR(!_c);
            break;
        }
        case PyFastTypes::T_Long: {
            bool _c = view_add_pylong<_IndentSetting, _IsInsideObj>(obj_viewer, val, cur_nested_depth);
            RETURN_JUMP_FAIL_ON_UNLIKELY_ERR(!_c);
            break;
        }
        case PyFastTypes::T_False: {
            bool _c = view_add_false<_IndentSetting, _IsInsideObj>(obj_viewer, cur_nested_depth);
            RETURN_JUMP_FAIL_ON_UNLIKELY_ERR(!_c);
            break;
        }
        case PyFastTypes::T_True: {
            bool _c = view_add_true<_IndentSetting, _IsInsideObj>(obj_viewer, cur_nested_depth);
            RETURN_JUMP_FAIL_ON_UNLIKELY_ERR(!_c);
            break;
        }
        case PyFastTypes::T_None: {
            bool _c = view_add_null<_IndentSetting, _IsInsideObj>(obj_viewer, cur_nested_depth);
            RETURN_JUMP_FAIL_ON_UNLIKELY_ERR(!_c);
            break;
        }
        case PyFastTypes::T_Float: {
            bool _c = view_add_pyfloat<_IndentSetting, _IsInsideObj>(obj_viewer, val, cur_nested_depth);
            RETURN_JUMP_FAIL_ON_UNLIKELY_ERR(!_c);
            break;
        }
        case PyFastTypes::T_List: {
            Py_ssize_t this_list_size = PyList_GET_SIZE(val);
            bool _c;
            if (unlikely(this_list_size == 0)) {
                _c = view_add_empty_ctn<_IndentSetting, false, _IsInsideObj>(obj_viewer, cur_nested_depth);
                RETURN_JUMP_FAIL_ON_UNLIKELY_ERR(!_c);
            } else {
                _c = view_add_ctn_begin<_IndentSetting, false, _IsInsideObj>(obj_viewer, cur_nested_depth);
                RETURN_JUMP_FAIL_ON_UNLIKELY_ERR(!_c);
                CTN_SIZE_GROW();
                CtnType *cur_write_ctn = ctn_stack + (cur_nested_depth++);
                cur_write_ctn->ctn = cur_obj;
                cur_write_ctn->index = cur_pos;
                cur_obj = val;
                cur_pos = 0;
                cur_list_size = this_list_size;
                return JumpFlag_ArrValBegin;
            }
            break;
        }
        case PyFastTypes::T_Dict: {
            bool _c;
            if (unlikely(PyDict_GET_SIZE(val) == 0)) {
                _c = view_add_empty_ctn<_IndentSetting, true, _IsInsideObj>(obj_viewer, cur_nested_depth);
                RETURN_JUMP_FAIL_ON_UNLIKELY_ERR(!_c);
            } else {
                _c = view_add_ctn_begin<_IndentSetting, true, _IsInsideObj>(obj_viewer, cur_nested_depth);
                RETURN_JUMP_FAIL_ON_UNLIKELY_ERR(!_c);
                CTN_SIZE_GROW();
                CtnType *cur_write_ctn = _CtnStack + (cur_nested_depth++);
                cur_write_ctn->ctn = cur_obj;
                cur_write_ctn->index = cur_pos;
                cur_obj = val;
                cur_pos = 0;
                return JumpFlag_DictPairBegin;
            }
            break;
        }
        case PyFastTypes::T_Tuple: {
            Py_ssize_t this_list_size = PyTuple_Size(val);
            if (unlikely(this_list_size == 0)) {
                bool _c = view_add_empty_ctn<_IndentSetting, false, _IsInsideObj>(obj_viewer, cur_nested_depth);
                RETURN_JUMP_FAIL_ON_UNLIKELY_ERR(!_c);
            } else {
                bool _c = view_add_ctn_begin<_IndentSetting, false, _IsInsideObj>(obj_viewer, cur_nested_depth);
                RETURN_JUMP_FAIL_ON_UNLIKELY_ERR(!_c);
                CTN_SIZE_GROW();
                CtnType *cur_write_ctn = _CtnStack + (cur_nested_depth++);
                cur_write_ctn->ctn = cur_obj;
                cur_write_ctn->index = cur_pos;
                cur_obj = val;
                cur_pos = 0;
                cur_list_size = this_list_size;
                return JumpFlag_TupleValBegin;
            }
            break;
        }
        default: {
            PyErr_SetString(JSONEncodeError, "Unsupported type");
            return JumpFlag_Fail;
        }
    }

    return JumpFlag_Default;
#undef RETURN_JUMP_FAIL_ON_UNLIKELY_ERR
#undef CTN_SIZE_GROW
}

template<IndentSetting _IndentSetting, X86SIMDLevel _SIMDLevel>
force_inline bool
pyyjson_dumps_obj_to_view(PyObject *in_obj, ObjViewer *obj_viewer) {
#define GOTO_FAIL_ON_UNLIKELY_ERR(_condition) \
    do {                                      \
        if (unlikely(_condition)) goto fail;  \
    } while (0)
    // cache
    Py_ssize_t cur_list_size;
    PyObject *key, *val;
    PyObject *cur_obj = in_obj;
    Py_ssize_t cur_pos = 0;
    Py_ssize_t cur_nested_depth = 0;
    // alias thread local buffer
    CtnType *const ctn_stack = _CtnStack;

    if (PyDict_CheckExact(cur_obj)) {
        if (unlikely(PyDict_GET_SIZE(cur_obj) == 0)) {
            bool _c = view_add_empty_ctn<_IndentSetting, true, true>(obj_viewer, 0);
            assert(_c);
            goto success;
        }
        bool _c = view_add_ctn_begin<_IndentSetting, true, true>(obj_viewer, 0);
        assert(_c);
        assert(!cur_nested_depth);
        cur_nested_depth = 1;
        // NOTE: ctn_stack[0] is always invalid
        goto dict_pair_begin;
    } else if (PyList_CheckExact(cur_obj)) {
        cur_list_size = PyList_GET_SIZE(cur_obj);
        if (unlikely(cur_list_size == 0)) {
            bool _c = view_add_empty_ctn<_IndentSetting, false, true>(obj_viewer, 0);
            assert(_c);
            goto success;
        }
        bool _c = view_add_ctn_begin<_IndentSetting, false, true>(obj_viewer, 0);
        assert(_c);
        assert(!cur_nested_depth);
        cur_nested_depth = 1;
        // NOTE: ctn_stack[0] is always invalid
        goto arr_val_begin;
    } else {
        if (unlikely(!PyTuple_CheckExact(cur_obj))) {
            goto fail_ctntype;
        }
        cur_list_size = PyTuple_GET_SIZE(cur_obj);
        if (unlikely(cur_list_size == 0)) {
            bool _c = view_add_empty_ctn<_IndentSetting, false, true>(obj_viewer, 0);
            assert(_c);
            goto success;
        }
        bool _c = view_add_ctn_begin<_IndentSetting, false, true>(obj_viewer, 0);
        assert(_c);
        assert(!cur_nested_depth);
        cur_nested_depth = 1;
        goto tuple_val_begin;
    }

    Py_UNREACHABLE();

dict_pair_begin:;
    assert(PyDict_GET_SIZE(cur_obj) != 0);

    if (PyDict_Next(cur_obj, &cur_pos, &key, &val)) {
        if (unlikely(!PyUnicode_CheckExact(key))) {
            goto fail_keytype;
        }
        view_update_str_info(obj_viewer, key);
        bool _c = view_add_key<_IndentSetting, _SIMDLevel>(obj_viewer, key, cur_nested_depth);
        GOTO_FAIL_ON_UNLIKELY_ERR(!_c);
        //
        auto jump_flag = view_process_val<_IndentSetting, _SIMDLevel, true>(val,
                                                                            obj_viewer,
                                                                            ctn_stack,
                                                                            cur_nested_depth,
                                                                            cur_obj,
                                                                            cur_pos,
                                                                            cur_list_size);
        JUMP_BY_FLAG(jump_flag);
        goto dict_pair_begin;
    } else {
        // dict end
        assert(cur_nested_depth);
        CtnType *last_pos = ctn_stack + (--cur_nested_depth);

        bool _c = view_add_ctn_end<_IndentSetting, true>(obj_viewer, cur_nested_depth);
        GOTO_FAIL_ON_UNLIKELY_ERR(!_c);
        if (unlikely(cur_nested_depth == 0)) {
            goto success;
        }

        // update cur_obj and cur_pos
        cur_obj = last_pos->ctn;
        cur_pos = last_pos->index;

        if (PyDict_CheckExact(cur_obj)) {
            goto dict_pair_begin;
        } else if (PyList_CheckExact(cur_obj)) {
            cur_list_size = PyList_GET_SIZE(cur_obj);
            goto arr_val_begin;
        } else {
            assert(PyTuple_CheckExact(cur_obj));
            cur_list_size = PyTuple_GET_SIZE(cur_obj);
            goto tuple_val_begin;
        }
    }

    Py_UNREACHABLE();

arr_val_begin:;
    assert(cur_list_size != 0);

    if (cur_pos < cur_list_size) {
        val = PyList_GET_ITEM(cur_obj, cur_pos);
        cur_pos++;
        //
        auto jump_flag = view_process_val<_IndentSetting, _SIMDLevel, false>(val,
                                                                             obj_viewer,
                                                                             ctn_stack,
                                                                             cur_nested_depth,
                                                                             cur_obj,
                                                                             cur_pos,
                                                                             cur_list_size);
        JUMP_BY_FLAG(jump_flag);
        //
        goto arr_val_begin;
    } else {
        // list end
        assert(cur_nested_depth);
        CtnType *last_pos = ctn_stack + (--cur_nested_depth);

        bool _c = view_add_ctn_end<_IndentSetting, false>(obj_viewer, cur_nested_depth);
        GOTO_FAIL_ON_UNLIKELY_ERR(!_c);
        if (unlikely(cur_nested_depth == 0)) {
            goto success;
        }

        // update cur_obj and cur_pos
        cur_obj = last_pos->ctn;
        cur_pos = last_pos->index;

        if (PyDict_CheckExact(cur_obj)) {
            goto dict_pair_begin;
        } else if (PyList_CheckExact(cur_obj)) {
            cur_list_size = PyList_GET_SIZE(cur_obj);
            goto arr_val_begin;
        } else {
            assert(PyTuple_CheckExact(cur_obj));
            cur_list_size = PyTuple_GET_SIZE(cur_obj);
            goto tuple_val_begin;
        }
    }
    Py_UNREACHABLE();

tuple_val_begin:;
    assert(cur_list_size != 0);

    if (cur_pos < cur_list_size) {
        val = PyTuple_GET_ITEM(cur_obj, cur_pos);
        cur_pos++;
        //
        auto jump_flag = view_process_val<_IndentSetting, _SIMDLevel, false>(val,
                                                                             obj_viewer,
                                                                             ctn_stack,
                                                                             cur_nested_depth,
                                                                             cur_obj,
                                                                             cur_pos,
                                                                             cur_list_size);
        JUMP_BY_FLAG(jump_flag);
        //
        goto tuple_val_begin;
    } else {
        // list end
        assert(cur_nested_depth);
        CtnType *last_pos = ctn_stack + (--cur_nested_depth);

        bool _c = view_add_ctn_end<_IndentSetting, false>(obj_viewer, cur_nested_depth);
        GOTO_FAIL_ON_UNLIKELY_ERR(!_c);
        if (unlikely(cur_nested_depth == 0)) {
            goto success;
        }

        // update cur_obj and cur_pos
        cur_obj = last_pos->ctn;
        cur_pos = last_pos->index;

        if (PyDict_CheckExact(cur_obj)) {
            goto dict_pair_begin;
        } else if (PyList_CheckExact(cur_obj)) {
            cur_list_size = PyList_GET_SIZE(cur_obj);
            goto arr_val_begin;
        } else {
            assert(PyTuple_CheckExact(cur_obj));
            cur_list_size = PyTuple_GET_SIZE(cur_obj);
            goto tuple_val_begin;
        }
    }
    Py_UNREACHABLE();

success:;
    assert(cur_nested_depth == 0);
    // remove trailing comma
    obj_viewer->m_unicode_count--;
    // write an empty view obj
    if (unlikely(!write_obj_view(obj_viewer, (u32) ViewObjType_Done))) {
        goto fail;
    }
    return true;
fail:;

    // TODO
    return false;
fail_ctntype:;

    // TODO
    goto fail;
fail_keytype:;

    // TODO
    goto fail;
}


force_inline ViewObjType view_obj(ObjViewer *obj_viewer) {
    // TODO
    u32 obj_view = read_obj_view(obj_viewer);
    ViewObjType ret = (ViewObjType) (obj_view & 0xffff);
    obj_viewer->m_view_payload = obj_view >> VIEW_PAYLOAD_BIT;
    return ret;
}

template<IndentSetting _IndentSetting, X86SIMDLevel _SIMDLevel, UCSKind _Kind>
force_inline PyObject *pyyjson_dumps_view(ObjViewer *obj_viewer) {
    using uu = UCSType_t<_Kind>;
    // UCSKind ucs_kind = read_ucs_kind_from_view(obj_viewer);
    Py_ssize_t ucs_len = read_size_from_view(obj_viewer);
    PyObject *ret = My_PyUnicode_New<_Kind>(ucs_len, get_max_char_from_ucs_kind(_Kind, obj_viewer->m_is_all_ascii));
    if (unlikely(!ret)) return NULL;
    uu *write_ptr = (uu *) PyUnicode_DATA(ret);
    uu *const end_ptr = write_ptr + ucs_len;
    while (1) {
        ViewObjType view = view_obj(obj_viewer);
        switch (view) {
            case ViewObjType_Done: {
                goto success;
            }
            case ViewObjType_EmptyObj: {
                write_newline_and_indent_from_view<_IndentSetting, _Kind, _SIMDLevel>(obj_viewer, write_ptr, end_ptr);
                write_empty_ctn<true, _Kind>(obj_viewer, write_ptr, end_ptr);
                break;
            }
            case ViewObjType_EmptyArr: {
                write_newline_and_indent_from_view<_IndentSetting, _Kind, _SIMDLevel>(obj_viewer, write_ptr, end_ptr);
                write_empty_ctn<false, _Kind>(obj_viewer, write_ptr, end_ptr);
                break;
            }
            case ViewObjType_ObjBegin: {
                write_newline_and_indent_from_view<_IndentSetting, _Kind, _SIMDLevel>(obj_viewer, write_ptr, end_ptr);
                write_ctn_begin<true, _Kind>(obj_viewer, write_ptr, end_ptr);
                // then increase indent
                view_increase_indent(obj_viewer, true);
                break;
            }
            case ViewObjType_ArrBegin: {
                write_newline_and_indent_from_view<_IndentSetting, _Kind, _SIMDLevel>(obj_viewer, write_ptr, end_ptr);
                write_ctn_begin<false, _Kind>(obj_viewer, write_ptr, end_ptr);
                // then increase indent
                view_increase_indent(obj_viewer, false);
                break;
            }
            case ViewObjType_ObjEnd: {
                // need to erase last comma
                write_ptr--;
                assert(*write_ptr == ',');
                // decrease indent
                view_decrease_indent(obj_viewer);
                write_newline_and_indent_from_view<_IndentSetting, _Kind, _SIMDLevel, true>(obj_viewer, write_ptr, end_ptr);
                write_ctn_end<true, _Kind>(obj_viewer, write_ptr, end_ptr);
                break;
            }
            case ViewObjType_ArrEnd: {
                // need to erase last comma
                write_ptr--;
                assert(*write_ptr == ',');
                // decrease indent first
                view_decrease_indent(obj_viewer);
                write_newline_and_indent_from_view<_IndentSetting, _Kind, _SIMDLevel, true>(obj_viewer, write_ptr, end_ptr);
                write_ctn_end<false, _Kind>(obj_viewer, write_ptr, end_ptr);
                break;
            }
            case ViewObjType_Key: {
                write_newline_and_indent_from_view<_IndentSetting, _Kind, _SIMDLevel, true>(obj_viewer, write_ptr, end_ptr);
                PyObject *key = read_view_data<PyObject *>(obj_viewer);
                write_key<_IndentSetting, _Kind, _SIMDLevel>(obj_viewer, key, write_ptr, end_ptr);
                break;
            }
            case ViewObjType_Str: {
                write_newline_and_indent_from_view<_IndentSetting, _Kind, _SIMDLevel>(obj_viewer, write_ptr, end_ptr);
                PyObject *str = read_view_data<PyObject *>(obj_viewer);
                write_str<_IndentSetting, _Kind, _SIMDLevel>(obj_viewer, str, write_ptr, end_ptr);
                break;
            }
            case ViewObjType_Zero: {
                write_newline_and_indent_from_view<_IndentSetting, _Kind, _SIMDLevel>(obj_viewer, write_ptr, end_ptr);
                write_zero<_Kind>(obj_viewer, write_ptr, end_ptr);
                break;
            }
            case ViewObjType_Number: {
                usize payload_number_size = read_number_written_size(obj_viewer);
                assert(payload_number_size <= 32);
                write_newline_and_indent_from_view<_IndentSetting, _Kind, _SIMDLevel>(obj_viewer, write_ptr, end_ptr);
                u8 *buffer = (u8 *) read_view_data_buffer<u8[32]>(obj_viewer);
                usize view_ptr_retreat = (32 - payload_number_size) / SIZEOF_VOID_P;
                obj_viewer->m_cur_data_rw_ptr -= view_ptr_retreat;
                write_number<_Kind, _SIMDLevel>(obj_viewer, buffer, payload_number_size, write_ptr, end_ptr);
                break;
            }
            case ViewObjType_True: {
                write_newline_and_indent_from_view<_IndentSetting, _Kind, _SIMDLevel>(obj_viewer, write_ptr, end_ptr);
                write_true<_Kind>(obj_viewer, write_ptr, end_ptr);
                break;
            }
            case ViewObjType_False: {
                write_newline_and_indent_from_view<_IndentSetting, _Kind, _SIMDLevel>(obj_viewer, write_ptr, end_ptr);
                write_false<_Kind>(obj_viewer, write_ptr, end_ptr);
                break;
            }
            case ViewObjType_Null: {
                write_newline_and_indent_from_view<_IndentSetting, _Kind, _SIMDLevel>(obj_viewer, write_ptr, end_ptr);
                write_null<_Kind>(obj_viewer, write_ptr, end_ptr);
                break;
            }
            // case ViewObjType_PosNaN: {
            //     write_newline_and_indent_from_view<_IndentSetting, _Kind, _SIMDLevel>(obj_viewer, write_ptr, end_ptr);
            //     write_pos_nan(obj_viewer, write_ptr, end_ptr);
            //     break;
            // }
            // case ViewObjType_NegNaN: {
            //     write_newline_and_indent_from_view<_IndentSetting, _Kind, _SIMDLevel>(obj_viewer, write_ptr, end_ptr);
            //     write_neg_nan(obj_viewer, write_ptr, end_ptr);
            //     break;
            // }
            // case ViewObjType_PosInf: {
            //     write_newline_and_indent_from_view<_IndentSetting, _Kind, _SIMDLevel>(obj_viewer, write_ptr, end_ptr);
            //     write_pos_inf(obj_viewer, write_ptr, end_ptr);
            //     break;
            // }
            // case ViewObjType_NegInf: {
            //     write_newline_and_indent_from_view<_IndentSetting, _Kind, _SIMDLevel>(obj_viewer, write_ptr, end_ptr);
            //     write_neg_inf(obj_viewer, write_ptr, end_ptr);
            //     break;
            // }
            //... TODO
            default: {
                Py_UNREACHABLE();
            }
        }
    }

success:;
    assert(write_ptr == end_ptr);
    PyObject* true_ret = My_Realloc_Unicode(ret, unicode_real_u8_size<_Kind>(ucs_len, obj_viewer->m_is_all_ascii));
    if(unlikely(!true_ret)){
        Py_DECREF(ret);
        return NULL;
    }
    return true_ret;
}

force_noinline PyObject *pyyjson_dumps_obj(PyObject *in_obj, DumpOption *option) {
    ObjViewer obj_viewer;
    init_viewer(&obj_viewer);
    PyObject *ret = NULL;
    //
    bool success = pyyjson_dumps_obj_to_view<IndentSetting::INDENT_4, X86SIMDLevel::AVX2>(in_obj, &obj_viewer);
    UCSKind kind;

    if (unlikely(!success)) goto finalize;

    reset_view_for_read(&obj_viewer);

    kind = read_ucs_kind_from_view(&obj_viewer);
    switch (kind) {
        case UCSKind::UCS1: {
            ret = pyyjson_dumps_view<IndentSetting::INDENT_4, X86SIMDLevel::AVX2, UCSKind::UCS1>(&obj_viewer);
            break;
        }
        case UCSKind::UCS2: {
            ret = pyyjson_dumps_view<IndentSetting::INDENT_4, X86SIMDLevel::AVX2, UCSKind::UCS2>(&obj_viewer);
            break;
        }
        case UCSKind::UCS4: {
            ret = pyyjson_dumps_view<IndentSetting::INDENT_4, X86SIMDLevel::AVX2, UCSKind::UCS4>(&obj_viewer);
            break;
        }
        default: {
            Py_UNREACHABLE();
            break;
        }
    }

finalize:;
    clear_viewer(&obj_viewer);

    return ret;
}

#undef GOTO_TUPLE_VAL_BEGIN_WITH_SIZE
#undef GOTO_ARR_VAL_BEGIN_WITH_SIZE

extern "C" {
PyObject *pyyjson_Encode(PyObject *self, PyObject *args, PyObject *kwargs) {
    PyObject *obj;
    int option_digit = 0;
    static const char *kwlist[] = {"obj", "options", NULL};
    DumpOption options = {IndentLevel::NONE};
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O|i", (char **) kwlist, &obj, &option_digit)) {
        PyErr_SetString(PyExc_TypeError, "Invalid argument");
        return NULL;
    }
    if (option_digit & 1) {
        if (option_digit & 2) {
            PyErr_SetString(PyExc_ValueError, "Cannot mix indent options");
            return NULL;
        }
        options.indent_level = IndentLevel::INDENT_2;
    } else if (option_digit & 2) {
        options.indent_level = IndentLevel::INDENT_4;
    }

    PyObject *ret = pyyjson_dumps_obj(obj, &options);

    if (unlikely(!ret)) {
        if (!PyErr_Occurred()) {
            PyErr_SetString(JSONEncodeError, "Failed to decode JSON: unknown error");
        }
    }

    return ret;
}
}


#undef NEED_INDENT
#undef RETURN_ON_UNLIKELY_RESIZE_FAIL
#undef RETURN_ON_UNLIKELY_ERR
