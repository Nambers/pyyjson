#include "encode.hpp"
#include "encode_avx2.inl"
#include "encode_avx512.inl"
#include "encode_sse2.inl"
#include "encode_sse4.inl"
#include "pyyjson_config.h"
#include "yyjson.h"
#include <algorithm>
#include <array>
#include <stddef.h>


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

/*==============================================================================
 * Wrappers for `cvtepu`
 *============================================================================*/

template<UCSKind __from, UCSKind __to, X86SIMDLevel __simd_level>
force_inline void cvtepu_128(__m128i &u, UCSType_t<__from> *&src, UCSType_t<__to> *&dst) {
    if constexpr (__simd_level >= X86SIMDLevel::AVX512) {
        cvtepu_128_avx512<__from, __to>(u, src, dst);
    } else if constexpr (__simd_level >= X86SIMDLevel::AVX2) {
        cvtepu_128_avx2<__from, __to>(u, src, dst);
    } else if constexpr (__simd_level >= X86SIMDLevel::SSE4) {
        cvtepu_128_sse4_1<__from, __to>(u, src, dst);
    } else {
        static_assert(false, "No SIMD support");
    }
}

template<UCSKind __from, UCSKind __to, X86SIMDLevel __simd_level>
force_inline void cvtepu_256(__m256i &u, UCSType_t<__from> *&src, UCSType_t<__to> *&dst) {
    static_assert(__simd_level >= X86SIMDLevel::AVX512, "Only AVX512 supports 256-bit cvtepu");
    return cvtepu_256_avx512<__from, __to>(u, src, dst);
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

/*==============================================================================
 * Copy UCS impl
 *============================================================================*/

template<UCSKind __from, UCSKind __to>
force_inline bool copy_ucs_with_runtime_resize(const SrcInfo<__from> &src_info, UCSType_t<__to> *&dst, BufferInfo &buffer_info) {
    // alright, no simd support
    // TODO fix this
    using usrc = UCSType_t<__from>;
    usrc *src = src_info.src_start;
    usrc *const src_end = src + static_cast<std::ptrdiff_t>(src_info.src_size);

    constexpr std::ptrdiff_t _ProcessCountOnce = 512 / 8 / sizeof(usrc);
    while (src_end - src >= _ProcessCountOnce) {
        auto additional_needed_count = (src_end - src - _ProcessCountOnce) + _ProcessCountOnce * 6;
        auto required_len_u8 = buffer_info.get_required_len_u8<__to>(dst, additional_needed_count);

        RETURN_ON_UNLIKELY_RESIZE_FAIL(__to, dst, buffer_info, required_len_u8);

        bool _c = copy_escape_fixsize<__from, __to, 512>(src, dst);
        RETURN_ON_UNLIKELY_ERR(!_c);
    }
    const auto left_count = src_end - src;
    assert(left_count < _ProcessCountOnce);
    auto required_len_u8 = buffer_info.get_required_len_u8<__to>(dst, left_count * 6);

    RETURN_ON_UNLIKELY_RESIZE_FAIL(__to, dst, buffer_info, required_len_u8);

    return copy_escape_impl<__from, __to>(src, dst, left_count);
}

template<UCSKind __from, UCSKind __to>
force_inline bool copy_ucs_elevate_avx512(const SrcInfo<__from> &src_info, UCSType_t<__to> *&dst, BufferInfo &buffer_info) {
    COMMON_TYPE_DEF();
    UCSType_t<__from> *src = src_info.src_start;
    usrc *const src_end = src + static_cast<std::ptrdiff_t>(src_info.src_size);
    // process 256 bits once
    constexpr std::ptrdiff_t _ProcessCountOnce = 256 / 8 / sizeof(usrc);
    __m256i u;
    while (src_end - src >= _ProcessCountOnce) {
        u = _mm256_lddqu_si256((__m256i *) src); // vlddqu, AVX
        bool v = check_escape_256bits<__from, X86SIMDLevel::AVX512>(u);
        if (likely(v)) {
            if constexpr (__from == UCSKind::UCS1 && __to == UCSKind ::UCS4) {
                __m128i u128;
                u128 = _mm256_extracti128_si256(u, 0); // vextracti128, AVX2
                cvtepu_128<__from, __to, X86SIMDLevel::AVX512>(u128, src, dst);
                u128 = _mm256_extracti128_si256(u, 1); // vextracti128, AVX2
                cvtepu_128<__from, __to, X86SIMDLevel::AVX512>(u128, src, dst);
            } else {
                cvtepu_256<__from, __to, X86SIMDLevel::AVX512>(u, src, dst);
            }
        } else {
            // first, check and resize
            auto additional_needed_count = (src_end - src - _ProcessCountOnce) + _ProcessCountOnce * 6;
            auto required_len_u8 = buffer_info.get_required_len_u8<__to>(dst, additional_needed_count);
            RETURN_ON_UNLIKELY_RESIZE_FAIL(__to, dst, buffer_info, required_len_u8);
            // if (unlikely(required_len_u8 > buffer_info.m_size)) {
            //     if (unlikely(!buffer_info.resizer<__to>(dst, required_len_u8))) {
            //         return false;
            //     }
            // }
            // verify 128 bits twice
            __m128i u1;
            u1 = _mm256_extracti128_si256(u, 0); // vextracti128, AVX2
            if (check_escape_128bits<__from, X86SIMDLevel::AVX512>(u1)) {
                cvtepu_128<__from, __to, X86SIMDLevel::AVX512>(u1, src, dst);
            } else {
                bool _c = copy_escape_fixsize<__from, __to, 128>(src, dst);
                RETURN_ON_UNLIKELY_ERR(!_c);
            }
            u1 = _mm256_extracti128_si256(u, 1); // vextracti128, AVX2
            if (check_escape_128bits<__from, X86SIMDLevel::AVX512>(u1)) {
                cvtepu_128<__from, __to, X86SIMDLevel::AVX512>(u1, src, dst);
            } else {
                bool _c = copy_escape_fixsize<__from, __to, 128>(src, dst);
                RETURN_ON_UNLIKELY_ERR(!_c);
            }
        }
    }
    const auto left_count = src_end - src;
    if (!left_count) return true;

    assert(left_count < _ProcessCountOnce);
    if (left_count <= _ProcessCountOnce / 2) {
        // copy once, only check 128
        auto required_len_u8 = std::max<Py_ssize_t>(buffer_info.get_required_len_u8<__to>(dst, left_count * 6), 128 / 8 / _SizeRatio);
        RETURN_ON_UNLIKELY_RESIZE_FAIL(__to, dst, buffer_info, required_len_u8);
        __m128i u1;
        u1 = _mm_lddqu_si128((__m128i *) src); // lddqu, sse3
        bool v = check_escape_tail_128bits<__from, X86SIMDLevel::AVX512>(u1, left_count);
        if (likely(v)) {
            // requires 128 / 8 * _SizeRatio bytes
            // discard this pointer!
            udst *_temp_d = dst;
            cvtepu_128<__from, __to, X86SIMDLevel::AVX512>(u1, src, _temp_d);
            dst += left_count;
        } else {
            bool _c = copy_escape_impl<__from, __to>(src, dst, left_count);
            RETURN_ON_UNLIKELY_ERR(!_c);
        }
        return true;
    }
    // the first part requires _ProcessCountOnce / 2 * sizeof(dst) * 6 bytes
    // the second part requires max((left_count - _ProcessCountOnce / 2) * sizeof(dst) * 6, 128 / 8 * _SizeRatio) bytes
    assert(left_count > _ProcessCountOnce / 2);
    Py_ssize_t required_len_u8 = _ProcessCountOnce / 2 * sizeof(dst) * 6 + std::max<Py_ssize_t>(buffer_info.get_required_len_u8<__to>(dst, (left_count - _ProcessCountOnce / 2) * 6), 128 / 8 / _SizeRatio);
    Py_ssize_t required_len_u8_2 = std::max<Py_ssize_t>(
            buffer_info.get_required_len_u8<__to>(dst, left_count * 6), 256 / 8 / _SizeRatio);
    required_len_u8 = std::max(required_len_u8_2, required_len_u8);
    RETURN_ON_UNLIKELY_RESIZE_FAIL(__to, dst, buffer_info, required_len_u8);
    u = _mm256_lddqu_si256((__m256i *) src); // vlddqu, AVX
    bool v = check_escape_tail_256bits<__from, X86SIMDLevel::AVX512>(u, left_count);
    if (likely(v)) {
        // requires 256 / 8 * _SizeRatio bytes
        // discard this pointer!
        udst *_temp_d = dst;
        cvtepu_256<__from, __to, X86SIMDLevel::AVX512>(u, src, _temp_d);
        dst += left_count;
    } else {
        __m128i u1;
        u1 = _mm256_extracti128_si256(u, 0); // vextracti128, AVX2
        v = check_escape_128bits<__from, X86SIMDLevel::AVX512>(u1);
        // requires _ProcessCountOnce / 2 * sizeof(dst) * 6 bytes
        if (v) {
            cvtepu_128<__from, __to, X86SIMDLevel::AVX512>(u1, src, dst);
        } else {
            bool _c = copy_escape_fixsize<__from, __to, 128>(src, dst);
            RETURN_ON_UNLIKELY_ERR(!_c);
        }
        auto cur_left_count = left_count - _ProcessCountOnce / 2;
        if (cur_left_count) {
            u1 = _mm256_extracti128_si256(u, 1); // vextracti128, AVX2
            v = check_escape_tail_128bits<__from, X86SIMDLevel::AVX512>(u1, cur_left_count);
            if (v) {
                udst *_temp_d = dst;
                cvtepu_128<__from, __to, X86SIMDLevel::AVX512>(u1, src, _temp_d);
                dst += cur_left_count;
            } else {
                bool _c = copy_escape_fixsize<__from, __to, 128>(src, dst);
                RETURN_ON_UNLIKELY_ERR(!_c);
            }
        }
    }
    return true;

    // auto required_len_u8 = buffer_info.get_required_len_u8<__to>(dst, left_count * 6);
    // RETURN_ON_UNLIKELY_RESIZE_FAIL(__to, dst, buffer_info, required_len_u8);
    // // if (unlikely(required_len_u8 > buffer_info.m_size)) {
    // //     if (unlikely(!buffer_info.resizer<__to>(dst, required_len_u8))) {
    // //         return false;
    // //     }
    // // }
    // return copy_escape_impl<__from, __to>(src, dst, left_count);
}

template<UCSKind __from, UCSKind __to, X86SIMDLevel _SIMDLevel>
force_inline void copy_ucs_elevate_avx512_nocheck(UCSType_t<__to> *&write_ptr, UCSType_t<__from> *src, Py_ssize_t len, Py_ssize_t allow_write_count) {
    static_assert(__from < __to);
    static_assert(_SIMDLevel >= X86SIMDLevel::AVX512);
    assert(allow_write_count >= 0);
    COMMON_TYPE_DEF();
    constexpr usize _ReadWriteOnce = 512 / 8 / sizeof(udst);
    const usize can_write_max_times = (usize) allow_write_count / _ReadWriteOnce;
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

template<UCSKind __from, UCSKind __to, X86SIMDLevel __simd_level>
force_inline bool copy_ucs_elevate_sse4_or_avx2(const SrcInfo<__from> &src_info, UCSType_t<__to> *&dst, BufferInfo &buffer_info) {
    static_assert(__simd_level == X86SIMDLevel::AVX2 || __simd_level == X86SIMDLevel::SSE4);
    COMMON_TYPE_DEF();
    UCSType_t<__from> *src = src_info.src_start;
    usrc *const src_end = src + static_cast<std::ptrdiff_t>(src_info.src_size);
    // process 128 bits once
    constexpr std::ptrdiff_t _ProcessCountOnce = 128 / 8 / sizeof(usrc);
    __m128i u;
    while (src_end - src >= _ProcessCountOnce) {
        u = _mm_lddqu_si128((__m128i *) src); // lddqu, sse3
        bool v = check_escape_128bits<__from, __simd_level>(u);
        if (likely(v)) {
            cvtepu_128<__from, __to, __simd_level>(u, src, dst);
        } else {
            // first, check and resize
            auto additional_needed_count = (src_end - src - _ProcessCountOnce) + _ProcessCountOnce * 6;
            auto required_len_u8 = buffer_info.get_required_len_u8<__to>(dst, additional_needed_count);
            RETURN_ON_UNLIKELY_RESIZE_FAIL(__to, dst, buffer_info, required_len_u8);
            // if (unlikely(required_len_u8 > buffer_info.m_size)) {
            //     if (unlikely(!buffer_info.resizer<__to>(dst, required_len_u8))) {
            //         return false;
            //     }
            // }
            bool _c = copy_escape_fixsize<__from, __to, 128>(src, dst);
            RETURN_ON_UNLIKELY_ERR(!_c);
        }
    }
    const auto left_count = src_end - src;
    if (left_count == 0) return true;
    assert(left_count < _ProcessCountOnce);
    auto required_len_u8 = std::max<Py_ssize_t>(128 / 8 / _SizeRatio, buffer_info.get_required_len_u8<__to>(dst, left_count * 6));
    RETURN_ON_UNLIKELY_RESIZE_FAIL(__to, dst, buffer_info, required_len_u8);
    // if (unlikely(required_len_u8 > buffer_info.m_size)) {
    //     if (unlikely(!buffer_info.resizer<__to>(dst, required_len_u8))) {
    //         return false;
    //     }
    // }
    u = _mm_lddqu_si128((__m128i *) src); // lddqu, sse3
    bool v = check_escape_tail_128bits<__from, __simd_level>(u, left_count);
    if (likely(v)) {
        // requires 128 / 8 * _SizeRatio bytes
        // discard this pointer!
        udst *_temp_d = dst;
        cvtepu_128<__from, __to, __simd_level>(u, src, _temp_d);
        dst += left_count;
        return true;
    } else {
        // requires left_count * 6 * sizeof(udst) bytes
        return copy_escape_impl<__from, __to>(src, dst, left_count);
    }
}

template<UCSKind __from, UCSKind __to, X86SIMDLevel _SIMDLevel>
force_inline void copy_ucs_elevate_sse4_or_avx2_nocheck(UCSType_t<__to> *&write_ptr, UCSType_t<__from> *src, Py_ssize_t len, Py_ssize_t allow_write_count) {
    static_assert(__from < __to);
    static_assert(_SIMDLevel >= X86SIMDLevel::SSE4);
    assert(allow_write_count >= 0);
    COMMON_TYPE_DEF();
    constexpr usize _ReadWriteOnce = 128 / 8 / sizeof(usrc);
    const usize can_write_max_times = (usize) allow_write_count / _ReadWriteOnce;
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
force_inline void copy_ucs_general_nocheck(UCSType_t<__to> *&write_ptr, UCSType_t<__from> *src, Py_ssize_t len, Py_ssize_t allow_write_count) {
    using udst = UCSType_t<__to>;
    assert(len >= 0);
    for (Py_ssize_t i = 0; i < len; ++i) {
        *write_ptr = (udst) *src;
        ++write_ptr;
        ++src;
    }
}

template<UCSKind __from, UCSKind __to, X86SIMDLevel _SIMDLevel>
force_inline void copy_elevate_nocheck(UCSType_t<__to> *&write_ptr, UCSType_t<__from> *src, Py_ssize_t len, Py_ssize_t allow_write_count) {
    static_assert(__from < __to);
    if constexpr (_SIMDLevel >= X86SIMDLevel::AVX512) {
        return copy_ucs_elevate_avx512_nocheck<__from, __to, _SIMDLevel>(write_ptr, src, len, allow_write_count);
    } else if constexpr (_SIMDLevel >= X86SIMDLevel::AVX2) {
        return copy_ucs_elevate_sse4_or_avx2_nocheck<__from, __to, _SIMDLevel>(write_ptr, src, len, allow_write_count);
    } else if constexpr (_SIMDLevel >= X86SIMDLevel::SSE4) {
        return copy_ucs_elevate_sse4_or_avx2_nocheck<__from, __to, _SIMDLevel>(write_ptr, src, len, allow_write_count);
    } else {
        return copy_ucs_general_nocheck<__from, __to, _SIMDLevel>(write_ptr, src, len, allow_write_count);
    }
}

template<UCSKind __from, UCSKind __to, X86SIMDLevel __simd_level>
force_inline bool copy_ucs_elevate(const SrcInfo<__from> &src_info, UCSType_t<__to> *&dst, BufferInfo &buffer_info) {
    COMMON_TYPE_DEF();
    static_assert(__from < __to);
    // check if dst is enough to store. If not, resize it

    auto requires_additional_count = src_info.src_size;
    auto required_len_u8 = buffer_info.get_required_len_u8<__to>(dst, requires_additional_count);

    RETURN_ON_UNLIKELY_RESIZE_FAIL(__to, dst, buffer_info, required_len_u8);
    // if (unlikely(required_len_u8 > buffer_info.m_size)) {
    //     if (unlikely(!buffer_info.resizer<__to>(dst, required_len_u8))) {
    //         return false;
    //     }
    // }

    if constexpr (__simd_level >= X86SIMDLevel::AVX512) {
        return copy_ucs_elevate_avx512<__from, __to>(src_info, dst, buffer_info);
    } else if constexpr (__simd_level >= X86SIMDLevel::AVX2) {
        return copy_ucs_elevate_sse4_or_avx2<__from, __to, X86SIMDLevel::AVX2>(src_info, dst, buffer_info);
    } else if constexpr (__simd_level >= X86SIMDLevel::SSE4) {
        return copy_ucs_elevate_sse4_or_avx2<__from, __to, X86SIMDLevel::SSE4>(src_info, dst, buffer_info);
    } else {
        return copy_ucs_with_runtime_resize<__from, __to>(src_info, dst, buffer_info);
    }
}

template<UCSKind __kind, X86SIMDLevel __simd_level>
force_inline bool copy_ucs_same_avx2_or_avx512(const SrcInfo<__kind> &src_info, UCSType_t<__kind> *&dst, BufferInfo &buffer_info) {
    static_assert(__simd_level >= X86SIMDLevel::AVX2, "Only AVX2 or above support this");
    using uu = UCSType_t<__kind>;

    UCSType_t<__kind> *src = src_info.src_start;
    uu *const src_end = src + static_cast<std::ptrdiff_t>(src_info.src_size);

    constexpr std::ptrdiff_t _ProcessCountOnce = 256 / 8 / sizeof(uu);

    __m256i u;
    while (src_end - src >= _ProcessCountOnce) {
        u = _mm256_lddqu_si256((__m256i *) src); // vlddqu, AVX
        bool v = check_escape_256bits<__kind, __simd_level>(u);
        if (likely(v)) {
            _mm256_storeu_si256((__m256i *) dst, u); // vmovdqu, AVX
            ptr_move_bits<__kind, 256>(src);
            ptr_move_bits<__kind, 256>(dst);
        } else {
            // first, check and resize
            auto additional_needed_count = (src_end - src - _ProcessCountOnce) + _ProcessCountOnce * 6;
            auto required_len_u8 = buffer_info.get_required_len_u8<__kind>(dst, additional_needed_count);
            RETURN_ON_UNLIKELY_RESIZE_FAIL(__kind, dst, buffer_info, required_len_u8);
            // if (unlikely(required_len_u8 > buffer_info.m_size)) {
            //     if (unlikely(!buffer_info.resizer<__kind>(dst, required_len_u8))) {
            //         return false;
            //     }
            // }
            // verify 128 bits twice
            __m128i u1;
            u1 = _mm256_extracti128_si256(u, 0); // vextracti128, AVX2
            if (check_escape_128bits<__kind, __simd_level>(u1)) {
                _mm_storeu_si128((__m128i *) dst, u1); // vmovdqu, AVX
                ptr_move_bits<__kind, 128>(src);
                ptr_move_bits<__kind, 128>(dst);
            } else {
                bool _c = copy_escape_fixsize<__kind, __kind, 128>(src, dst);
                RETURN_ON_UNLIKELY_ERR(!_c);
            }
            u1 = _mm256_extracti128_si256(u, 1); // vextracti128, AVX2
            if (check_escape_128bits<__kind, __simd_level>(u1)) {
                _mm_storeu_si128((__m128i *) dst, u1); // vmovdqu, AVX
                ptr_move_bits<__kind, 128>(src);
                ptr_move_bits<__kind, 128>(dst);
            } else {
                bool _c = copy_escape_fixsize<__kind, __kind, 128>(src, dst);
                RETURN_ON_UNLIKELY_ERR(!_c);
            }
        }
    }
    const auto left_count = src_end - src;
    if (!left_count) return true;
    assert(left_count < _ProcessCountOnce);
    auto required_len_u8 = buffer_info.get_required_len_u8<__kind>(dst, left_count * 6);
    RETURN_ON_UNLIKELY_RESIZE_FAIL(__kind, dst, buffer_info, required_len_u8);
    // TODO fix this
    u = _mm256_lddqu_si256((__m256i *) src); // vlddqu, AVX
    bool v = check_escape_tail_256bits<__kind, __simd_level>(u, left_count);
    if (likely(v)) {
        _mm256_storeu_si256((__m256i *) dst, u); // vmovdqu, AVX
        dst += left_count;
    } else {
        return copy_escape_impl<__kind, __kind>(src, dst, left_count);
    }
    return true;
}

template<UCSKind __kind>
force_inline bool copy_ucs_same_sse4(const SrcInfo<__kind> &src_info, UCSType_t<__kind> *&dst, BufferInfo &buffer_info) {
    using uu = UCSType_t<__kind>;

    UCSType_t<__kind> *src = src_info.src_start;
    uu *const src_end = src + static_cast<std::ptrdiff_t>(src_info.src_size);

    constexpr std::ptrdiff_t _ProcessCountOnce = 128 / 8 / sizeof(uu);

    while (src_end - src >= _ProcessCountOnce) {
        __m128i u;
        u = _mm_lddqu_si128((__m128i *) src); // lddqu, sse3
        bool v = check_escape_128bits<__kind, X86SIMDLevel::SSE4>(u);
        if (likely(v)) {
            _mm_storeu_si128((__m128i *) dst, u); // vmovdqu, sse2
            ptr_move_bits<__kind, 128>(src);
            ptr_move_bits<__kind, 128>(dst);
        } else {
            // first, check and resize
            auto additional_needed_count = (src_end - src - _ProcessCountOnce) + _ProcessCountOnce * 6;
            auto required_len_u8 = buffer_info.get_required_len_u8<__kind>(dst, additional_needed_count);
            RETURN_ON_UNLIKELY_RESIZE_FAIL(__kind, dst, buffer_info, required_len_u8);
            // if (unlikely(required_len_u8 > buffer_info.m_size)) {
            //     if (unlikely(!buffer_info.resizer<__kind>(dst, required_len_u8))) {
            //         return false;
            //     }
            // }
            bool _c = copy_escape_fixsize<__kind, __kind, 128>(src, dst);
            RETURN_ON_UNLIKELY_ERR(!_c);
        }
    }
    const auto left_count = src_end - src;
    assert(left_count < _ProcessCountOnce);
    auto required_len_u8 = buffer_info.get_required_len_u8<__kind>(dst, left_count * 6);
    RETURN_ON_UNLIKELY_RESIZE_FAIL(__kind, dst, buffer_info, required_len_u8);
    // if (unlikely(required_len_u8 > buffer_info.m_size)) {
    //     if (unlikely(!buffer_info.resizer<__kind>(dst, required_len_u8))) {
    //         return false;
    //     }
    // }
    return copy_escape_impl<__kind, __kind>(src, dst, left_count);
}

template<UCSKind __kind, X86SIMDLevel __simd_level>
force_inline bool copy_ucs_same(const SrcInfo<__kind> &src_info, UCSType_t<__kind> *&dst, BufferInfo &buffer_info) {
    auto requires_additional_count = src_info.src_size;
    auto required_len_u8 = buffer_info.get_required_len_u8<__kind>(dst, requires_additional_count);

    RETURN_ON_UNLIKELY_RESIZE_FAIL(__kind, dst, buffer_info, required_len_u8);
    // if (unlikely(required_len_u8 > buffer_info.m_size)) {
    //     if (unlikely(!buffer_info.resizer<__kind>(dst, required_len_u8))) {
    //         return false;
    //     }
    // }

    if constexpr (__simd_level >= X86SIMDLevel::AVX512) {
        return copy_ucs_same_avx2_or_avx512<__kind, X86SIMDLevel::AVX512>(src_info, dst, buffer_info);
    } else if constexpr (__simd_level >= X86SIMDLevel::AVX2) {
        return copy_ucs_same_avx2_or_avx512<__kind, X86SIMDLevel::AVX2>(src_info, dst, buffer_info);
    } else if constexpr (__simd_level >= X86SIMDLevel::SSE4) {
        return copy_ucs_same_sse4<__kind>(src_info, dst, buffer_info);
    } else {
        return copy_ucs_with_runtime_resize<__kind, __kind>(src_info, dst, buffer_info);
    }
}

template<UCSKind __from, UCSKind __to, X86SIMDLevel __simd_level>
force_inline bool copy_ucs(const SrcInfo<__from> &src_info, UCSType_t<__to> *&dst, BufferInfo &buffer_info) {
    if constexpr (__from != __to) {
        return copy_ucs_elevate<__from, __to, __simd_level>(src_info, dst, buffer_info);
    } else {
        return copy_ucs_same<__from, __simd_level>(src_info, dst, buffer_info);
    }
}

typedef struct CtnType {
    PyObject *ctn;
    Py_ssize_t index;
} CtnType;


// enum class EncodeOpType : u8 {
//     END,
//     STRING,
//     KEY,
//     DICT_EMPTY,
//     DICT_BEGIN,
//     DICT_END,
//     ARRAY_EMPTY,
//     ARRAY_BEGIN,
//     ARRAY_END,
//     U64,
//     I64,
//     FLOAT,
//     TRUE_VALUE,
//     FALSE_VALUE,
//     NULL_VALUE,
//     POS_NAN,
//     NEG_NAN,
//     ZERO_VALUE,
//     POS_INF,
//     NEG_INF,
// };


// typedef struct EncodeOpBufferLinkedList {
//     void **m_buffer;
//     size_t m_buffer_size;

//     static force_inline EncodeOpBufferLinkedList init_buffer(size_t in_size) {
//         EncodeOpBufferLinkedList ret{
//                 nullptr,
//                 in_size,
//         };
//         void **_buffer = (void **) malloc(in_size * sizeof(void *));
//         if (!_buffer) {
//             PyErr_NoMemory();
//         } else {
//             ret.m_buffer = _buffer;
//             ret.init_inplace();
//         }
//         return ret;
//     }

//     force_inline void init_inplace() {
//         assert(this->m_buffer);
// #ifndef NDEBUG
//         memset(this->m_buffer, 0, this->m_buffer_size * sizeof(void *));
// #endif
//     }
// } EncodeOpBufferLinkedList;

thread_local CtnType _CtnStack[PYYJSON_ENCODE_MAX_RECURSION];
// thread_local EncodeOpBufferLinkedList op_buffer_nodes[128];
// thread_local void *op_buffer_head[PYYJSON_ENCODE_OP_BUFFER_INIT_SIZE];

// typedef struct EncodeOpDescriber {
//     size_t op_buffer_node_index;
//     void **next_ptr;
//     u8 *next_op;
//     size_t read_buffer_node_index;

//     force_inline static EncodeOpDescriber init() {
//         static_assert(PYYJSON_ENCODE_OP_BUFFER_INIT_SIZE >= 1);
//         op_buffer_nodes[0].m_buffer = &op_buffer_head[0];
//         op_buffer_nodes[0].m_buffer_size = PYYJSON_ENCODE_OP_BUFFER_INIT_SIZE;
//         EncodeOpDescriber ret{
//                 0,
//                 &op_buffer_head[0] + 1,
//                 (u8 *) &op_buffer_head[0],
//                 0,
//         };
//         return ret;
//     }

//     force_inline static EncodeOpDescriber init_as_read(EncodeOpDescriber *old_op_desc) {
//         assert(old_op_desc->read_buffer_node_index == 0);
//         EncodeOpDescriber ret{
//                 old_op_desc->op_buffer_node_index,
//                 &op_buffer_head[0] + 1,
//                 (u8 *) &op_buffer_head[0],
//                 0,
//         };
//         return ret;
//     }

//     /* Write op without any data. */
//     force_inline bool write_simple_op(EncodeOpType op) {
//         static_assert((sizeof(void *) & (sizeof(void *) - 1)) == 0);
//         //
//         assert(this->next_op != (u8 *) this->next_ptr);
//         if (!(((size_t) this->next_op) & (sizeof(void *) - 1))) {
//             assert(this->next_op == (u8 *) (this->next_ptr - 1));
//         }
//         //
//         *this->next_op++ = static_cast<u8>(op);
//         if (((size_t) this->next_op) & (sizeof(void *) - 1)) {
//             return true;
//         }
//         // move next_op to next_ptr
//         const auto &cur_op_node = op_buffer_nodes[this->op_buffer_node_index];
//         if (unlikely(this->next_ptr >= cur_op_node.m_buffer + cur_op_node.m_buffer_size)) {
//             assert(this->next_ptr == cur_op_node.m_buffer + cur_op_node.m_buffer_size);
//             if (unlikely(this->op_buffer_node_index + 1 >= 128 || (cur_op_node.m_buffer_size & (1ULL << (sizeof(size_t) * 8 - 1))))) {
//                 PyErr_NoMemory();
//                 return false;
//             }
//             auto &new_op_node = op_buffer_nodes[this->op_buffer_node_index + 1];
//             new_op_node = EncodeOpBufferLinkedList::init_buffer(cur_op_node.m_buffer_size << 1);
//             if (unlikely(new_op_node.m_buffer == nullptr)) {
//                 return false;
//             }
//             this->op_buffer_node_index++;
//             this->next_ptr = new_op_node.m_buffer;
//         }
//         this->next_op = (u8 *) (this->next_ptr++);
//         return true;
//     }

//     template<typename T>
//     force_inline bool write_op_with_data(EncodeOpType op, T obj) {
//         static_assert((sizeof(void *) & (sizeof(void *) - 1)) == 0);
//         static_assert((sizeof(T) % sizeof(void *)) == 0);
//         constexpr size_t _SizeRatio = sizeof(T) / sizeof(void *);
//         //
//         assert(this->next_op != (u8 *) this->next_ptr);
//         if (!(((size_t) this->next_op) & (sizeof(void *) - 1))) {
//             assert(this->next_op == (u8 *) (this->next_ptr - 1));
//         }
//         //
//         *this->next_op++ = static_cast<u8>(op);

//         const auto &cur_op_node = op_buffer_nodes[this->op_buffer_node_index];
//         if (unlikely(this->next_ptr + _SizeRatio >= cur_op_node.m_buffer + cur_op_node.m_buffer_size)) {
//             {
//                 bool _c = this->op_buffer_node_index + 1 >= 128 || (cur_op_node.m_buffer_size & (1ULL << (sizeof(size_t) * 8 - 1)));
//                 if (unlikely(_c)) {
//                     PyErr_NoMemory();
//                     return false;
//                 }
//             }
//             auto &new_op_node = op_buffer_nodes[this->op_buffer_node_index + 1];
//             new_op_node = EncodeOpBufferLinkedList::init_buffer(cur_op_node.m_buffer_size << 1);
//             if (unlikely(new_op_node.m_buffer == nullptr)) {
//                 return false;
//             }

//             assert(new_op_node.m_buffer_size > _SizeRatio);
//             this->op_buffer_node_index++;
//             if (this->next_ptr + _SizeRatio == cur_op_node.m_buffer + cur_op_node.m_buffer_size) {
//                 memcpy(this->next_ptr, &obj, sizeof(T));
//                 this->next_ptr = new_op_node.m_buffer;
//                 if (!(((size_t) this->next_op) & (sizeof(void *) - 1))) {
//                     // move next_op to next_ptr
//                     this->next_op = (u8 *) (this->next_ptr++);
//                 }
//                 return true;
//             }
//             this->next_ptr = new_op_node.m_buffer;
//         }
//         memcpy(this->next_ptr, &obj, sizeof(T));
//         this->next_ptr += _SizeRatio;
//         if (!(((size_t) this->next_op) & (sizeof(void *) - 1))) {
//             // move next_op to next_ptr
//             this->next_op = (u8 *) (this->next_ptr++);
//         }
//         return true;
//     }

//     force_inline EncodeOpType peek_op() {
//         return static_cast<EncodeOpType>(*this->next_op);
//     }

//     force_inline void read_simple_op() {
//         if (((size_t)++ this->next_op) & (sizeof(void *) - 1)) {
//             return;
//         }
//         this->next_op = (u8 *) (this->next_ptr++);
//         // move next_op to next_ptr
//         const auto &cur_op_node = op_buffer_nodes[this->read_buffer_node_index];
//         if (unlikely(this->next_ptr >= cur_op_node.m_buffer + cur_op_node.m_buffer_size)) {
//             assert(this->next_ptr == cur_op_node.m_buffer + cur_op_node.m_buffer_size);
//             auto &new_op_node = op_buffer_nodes[++this->read_buffer_node_index];
//             this->next_ptr = new_op_node.m_buffer;
//             assert(this->next_ptr);
//         }
//     }

//     template<typename T>
//     force_inline T read_op_data() {
//         static_assert((sizeof(void *) & (sizeof(void *) - 1)) == 0);
//         static_assert((sizeof(T) % sizeof(void *)) == 0);
//         T ret;
//         constexpr size_t _SizeRatio = sizeof(T) / sizeof(void *);
//         this->next_op++;
//         const auto &cur_op_node = op_buffer_nodes[this->read_buffer_node_index];
//         if (unlikely(this->next_ptr + _SizeRatio >= cur_op_node.m_buffer + cur_op_node.m_buffer_size)) {
//             auto &new_op_node = op_buffer_nodes[++this->read_buffer_node_index];
//             assert(new_op_node.m_buffer);
//             if (this->next_ptr + _SizeRatio == cur_op_node.m_buffer + cur_op_node.m_buffer_size) {
//                 ret = *(T *) this->next_ptr;
//                 this->next_ptr = new_op_node.m_buffer;
//                 if (!(((size_t) this->next_op) & (sizeof(void *) - 1))) {
//                     // move next_op to next_ptr
//                     this->next_op = (u8 *) (this->next_ptr++);
//                 }
//                 return ret;
//             }
//             this->next_ptr = new_op_node.m_buffer;
//         }
//         ret = *(T *) this->next_ptr;
//         this->next_ptr += _SizeRatio;
//         if (!(((size_t) this->next_op) & (sizeof(void *) - 1))) {
//             // move next_op to next_ptr
//             this->next_op = (u8 *) (this->next_ptr++);
//         }
//         return ret;
//     }

//     force_inline void release() {
//         for (size_t i = 1; i <= this->op_buffer_node_index; i++) {
//             free(op_buffer_nodes[i].m_buffer);
//             op_buffer_nodes[i].m_buffer = nullptr;
//         }
//     }
// } EncodeOpDescriber;


// typedef struct EncodeWriteOpConfig {
//     Py_ssize_t cur_nested_depth = -1;
//     PyObject *cur_obj;
//     Py_ssize_t cur_pos = 0;
//     UCSKind cur_ucs_kind = UCSKind::UCS1;
//     bool is_all_ascii = true;
//     // temp variables storing key and value
//     Py_ssize_t cur_list_size; // cache for each list, no need to check it repeatedly
//     PyObject *key, *val;
// } EncodeWriteOpConfig;

// typedef struct EncodeWriteUnicodeConfig {
//     Py_ssize_t indent_level;
//     const UCSKind cur_ucs_kind;
//     bool is_obj;
//     const bool is_all_ascii;

//     force_inline static EncodeWriteUnicodeConfig init_from_op_config(EncodeWriteOpConfig &op_cfg) {
//         EncodeWriteUnicodeConfig ret{
//                 0,
//                 op_cfg.cur_ucs_kind,
//                 true, // to avoid an extra \n
//                 op_cfg.is_all_ascii,
//         };
//         return ret;
//     }

//     force_inline void increase_indent(bool _is_obj) {
//         int *const obj_stack = (int *) &_CtnStack[0];
//         obj_stack[this->indent_level++] = this->is_obj ? 1 : 0;
//         this->is_obj = _is_obj;
//         assert(this->indent_level);
//     }

//     force_inline void decrease_indent() {
//         int *const obj_stack = (int *) &_CtnStack[0];
//         assert(this->indent_level);
//         this->indent_level--;
//         this->is_obj = obj_stack[this->indent_level];
//     }

// } EncodeWriteUnicodeConfig;

template<typename T, size_t L, T _DefaultVal, size_t... Is>
constexpr auto make_filled_array_impl(std::index_sequence<Is...>) {
    return std::array<T, L>{((void) Is, _DefaultVal)...};
}

template<typename T, size_t L, T _DefaultVal>
constexpr std::array<T, L> create_array() {
    return make_filled_array_impl<T, L, _DefaultVal>(std::make_index_sequence<L>{});
}

// template<IndentLevel __indent, UCSKind __kind>
// force_inline bool write_line_and_indent_impl(UCSType_t<__kind> *&dst, BufferInfo &buffer_info, const EncodeWriteUnicodeConfig &cfg, Py_ssize_t additional_reserve) {
// #define INDENT_SIZE(__indent) (static_cast<int>(__indent))
// #define IDENT_WITH_NEWLINE_SIZE(__indent, _indent_level) ((INDENT_SIZE(__indent) != 0) ? (_indent_level * INDENT_SIZE(__indent)) : 0)
//     static_assert(NEED_INDENT(__indent));
//     using uu = UCSType_t<__kind>;
//     Py_ssize_t need_space_count = IDENT_WITH_NEWLINE_SIZE(__indent, cfg.indent_level);
//     Py_ssize_t need_reserve_count = need_space_count + 1 + additional_reserve;
//     Py_ssize_t required_len_u8 = buffer_info.get_required_len_u8<__kind>(dst, need_reserve_count);
//     RETURN_ON_UNLIKELY_RESIZE_FAIL(__kind, dst, buffer_info, required_len_u8);

//     *dst++ = '\n';
//     // need_count--;
//     constexpr size_t _TemplateSize = size_align_up(sizeof(uu), SIZEOF_VOID_P) / sizeof(uu);
//     constexpr uu _FillVal = (uu) ' ';
//     constexpr std::array<uu, _TemplateSize> _Template = create_array<uu, _TemplateSize, _FillVal>();
//     while (need_space_count >= _TemplateSize) {
//         memcpy((void *) dst, (void *) &_Template[0], sizeof(_Template));
//         dst += _TemplateSize;
//         need_space_count -= _TemplateSize;
//     }
//     while (need_space_count > 0) {
//         *dst++ = _FillVal;
//         need_space_count--;
//     }
//     return true;
// #undef IDENT_WITH_NEWLINE_SIZE
// #undef INDENT_SIZE
// }

// template<IndentLevel __indent, UCSKind __kind, Py_ssize_t __additional_reserve>
// force_inline bool write_line_and_indent(UCSType_t<__kind> *&dst, BufferInfo &buffer_info, const EncodeWriteUnicodeConfig &cfg) {
//     return write_line_and_indent_impl<__indent, __kind>(dst, buffer_info, cfg, __additional_reserve);
// }

// template<UCSKind __kind>
// force_inline bool write_reserve_impl(UCSType_t<__kind> *&dst, BufferInfo &buffer_info, size_t reserve_count) {
//     using uu = UCSType_t<__kind>;
//     auto required_len_u8 = buffer_info.get_required_len_u8<__kind>(dst, reserve_count);
//     RETURN_ON_UNLIKELY_RESIZE_FAIL(__kind, dst, buffer_info, required_len_u8);
//     return true;
// }

// template<UCSKind __kind, size_t __reserve>
// force_inline bool write_reserve(UCSType_t<__kind> *&dst, BufferInfo &buffer_info) {
//     return write_reserve_impl<__kind>(dst, buffer_info, __reserve);
// }

// template<IndentLevel __indent, UCSKind __kind, bool is_obj>
// force_inline bool write_empty_ctn(UCSType_t<__kind> *&dst, BufferInfo &buffer_info, const EncodeWriteUnicodeConfig &cfg) {
//     using uu = UCSType_t<__kind>;
//     constexpr Py_ssize_t reserve = size_align_up(3 * sizeof(uu), SIZEOF_VOID_P) / sizeof(uu);
//     // buffer size check
//     bool _c;
//     if constexpr (NEED_INDENT(__indent)) {
//         if (!cfg.is_obj) {
//             _c = write_line_and_indent<__indent, __kind, reserve>(dst, buffer_info, cfg);
//         } else {
//             _c = write_reserve<__kind, reserve>(dst, buffer_info);
//         }
//     } else {
//         _c = write_reserve<__kind, reserve>(dst, buffer_info);
//     }
//     RETURN_ON_UNLIKELY_ERR(!_c);
//     //
//     constexpr uu _Template[reserve] = {
//             (uu) ('[' | ((u8) is_obj << 5)),
//             (uu) (']' | ((u8) is_obj << 5)),
//             (uu) ',',
//     };
//     memcpy(dst, &_Template[0], sizeof(_Template));
//     dst += 3;
//     return true;
// }

// template<IndentLevel __indent, UCSKind __kind, bool is_obj>
// force_inline bool write_ctn_begin(UCSType_t<__kind> *&dst, BufferInfo &buffer_info, EncodeWriteUnicodeConfig &cfg) {
//     using uu = UCSType_t<__kind>;
//     // ut *dst = (ut *) *dst_raw;
//     bool _c;
//     if constexpr (NEED_INDENT(__indent)) {
//         if (!cfg.is_obj) {
//             _c = write_line_and_indent<__indent, __kind, 1>(dst, buffer_info, cfg);
//         } else {
//             _c = write_reserve<__kind, 1>(dst, buffer_info);
//         }
//     } else {
//         _c = write_reserve<__kind, 1>(dst, buffer_info);
//     }
//     RETURN_ON_UNLIKELY_ERR(!_c);
//     constexpr uu to_write = (uu) ('[' | ((u8) is_obj << 5));
//     *dst++ = to_write;
//     // write current ctn type
//     // new line, increase indent
//     cfg.increase_indent(is_obj);
//     return true;
// }

// template<IndentLevel __indent, UCSKind __kind, bool is_obj>
// force_inline bool write_ctn_end(UCSType_t<__kind> *&dst, BufferInfo &buffer_info, EncodeWriteUnicodeConfig &cfg) {
//     using uu = UCSType_t<__kind>;
//     constexpr Py_ssize_t reserve = size_align_up(2 * sizeof(uu), SIZEOF_VOID_P) / sizeof(uu);
//     // ut *dst = (ut *) *dst_raw;
//     // DECREASE_INDENT();
//     cfg.decrease_indent();
//     // erase last ','
//     dst--;
//     bool _c;
//     if constexpr (NEED_INDENT(__indent)) {
//         // WRITE_LINE();
//         // WRITE_INDENT();
//         _c = write_line_and_indent<__indent, __kind, reserve>(dst, buffer_info, cfg);
//     } else {
//         _c = write_reserve<__kind, reserve>(dst, buffer_info);
//     }
//     RETURN_ON_UNLIKELY_ERR(!_c);

//     constexpr uu _Template[reserve] = {
//             (uu) (']' | ((u8) is_obj << 5)),
//             (uu) ',',
//     };
//     memcpy(dst, &_Template[0], sizeof(_Template));
//     dst += 2;
//     return true;

//     // *(dst++) = (ut) (']' | ((u8) GET_CTN_TYPE() << 5));
//     // *(dst++) = (ut) ',';
//     // *dst_raw = (void *) dst;
// }

template<UCSKind __kind, X86SIMDLevel __simd_level>
force_inline bool write_raw_string(UCSType_t<__kind> *&dst, void *src_raw, Py_ssize_t size, unsigned int src_kind, Py_ssize_t additional_reserve, BufferInfo &buffer_info) {
#define UCS1_DISPATCH()                          \
    u8 *usrc = (u8 *) src_raw;                   \
    SrcInfo<UCSKind::UCS1> src_info{usrc, size}; \
    _c = copy_ucs<UCSKind::UCS1, __kind, __simd_level>(src_info, dst, buffer_info)
#define UCS2_DISPATCH()                          \
    u16 *usrc = (u16 *) src_raw;                 \
    SrcInfo<UCSKind::UCS2> src_info{usrc, size}; \
    _c = copy_ucs<UCSKind::UCS2, __kind, __simd_level>(src_info, dst, buffer_info)
#define UCS4_DISPATCH()                          \
    u32 *usrc = (u32 *) src_raw;                 \
    SrcInfo<UCSKind::UCS4> src_info{usrc, size}; \
    _c = copy_ucs<UCSKind::UCS4, __kind, __simd_level>(src_info, dst, buffer_info)

    using udst = UCSType_t<__kind>;

    assert(static_cast<int>(src_kind) <= static_cast<int>(__kind));

    std::ptrdiff_t required_len_u8 = buffer_info.calc_dst_offset_u8<__kind>(dst + additional_reserve + size);
    RETURN_ON_UNLIKELY_RESIZE_FAIL(__kind, dst, buffer_info, required_len_u8);

    bool _c;

    if constexpr (__kind == UCSKind::UCS1) {
        UCS1_DISPATCH();
    } else if constexpr (__kind == UCSKind::UCS2) {
        switch (src_kind) {
            case PyUnicode_1BYTE_KIND: {
                UCS1_DISPATCH();
                break;
            }
            case PyUnicode_2BYTE_KIND: {
                UCS2_DISPATCH();
                break;
            }
            default: {
                Py_UNREACHABLE();
                _c = false;
            }
        }
    } else {
        switch (src_kind) {
            case PyUnicode_1BYTE_KIND: {
                UCS1_DISPATCH();
                break;
            }
            case PyUnicode_2BYTE_KIND: {
                UCS2_DISPATCH();
                break;
            }
            case PyUnicode_4BYTE_KIND: {
                UCS4_DISPATCH();
                break;
            }
            default: {
                Py_UNREACHABLE();
                _c = false;
            }
        }
    }

    RETURN_ON_UNLIKELY_ERR(!_c);

    required_len_u8 = buffer_info.calc_dst_offset_u8<__kind>(dst + additional_reserve);
    RETURN_ON_UNLIKELY_RESIZE_FAIL(__kind, dst, buffer_info, required_len_u8);

    return true;
}

// template<IndentLevel __indent, UCSKind __kind, X86SIMDLevel __simd_level>
// force_inline bool write_str(PyObject *key, UCSType_t<__kind> *&dst, BufferInfo &buffer_info, const EncodeWriteUnicodeConfig &cfg) {
//     using uu = UCSType_t<__kind>;
//     assert(PyUnicode_Check(key));
//     Py_ssize_t unicode_size = PyUnicode_GET_LENGTH(key);
//     unsigned int src_kind = PyUnicode_KIND(key);
//     auto data_raw = PyUnicode_DATA(key);
//     bool _c;
//     if constexpr (NEED_INDENT(__indent)) {
//         //     WRITE_LINE();
//         // WRITE_INDENT();
//         if (!cfg.is_obj) {
//             _c = write_line_and_indent_impl<__indent, __kind>(dst, buffer_info, cfg, unicode_size + 2);
//         } else {
//             _c = write_reserve_impl<__kind>(dst, buffer_info, unicode_size + 2);
//         }
//     } else {
//         _c = write_reserve_impl<__kind>(dst, buffer_info, unicode_size + 2);
//     }
//     RETURN_ON_UNLIKELY_ERR(!_c);

//     *(dst++) = (uu) '"';

//     _c = write_raw_string<__kind, __simd_level>(dst, data_raw, unicode_size, src_kind, 2, buffer_info);
//     RETURN_ON_UNLIKELY_ERR(!_c);

//     constexpr uu _Template[2] = {
//             (uu) '"',
//             (uu) ',',
//     };
//     memcpy(dst, &_Template[0], sizeof(_Template));
//     dst += 2;
//     return true;
// }

// template<IndentLevel __indent, UCSKind __kind, X86SIMDLevel __simd_level>
// force_inline bool write_key(PyObject *key, UCSType_t<__kind> *&dst, BufferInfo &buffer_info, const EncodeWriteUnicodeConfig &cfg) {
//     using uu = UCSType_t<__kind>;
//     assert(PyUnicode_Check(key));
//     Py_ssize_t unicode_size = PyUnicode_GET_LENGTH(key);
//     unsigned int src_kind = PyUnicode_KIND(key);
//     auto data_raw = PyUnicode_DATA(key);
//     bool _c;
//     // ut *dst = (ut *) *dst_raw;
//     if constexpr (NEED_INDENT(__indent)) {
//         //     WRITE_LINE();
//         // WRITE_INDENT();
//         _c = write_line_and_indent_impl<__indent, __kind>(dst, buffer_info, cfg, unicode_size + 4);
//     } else {
//         _c = write_reserve_impl<__kind>(dst, buffer_info, unicode_size + 4);
//     }
//     RETURN_ON_UNLIKELY_ERR(!_c);

//     *(dst++) = (uu) '"';

//     _c = write_raw_string<__kind, __simd_level>(dst, data_raw, unicode_size, src_kind, 3, buffer_info);
//     RETURN_ON_UNLIKELY_ERR(!_c);

//     constexpr size_t _Reserve = size_align_up(3 * sizeof(uu), SIZEOF_VOID_P) / sizeof(uu);

//     constexpr uu _Template[_Reserve] = {
//             (uu) '"',
//             (uu) ':',
//             (uu) ' ',
//     };
//     memcpy(dst, &_Template[0], sizeof(_Template));
//     constexpr size_t _Move = (NEED_INDENT(__indent)) ? 3 : 2;
//     dst += _Move;
//     return true;
// }

// template<IndentLevel __indent, UCSKind __kind>
// force_inline bool write_true(UCSType_t<__kind> *&dst, BufferInfo &buffer_info, const EncodeWriteUnicodeConfig &cfg) {
//     using uu = UCSType_t<__kind>;
//     constexpr Py_ssize_t reserve = size_align_up(5 * sizeof(uu), SIZEOF_VOID_P) / sizeof(uu);

//     bool _c;
//     if constexpr (NEED_INDENT(__indent)) {
//         if (!cfg.is_obj) {
//             // WRITE_LINE();
//             // WRITE_INDENT();
//             _c = write_line_and_indent<__indent, __kind, reserve>(dst, buffer_info, cfg);
//         } else {
//             _c = write_reserve<__kind, reserve>(dst, buffer_info);
//         }
//     } else {
//         _c = write_reserve<__kind, reserve>(dst, buffer_info);
//     }
//     RETURN_ON_UNLIKELY_ERR(!_c);

//     constexpr uu _Template[reserve] = {
//             (uu) 't',
//             (uu) 'r',
//             (uu) 'u',
//             (uu) 'e',
//             (uu) ',',
//     };
//     memcpy(dst, _Template, sizeof(_Template));
//     dst += 5;
//     return true;
// }

// template<IndentLevel __indent, UCSKind __kind>
// force_inline bool write_false(UCSType_t<__kind> *&dst, BufferInfo &buffer_info, const EncodeWriteUnicodeConfig &cfg) {
//     using uu = UCSType_t<__kind>;
//     constexpr Py_ssize_t reserve = size_align_up(6 * sizeof(uu), SIZEOF_VOID_P) / sizeof(uu);

//     bool _c;
//     if constexpr (NEED_INDENT(__indent)) {
//         if (!cfg.is_obj) {
//             // WRITE_LINE();
//             // WRITE_INDENT();
//             _c = write_line_and_indent<__indent, __kind, reserve>(dst, buffer_info, cfg);
//         } else {
//             _c = write_reserve<__kind, reserve>(dst, buffer_info);
//         }
//     } else {
//         _c = write_reserve<__kind, reserve>(dst, buffer_info);
//     }

//     RETURN_ON_UNLIKELY_ERR(!_c);

//     constexpr uu _Template[reserve] = {
//             (uu) 'f',
//             (uu) 'a',
//             (uu) 'l',
//             (uu) 's',
//             (uu) 'e',
//             (uu) ',',
//     };
//     memcpy(dst, _Template, sizeof(_Template));
//     dst += 6;
//     return true;
// }

// template<IndentLevel __indent, UCSKind __kind>
// force_inline bool write_null(UCSType_t<__kind> *&dst, BufferInfo &buffer_info, const EncodeWriteUnicodeConfig &cfg) {
//     using uu = UCSType_t<__kind>;
//     constexpr Py_ssize_t reserve = size_align_up(5 * sizeof(uu), SIZEOF_VOID_P) / sizeof(uu);

//     bool _c;
//     if constexpr (NEED_INDENT(__indent)) {
//         if (!cfg.is_obj) {
//             // WRITE_LINE();
//             // WRITE_INDENT();
//             _c = write_line_and_indent<__indent, __kind, reserve>(dst, buffer_info, cfg);
//         } else {
//             _c = write_reserve<__kind, reserve>(dst, buffer_info);
//         }
//     } else {
//         _c = write_reserve<__kind, reserve>(dst, buffer_info);
//     }
//     RETURN_ON_UNLIKELY_ERR(!_c);

//     constexpr uu _Template[reserve] = {
//             (uu) 'n',
//             (uu) 'u',
//             (uu) 'l',
//             (uu) 'l',
//             (uu) ',',
//     };
//     memcpy(dst, _Template, sizeof(_Template));
//     dst += 5;
//     return true;
// }

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


// template<IndentLevel __indent, UCSKind __kind, bool __is_signed>
// force_inline bool write_nan(UCSType_t<__kind> *&dst, BufferInfo &buffer_info, const EncodeWriteUnicodeConfig &cfg) {
//     using uu = UCSType_t<__kind>;
//     constexpr Py_ssize_t move_size = __is_signed ? 5 : 4;
//     constexpr Py_ssize_t reserve = size_align_up(move_size * sizeof(uu), SIZEOF_VOID_P) / sizeof(uu);

//     bool _c;
//     if constexpr (NEED_INDENT(__indent)) {
//         if (!cfg.is_obj) {
//             // WRITE_LINE();
//             // WRITE_INDENT();
//             _c = write_line_and_indent<__indent, __kind, reserve>(dst, buffer_info, cfg);
//         } else {
//             _c = write_reserve<__kind, reserve>(dst, buffer_info);
//         }
//     } else {
//         _c = write_reserve<__kind, reserve>(dst, buffer_info);
//     }
//     RETURN_ON_UNLIKELY_ERR(!_c);

//     constexpr auto &_Template = NaN_Template<uu, reserve, __is_signed>::value;
//     memcpy(dst, &_Template[0], sizeof(_Template));
//     dst += move_size;
//     return true;
// }

// template<IndentLevel __indent, UCSKind __kind>
// force_inline bool write_zero(UCSType_t<__kind> *&dst, BufferInfo &buffer_info, const EncodeWriteUnicodeConfig &cfg) {
//     using uu = UCSType_t<__kind>;
//     constexpr Py_ssize_t reserve = size_align_up(2 * sizeof(uu), SIZEOF_VOID_P) / sizeof(uu);

//     bool _c;
//     if constexpr (NEED_INDENT(__indent)) {
//         if (!cfg.is_obj) {
//             // WRITE_LINE();
//             // WRITE_INDENT();
//             _c = write_line_and_indent<__indent, __kind, reserve>(dst, buffer_info, cfg);
//         } else {
//             _c = write_reserve<__kind, reserve>(dst, buffer_info);
//         }
//     } else {
//         _c = write_reserve<__kind, reserve>(dst, buffer_info);
//     }
//     RETURN_ON_UNLIKELY_ERR(!_c);

//     constexpr uu _Template[reserve] = {
//             (uu) '0',
//             (uu) ',',
//     };
//     memcpy(dst, _Template, sizeof(_Template));
//     dst += 2;
//     return true;
// }

// template<IndentLevel __indent, UCSKind __kind, bool __is_signed>
// force_inline bool write_inf(UCSType_t<__kind> *&dst, BufferInfo &buffer_info, const EncodeWriteUnicodeConfig &cfg) {
//     using uu = UCSType_t<__kind>;
//     constexpr Py_ssize_t move_size = __is_signed ? 5 : 4;
//     constexpr Py_ssize_t reserve = size_align_up(move_size * sizeof(uu), SIZEOF_VOID_P) / sizeof(uu);

//     bool _c;
//     if constexpr (NEED_INDENT(__indent)) {
//         if (!cfg.is_obj) {
//             // WRITE_LINE();
//             // WRITE_INDENT();
//             _c = write_line_and_indent<__indent, __kind, reserve>(dst, buffer_info, cfg);
//         } else {
//             _c = write_reserve<__kind, reserve>(dst, buffer_info);
//         }
//     } else {
//         _c = write_reserve<__kind, reserve>(dst, buffer_info);
//     }
//     RETURN_ON_UNLIKELY_ERR(!_c);

//     constexpr auto &_Template = Inf_Template<uu, reserve, __is_signed>::value;
//     memcpy(dst, &_Template[0], sizeof(_Template));
//     dst += move_size;
//     return true;
// }


// template<IndentLevel __indent, UCSKind __kind>
// force_inline bool write_i64(i64 val, UCSType_t<__kind> *&dst, BufferInfo &buffer_info, const EncodeWriteUnicodeConfig &cfg) {
//     assert(false);
//     return false;
// }

// template<IndentLevel __indent, UCSKind __kind>
// force_inline bool write_float(i64 val, UCSType_t<__kind> *&dst, BufferInfo &buffer_info, const EncodeWriteUnicodeConfig &cfg) {
//     assert(false);
//     return false;
// }

// template<IndentLevel __indent, UCSKind __kind>
// force_inline bool write_u64(u64 val, UCSType_t<__kind> *&dst, BufferInfo &buffer_info, const EncodeWriteUnicodeConfig &cfg) {
//     assert(false);
//     return false;
// }

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

/**/
// #define GOTO_ARR_VAL_BEGIN_WITH_SIZE(in_size) \
//     do {                                      \
//         cur_list_size = (in_size);            \
//         goto arr_val_begin;                   \
//     } while (0)
// #define GOTO_TUPLE_VAL_BEGIN_WITH_SIZE(in_size) \
//     do {                                        \
//         cur_list_size = (in_size);              \
//         goto tuple_val_begin;                   \
//     } while (0)


// Py_ssize_t cur_nested_depth = -1;
//     PyObject *cur_obj;
//     Py_ssize_t cur_pos;
//     Py_ssize_t cur_list_size; // cache for each list, no need to check it repeatedly
//     UCSKind cur_ucs_kind = UCSKind::UCS1;
//     u8 *dst1 = (u8 *) buffer_info.m_buffer_start;
//     u16 *dst2 = nullptr;         // enabled if kind >= 2
//     u32 *dst4 = nullptr;         // enabled if kind == 4
//     Py_ssize_t dst1_size = 0; // valid if kind >= 2
//     Py_ssize_t dst2_size = 0; // valid if kind == 4

force_inline bool write_PyLongOp(PyObject *obj, EncodeOpDescriber &desc) {
    if (pylong_is_zero(obj)) {
        desc.write_simple_op(EncodeOpType::ZERO_VALUE);
        // APPEND_OP(ZERO);
    } else if (pylong_is_unsigned(obj)) {
        unsigned long long val = pylong_value_unsigned(obj);
        if (unlikely(val == (unsigned long long) -1 && PyErr_Occurred())) {
            return false;
        }
        desc.write_op_with_data<u64>(EncodeOpType::U64, val);
        // APPEND_U64(val);
    } else {
        long long val = PyLong_AsLongLong(obj);
        if (unlikely(val == -1 && PyErr_Occurred())) {
            return false;
        }
        desc.write_op_with_data<i64>(EncodeOpType::U64, val);
        // APPEND_I64<i64>(val);
    }
    return true;
}

// force_inline bool write_PyFloatOp(PyObject *obj, EncodeOpDescriber &desc) {
//     assert(false);
//     return false;
//     // TODO
// }

// template<IndentLevel __indent, UCSKind __kind, X86SIMDLevel __simd_level>
// force_noinline PyObject *pyyjson_dumps_read_op_write_str(EncodeWriteOpConfig *op_cfg, EncodeOpDescriber *op_desc_old) {
//     EncodeOpType op;
//     BufferInfo buffer_info = BufferInfo::init();
//     EncodeWriteUnicodeConfig cfg = EncodeWriteUnicodeConfig::init_from_op_config(*op_cfg);
//     EncodeOpDescriber op_desc = EncodeOpDescriber::init_as_read(op_desc_old);
//     using udst = UCSType_t<__kind>;
//     udst *dst = (udst *) buffer_info.m_buffer_start;

//     bool _c;
//     // final
//     Py_ssize_t final_size;
//     Py_UCS4 max_char;
//     PyObject *ret;
//     while (1) {
//         op = op_desc.peek_op();
//         switch (op) {
//             case EncodeOpType::DICT_EMPTY: {
//                 op_desc.read_simple_op();
//                 _c = write_empty_ctn<__indent, __kind, true>(dst, buffer_info, cfg);
//                 if (unlikely(!_c)) {
//                     goto fail;
//                 }
//                 break;
//             }
//             case EncodeOpType::DICT_BEGIN: {
//                 op_desc.read_simple_op();
//                 _c = write_ctn_begin<__indent, __kind, true>(dst, buffer_info, cfg);
//                 if (unlikely(!_c)) {
//                     goto fail;
//                 }
//                 break;
//             }
//             case EncodeOpType::DICT_END: {
//                 op_desc.read_simple_op();
//                 _c = write_ctn_end<__indent, __kind, true>(dst, buffer_info, cfg);
//                 if (unlikely(!_c)) {
//                     goto fail;
//                 }
//                 break;
//             }
//             case EncodeOpType::ARRAY_EMPTY: {
//                 op_desc.read_simple_op();
//                 _c = write_empty_ctn<__indent, __kind, false>(dst, buffer_info, cfg);
//                 if (unlikely(!_c)) {
//                     goto fail;
//                 }
//                 break;
//             }
//             case EncodeOpType::ARRAY_BEGIN: {
//                 op_desc.read_simple_op();
//                 _c = write_ctn_begin<__indent, __kind, false>(dst, buffer_info, cfg);
//                 if (unlikely(!_c)) {
//                     goto fail;
//                 }
//                 break;
//             }
//             case EncodeOpType::ARRAY_END: {
//                 op_desc.read_simple_op();
//                 _c = write_ctn_end<__indent, __kind, false>(dst, buffer_info, cfg);
//                 if (unlikely(!_c)) {
//                     goto fail;
//                 }
//                 break;
//             }
//             case EncodeOpType::STRING: {
//                 PyObject *unicode = op_desc.read_op_data<PyObject *>();
//                 _c = write_str<__indent, __kind, __simd_level>(unicode, dst, buffer_info, cfg);
//                 if (unlikely(!_c)) {
//                     goto fail;
//                 }
//                 break;
//             }
//             case EncodeOpType::KEY: {
//                 PyObject *unicode = op_desc.read_op_data<PyObject *>();
//                 _c = write_key<__indent, __kind, __simd_level>(unicode, dst, buffer_info, cfg);
//                 if (unlikely(!_c)) {
//                     goto fail;
//                 }
//                 break;
//             }
//             case EncodeOpType::U64: {
//                 u64 val = op_desc.read_op_data<u64>();
//                 _c = write_u64<__indent, __kind>(val, dst, buffer_info, cfg);
//                 if (unlikely(!_c)) {
//                     goto fail;
//                 }
//                 break;
//             }
//             case EncodeOpType::I64: {
//                 i64 val = op_desc.read_op_data<i64>();
//                 _c = write_i64<__indent, __kind>(val, dst, buffer_info, cfg);
//                 if (unlikely(!_c)) {
//                     goto fail;
//                 }
//                 break;
//             }
//             case EncodeOpType::FLOAT: {
//                 double val = op_desc.read_op_data<double>();
//                 _c = write_float<__indent, __kind>(val, dst, buffer_info, cfg);
//                 if (unlikely(!_c)) {
//                     goto fail;
//                 }
//                 break;
//             }
//             case EncodeOpType::TRUE_VALUE: {
//                 op_desc.read_simple_op();
//                 _c = write_true<__indent, __kind>(dst, buffer_info, cfg);
//                 if (unlikely(!_c)) {
//                     goto fail;
//                 }
//                 break;
//             }
//             case EncodeOpType::FALSE_VALUE: {
//                 op_desc.read_simple_op();
//                 _c = write_false<__indent, __kind>(dst, buffer_info, cfg);
//                 if (unlikely(!_c)) {
//                     goto fail;
//                 }
//                 break;
//             }
//             case EncodeOpType::NULL_VALUE: {
//                 op_desc.read_simple_op();
//                 _c = write_null<__indent, __kind>(dst, buffer_info, cfg);
//                 if (unlikely(!_c)) {
//                     goto fail;
//                 }
//                 break;
//             }
//             case EncodeOpType::POS_NAN: {
//                 op_desc.read_simple_op();
//                 _c = write_nan<__indent, __kind, false>(dst, buffer_info, cfg);
//                 if (unlikely(!_c)) {
//                     goto fail;
//                 }
//                 break;
//             }
//             case EncodeOpType::NEG_NAN: {
//                 op_desc.read_simple_op();
//                 _c = write_nan<__indent, __kind, true>(dst, buffer_info, cfg);
//                 if (unlikely(!_c)) {
//                     goto fail;
//                 }
//                 break;
//             }
//             case EncodeOpType::ZERO_VALUE: {
//                 op_desc.read_simple_op();
//                 _c = write_zero<__indent, __kind>(dst, buffer_info, cfg);
//                 if (unlikely(!_c)) {
//                     goto fail;
//                 }
//                 break;
//             }
//             case EncodeOpType::POS_INF: {
//                 op_desc.read_simple_op();
//                 _c = write_inf<__indent, __kind, false>(dst, buffer_info, cfg);
//                 if (unlikely(!_c)) {
//                     goto fail;
//                 }
//                 break;
//             }
//             case EncodeOpType::NEG_INF: {
//                 op_desc.read_simple_op();
//                 _c = write_inf<__indent, __kind, true>(dst, buffer_info, cfg);
//                 if (unlikely(!_c)) {
//                     goto fail;
//                 }
//                 break;
//             }
//             case EncodeOpType::END: {
//                 goto success;
//             }
//             default: {
//                 Py_UNREACHABLE();
//                 assert(false);
//             }
//         }
//     }

// success:
//     dst--;
//     final_size = dst - (udst *) buffer_info.m_buffer_start;
//     assert(final_size >= 0);

//     if constexpr (__kind == UCSKind::UCS1) {
//         if (cfg.is_all_ascii) {
//             max_char = 0x80;
//         } else {
//             max_char = 0xff;
//         }
//     } else if constexpr (__kind == UCSKind::UCS2) {
//         max_char = 0xffff;
//     } else {
//         max_char = 0x10ffff;
//     }
//     ret = PyUnicode_New(final_size, max_char);
//     if (unlikely(!ret)) {
//         goto fail;
//     }
//     cpy_fast<__simd_level>(PyUnicode_DATA(ret), buffer_info.m_buffer_start, final_size * sizeof(udst), final_size * sizeof(udst));
//     // memcpy(PyUnicode_DATA(ret), buffer_info.m_buffer_start, final_size * sizeof(udst));
//     buffer_info.release();
//     return ret;
// fail:
//     buffer_info.release();
//     return nullptr;
// }


// TODO
#define CTN_SIZE_GROW()                                                      \
    do {                                                                     \
        if (unlikely(cur_nested_depth == PYYJSON_ENCODE_MAX_RECURSION)) {    \
            PyErr_SetString(PyExc_ValueError, "Too many nested structures"); \
            return JumpFlag_Fail;                                            \
        }                                                                    \
    } while (0)

#define TODO() assert(false)


#define ALIAS_NAMES()                              \
    auto &cur_nested_depth = cfg.cur_nested_depth; \
    auto &cur_obj = cfg.cur_obj;                   \
    auto &cur_pos = cfg.cur_pos;                   \
    auto &cur_list_size = cfg.cur_list_size;       \
    auto &cur_ucs_kind = cfg.cur_ucs_kind;         \
    auto &key = cfg.key;                           \
    auto &val = cfg.val


force_inline void update_string_info(EncodeWriteOpConfig &cfg, PyObject *val) {
    cfg.cur_ucs_kind = std::max(cfg.cur_ucs_kind, static_cast<UCSKind>(PyUnicode_KIND(val)));
    cfg.is_all_ascii = cfg.is_all_ascii && PyUnicode_IS_ASCII(val);
}

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


// #define PROCESS_VAL(val)                                                 \
//     PyFastTypes fast_type = fast_type_check(val);                        \
//     switch (fast_type) {                                                 \
//         case PyFastTypes::T_Unicode: {                                   \
//             update_string_info(cfg, val);                                \
//             op_desc.write_op_with_data(EncodeOpType::STRING, val);       \
//             break;                                                       \
//         }                                                                \
//         case PyFastTypes::T_Long: {                                      \
//             if (unlikely(!write_PyLongOp(val, op_desc))) {               \
//                 goto fail;                                               \
//             }                                                            \
//             break;                                                       \
//         }                                                                \
//         case PyFastTypes::T_False: {                                     \
//             op_desc.write_simple_op(EncodeOpType::FALSE_VALUE);          \
//             break;                                                       \
//         }                                                                \
//         case PyFastTypes::T_True: {                                      \
//             op_desc.write_simple_op(EncodeOpType::TRUE_VALUE);           \
//             break;                                                       \
//         }                                                                \
//         case PyFastTypes::T_None: {                                      \
//             op_desc.write_simple_op(EncodeOpType::NULL_VALUE);           \
//             break;                                                       \
//         }                                                                \
//         case PyFastTypes::T_Float: {                                     \
//             if (unlikely(!write_PyFloatOp(val, op_desc))) {              \
//                 goto fail;                                               \
//             }                                                            \
//             break;                                                       \
//         }                                                                \
//         case PyFastTypes::T_List: {                                      \
//             Py_ssize_t this_list_size = PyList_Size(val);                \
//             if (unlikely(cur_list_size == 0)) {                          \
//                 op_desc.write_simple_op(EncodeOpType::ARRAY_EMPTY);      \
//             } else {                                                     \
//                 CTN_SIZE_GROW();                                         \
//                 CtnType *cur_write_ctn = _CtnStack + ++cur_nested_depth; \
//                 cur_write_ctn->ctn = cur_obj;                            \
//                 cur_write_ctn->index = cur_pos;                          \
//                 cur_obj = val;                                           \
//                 cur_pos = 0;                                             \
//                 op_desc.write_simple_op(EncodeOpType::ARRAY_BEGIN);      \
//                 GOTO_ARR_VAL_BEGIN_WITH_SIZE(this_list_size);            \
//             }                                                            \
//             break;                                                       \
//         }                                                                \
//         case PyFastTypes::T_Dict: {                                      \
//             if (unlikely(PyDict_Size(val) == 0)) {                       \
//                 op_desc.write_simple_op(EncodeOpType::DICT_EMPTY);       \
//             } else {                                                     \
//                 CTN_SIZE_GROW();                                         \
//                 CtnType *cur_write_ctn = _CtnStack + ++cur_nested_depth; \
//                 cur_write_ctn->ctn = cur_obj;                            \
//                 cur_write_ctn->index = cur_pos;                          \
//                 cur_obj = val;                                           \
//                 cur_pos = 0;                                             \
//                 op_desc.write_simple_op(EncodeOpType::DICT_BEGIN);       \
//                 goto dict_pair_begin;                                    \
//             }                                                            \
//             break;                                                       \
//         }                                                                \
//         case PyFastTypes::T_Tuple: {                                     \
//             Py_ssize_t this_list_size = PyTuple_Size(val);               \
//             if (unlikely(this_list_size == 0)) {                         \
//                 op_desc.write_simple_op(EncodeOpType::ARRAY_EMPTY);      \
//             } else {                                                     \
//                 CTN_SIZE_GROW();                                         \
//                 CtnType *cur_write_ctn = _CtnStack + ++cur_nested_depth; \
//                 cur_write_ctn->ctn = cur_obj;                            \
//                 cur_write_ctn->index = cur_pos;                          \
//                 cur_obj = val;                                           \
//                 cur_pos = 0;                                             \
//                 op_desc.write_simple_op(EncodeOpType::ARRAY_BEGIN);      \
//                 GOTO_TUPLE_VAL_BEGIN_WITH_SIZE(this_list_size);          \
//             }                                                            \
//             break;                                                       \
//         }                                                                \
//         default: {                                                       \
//             PyErr_SetString(JSONEncodeError, "Unsupported type");        \
//             goto fail;                                                   \
//         }                                                                \
//     }

struct DumpOption {
    IndentLevel indent_level;
};

constexpr X86SIMDLevel check_simd_level() {
    // TODO
    return X86SIMDLevel::AVX2;
}

// #define JUMP_BY_UCS_TYPE(_indent_)                                                                            \
//     switch (op_cfg->cur_ucs_kind) {                                                                           \
//         case UCSKind::UCS1: {                                                                                 \
//             return pyyjson_dumps_read_op_write_str<_indent_, UCSKind::UCS1, simd_level>(op_cfg, op_desc_old); \
//         }                                                                                                     \
//         case UCSKind::UCS2: {                                                                                 \
//             return pyyjson_dumps_read_op_write_str<_indent_, UCSKind::UCS2, simd_level>(op_cfg, op_desc_old); \
//         }                                                                                                     \
//         case UCSKind::UCS4: {                                                                                 \
//             return pyyjson_dumps_read_op_write_str<_indent_, UCSKind::UCS4, simd_level>(op_cfg, op_desc_old); \
//         }                                                                                                     \
//         default: {                                                                                            \
//             Py_UNREACHABLE();                                                                                 \
//             break;                                                                                            \
//         }                                                                                                     \
//     }

// PyObject *pyyjson_dumps_read_op_write_str_call_jump(DumpOption *options, EncodeWriteOpConfig *op_cfg, EncodeOpDescriber *op_desc_old) {
//     constexpr X86SIMDLevel simd_level = check_simd_level();
//     switch (options->indent_level) {
//         case IndentLevel::NONE: {
//             JUMP_BY_UCS_TYPE(IndentLevel::NONE);
//         }
//         case IndentLevel::INDENT_2: {
//             JUMP_BY_UCS_TYPE(IndentLevel::INDENT_2);
//         }
//         case IndentLevel::INDENT_4: {
//             JUMP_BY_UCS_TYPE(IndentLevel::INDENT_4);
//         }
//     }

//     Py_UNREACHABLE();
//     return nullptr;
// }

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

#define INIT_FROM_THREAD_LOCAL_BUFFER(viewer, rw_ptr, end_ptr, tls_buffer, offset) \
    do {                                                                           \
        viewer->rw_ptr = (decltype(viewer->rw_ptr)) &tls_buffer[0];                \
        viewer->end_ptr = viewer->rw_ptr + (offset);                               \
    } while (0)

force_inline void init_viewer(ObjViewer *viewer) {
    viewer->m_unicode_count = 0;
    viewer->m_unicode_kind = UCSKind::UCS1;
    viewer->m_is_all_ascii = true;
    viewer->m_cur_depth = 0;
    //
    viewer->m_obj_view_buffer_count = 1;
    viewer->m_obj_view_cur_rw_index = 0;
    INIT_FROM_THREAD_LOCAL_BUFFER(viewer, m_cur_obj_view_rw_ptr, m_cur_obj_view_end_ptr, _ObjViewFirstBuffer, PYYJSON_ENCODE_OBJ_VIEW_BUFFER_INIT_SIZE);
    //
    viewer->m_data_buffer_count = 1;
    viewer->m_data_cur_rw_index = 0;
    INIT_FROM_THREAD_LOCAL_BUFFER(viewer, m_cur_data_rw_ptr, m_cur_data_end_ptr, _ViewDataFirstBuffer, PYYJSON_ENCODE_VIEW_DATA_BUFFER_INIT_SIZE);
}

force_inline Py_ssize_t get_view_buffer_length(usize index) {
    return PYYJSON_ENCODE_OBJ_VIEW_BUFFER_INIT_SIZE << index;
}

force_inline Py_ssize_t get_data_buffer_length(usize index) {
    return PYYJSON_ENCODE_VIEW_DATA_BUFFER_INIT_SIZE << index;
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
            return is_all_ascii ? 0x80 : 0xff;
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

force_inline bool read_is_dict_from_view(ObjViewer *viewer) {
    return (bool) viewer->m_view_payload;
}

force_inline bool read_is_unicode_escaped(ObjViewer *viewer) {
    return (bool) viewer->m_view_payload;
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
                _mm_store_si128((__m128i_u *) dst, _mm_load_si128((const __m128i *) aligned_src));
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


template<IndentSetting _IndentSetting, UCSKind _Kind>
void write_newline_and_indent_from_view(ObjViewer *viewer, UCSType_t<_Kind> *&write_ptr, UCSType_t<_Kind> *const end_ptr) {
    using uu = UCSType_t<_Kind>;

    if constexpr (!need_indent(_IndentSetting)) {
        return;
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

template<UCSKind _Kind>
force_inline void write_ctn_end(ObjViewer *viewer, UCSType_t<_Kind> *&write_ptr, UCSType_t<_Kind> *const end_ptr) {
    using uu = UCSType_t<_Kind>;

    // need to erase last comma
    *(write_ptr - 1) = (uu) ']' | ((uu) viewer->m_is_in_obj << 5);
    if (unlikely(write_ptr >= end_ptr)) return;
    *write_ptr++ = (uu) ',';
}

template<UCSKind _Kind>
void copy_raw_unicode_with_escape(ObjViewer *viewer, PyObject *unicode, UCSType_t<_Kind> *&write_ptr, UCSType_t<_Kind> *const end_ptr) {
    // TODO
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
            copy_elevate_nocheck<UCSKind::UCS1, UCSKind::UCS2>(write_ptr, (u8 *) PyUnicode_DATA(unicode), len, end_ptr - write_ptr);
        } else {
            assert(src_kind == UCSKind::UCS2);
            cpy_fast<_SIMDLevel>((void *) write_ptr, (void *) PyUnicode_DATA(unicode), len * 2, (Py_ssize_t) end_ptr - (Py_ssize_t) write_ptr);
            write_ptr += len;
        }
    } else {
        static_assert(_Kind == UCSKind::UCS4);
        if (src_kind == UCSKind::UCS1) {
            copy_elevate_nocheck<UCSKind::UCS1, UCSKind::UCS4>(write_ptr, (u8 *) PyUnicode_DATA(unicode), len, end_ptr - write_ptr);
        } else if (src_kind == UCSKind::UCS2) {
            copy_elevate_nocheck<UCSKind::UCS2, UCSKind::UCS4>(write_ptr, (u16 *) PyUnicode_DATA(unicode), len, end_ptr - write_ptr);
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

    return len;
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

template<IndentSetting _IndentSetting, UCSKind _Kind>
force_inline void write_key(ObjViewer *viewer, PyObject *key, UCSType_t<_Kind> *&write_ptr, UCSType_t<_Kind> *const end_ptr) {
    using uu = UCSType_t<_Kind>;
    assert(PyUnicode_CheckExact(key));
    *write_ptr++ = (uu) '"';
    bool has_escape = read_is_unicode_escaped(viewer);
    if (unlikely(has_escape)) {
        copy_raw_unicode_with_escape(viewer, key, write_ptr, end_ptr);
    } else {
        copy_raw_unicode(viewer, key, write_ptr, end_ptr);
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
    ViewObjType_I64,
    ViewObjType_U64,
    ViewObjType_True,
    ViewObjType_False,
    ViewObjType_PyNull,
    ViewObjType_Double,
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


template<IndentSetting _IndentSetting, bool _IsWriteObj>
force_inline bool view_add_empty_ctn(ObjViewer *viewer, Py_ssize_t cur_nested_depth, bool is_inside_obj) {
    bool _c = write_obj_view(viewer, (u32) (_IsWriteObj ? ViewObjType_EmptyObj : ViewObjType_EmptyArr));
    if constexpr (need_indent(_IndentSetting)) {
        if (!is_inside_obj) {
            viewer->m_unicode_count += 1 + _get_indent_space_count(cur_nested_depth, _IndentSetting);
        }
    }
    viewer->m_unicode_count += 3;
    return _c;
}

template<IndentSetting _IndentSetting, bool _IsWriteObj>
force_inline bool view_add_ctn_begin(ObjViewer *viewer, Py_ssize_t cur_nested_depth, bool is_inside_obj) {
    bool _c = write_obj_view(viewer, (u32) (_IsWriteObj ? ViewObjType_ObjBegin : ViewObjType_ArrBegin));
    if constexpr (need_indent(_IndentSetting)) {
        if (!is_inside_obj) {
            viewer->m_unicode_count += 1 + _get_indent_space_count(cur_nested_depth, _IndentSetting);
        }
    }
    viewer->m_unicode_count += 1;
    return _c;
}

force_inline void view_update_str_info(ObjViewer *viewer, PyObject *str) {
    assert(PyUnicode_CheckExact(str));
    viewer->m_unicode_kind = std::max<UCSKind>(viewer->m_unicode_kind, (UCSKind) PyUnicode_KIND(str));
    assert(viewer->m_unicode_kind <= UCSKind::UCS4);
    viewer->m_is_all_ascii = viewer->m_is_all_ascii && PyUnicode_IS_ASCII(str);
}

template<IndentSetting _IndentSetting>
force_inline bool view_add_key(ObjViewer *viewer, PyObject *str, Py_ssize_t cur_nested_depth) {
    assert(PyUnicode_CheckExact(str));
    Py_ssize_t escaped_len = scan_unicode_escaped_len(str);
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

template<IndentSetting _IndentSetting, X86SIMDLevel _SIMDLevel>
force_inline bool view_add_str(ObjViewer *viewer, PyObject *str, Py_ssize_t cur_nested_depth, bool is_inside_obj) {
    assert(PyUnicode_CheckExact(str));
    Py_ssize_t escaped_len = scan_unicode_escaped_len<_SIMDLevel>(str);
    bool has_escape = escaped_len > PyUnicode_GET_LENGTH(str);
    u32 view = (u32) ViewObjType_Str | ((u32) has_escape << VIEW_PAYLOAD_BIT);
    bool _c = write_obj_view(viewer, view) && write_view_data<PyObject *>(viewer, str);
    if (unlikely(has_escape)) {
        _c = _c && write_view_data<Py_ssize_t>(viewer, escaped_len);
    }
    if constexpr (need_indent(_IndentSetting)) {
        if (!is_inside_obj) {
            // indent and spaces, two quotes, unicode, and one comma
            viewer->m_unicode_count += 1 + _get_indent_space_count(cur_nested_depth, _IndentSetting) + 3 + escaped_len;
        } else {
            // two quotes, unicode, and one comma
            viewer->m_unicode_count += 3 + escaped_len;
        }
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

template<IndentSetting _IndentSetting, X86SIMDLevel _SIMDLevel>
force_inline EncodeValJumpFlag view_process_val(
        PyObject *val,
        ObjViewer *obj_viewer,
        CtnType *const ctn_stack,
        Py_ssize_t &cur_nested_depth,
        PyObject *&cur_obj,
        Py_ssize_t &cur_pos,
        Py_ssize_t &cur_list_size, bool is_inside_obj) {
    PyFastTypes fast_type = fast_type_check(val);
    switch (fast_type) {
        case PyFastTypes::T_Unicode: {
            view_update_str_info(obj_viewer, val);
            view_add_str<_IndentSetting, _SIMDLevel>(obj_viewer, val, is_inside_obj);
            break;
        }
        case PyFastTypes::T_Long: {
            view_add_pylong<_IndentSetting>(obj_viewer, val);
            break;
        }
        case PyFastTypes::T_False: {
            view_add_false<_IndentSetting>(obj_viewer);
            break;
        }
        case PyFastTypes::T_True: {
            view_add_true<_IndentSetting>(obj_viewer);
            break;
        }
        case PyFastTypes::T_None: {
            view_add_null<_IndentSetting>(obj_viewer);
            break;
        }
        case PyFastTypes::T_Float: {
            view_add_pyfloat<_IndentSetting>(obj_viewer);
            break;
        }
        case PyFastTypes::T_List: {
            Py_ssize_t this_list_size = PyList_Size(val);
            if (unlikely(this_list_size == 0)) {
                view_add_empty_ctn<_IndentSetting, false>(obj_viewer);
            } else {
                view_add_ctn_begin<_IndentSetting, false>(obj_viewer);
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
            if (unlikely(PyDict_Size(val) == 0)) {
                view_add_empty_ctn<_IndentSetting, true>(obj_viewer);
            } else {
                view_add_ctn_begin<_IndentSetting, true>(obj_viewer);
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
                view_add_empty_ctn<_IndentSetting, false>(obj_viewer);
            } else {
                view_add_ctn_begin<_IndentSetting, false>(obj_viewer);
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
}

template<IndentSetting _IndentSetting, X86SIMDLevel _SIMDLevel>
force_inline bool
pyyjson_dumps_obj_to_view(PyObject *in_obj, ObjViewer *obj_viewer) {
    // cache
    Py_ssize_t cur_list_size;
    PyObject *key, *val;
    PyObject *cur_obj = in_obj;
    Py_ssize_t cur_pos = 0;
    Py_ssize_t cur_nested_depth = 0;
    // alias thread local buffer
    CtnType *const ctn_stack = _CtnStack;

    if (PyDict_CheckExact(cur_obj)) {
        if (unlikely(PyDict_Size(cur_obj) == 0)) {
            view_add_empty_ctn<_IndentSetting, true>(obj_viewer);
            goto success;
        }
        view_add_ctn_begin<_IndentSetting, true>(obj_viewer);
        assert(!cur_nested_depth);
        cur_nested_depth = 1;
        // NOTE: ctn_stack[0] is always invalid
        goto dict_pair_begin;
    } else if (PyList_CheckExact(cur_obj)) {
        cur_list_size = PyList_GET_SIZE(cur_obj);
        if (unlikely(cur_list_size == 0)) {
            view_add_empty_ctn<_IndentSetting, false>(obj_viewer);
            goto success;
        }
        view_add_ctn_begin<_IndentSetting, false>(obj_viewer);
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
            view_add_empty_ctn<_IndentSetting, false>(obj_viewer);
            goto success;
        }
        view_add_ctn_begin<_IndentSetting, false>(obj_viewer);
        assert(!cur_nested_depth);
        cur_nested_depth = 1;
        goto tuple_val_begin;
    }

    Py_UNREACHABLE();

dict_pair_begin:;
    assert(PyDict_Size(cur_obj) != 0);

    if (PyDict_Next(cur_obj, &cur_pos, &key, &val)) {
        if (unlikely(!PyUnicode_CheckExact(key))) {
            goto fail_keytype;
        }
        view_update_str_info(obj_viewer, key);
        view_add_key(obj_viewer, key);
        //
        auto jump_flag = view_process_val<_IndentSetting, X86SIMDLevel>(val,
                                                                        obj_viewer,
                                                                        ctn_stack,
                                                                        cur_nested_depth,
                                                                        cur_obj,
                                                                        cur_pos,
                                                                        cur_list_size,
                                                                        true);
        JUMP_BY_FLAG(jump_flag);
        goto dict_pair_begin;
    } else {
        // dict end
        assert(cur_nested_depth);
        CtnType *last_pos = ctn_stack + (--cur_nested_depth);

        view_add_ctn_end<true>(obj_viewer);
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
        auto jump_flag = view_process_val<_IndentSetting, _SIMDLevel>(val,
                                                                      obj_viewer,
                                                                      ctn_stack,
                                                                      cur_nested_depth,
                                                                      cur_obj,
                                                                      cur_pos,
                                                                      cur_list_size,
                                                                      false);
        JUMP_BY_FLAG(jump_flag);
        //
        goto arr_val_begin;
    } else {
        // list end
        assert(cur_nested_depth);
        CtnType *last_pos = ctn_stack + (--cur_nested_depth);

        view_add_ctn_end<false>(obj_viewer);
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
        auto jump_flag = view_process_val<_IndentSetting, _SIMDLevel>(val,
                                                                      obj_viewer,
                                                                      ctn_stack,
                                                                      cur_nested_depth,
                                                                      cur_obj,
                                                                      cur_pos,
                                                                      cur_list_size,
                                                                      false);
        JUMP_BY_FLAG(jump_flag);
        //
        goto tuple_val_begin;
    } else {
        // list end
        assert(cur_nested_depth);
        CtnType *last_pos = ctn_stack + (--cur_nested_depth);

        view_add_ctn_end<_IndentSetting, false>(obj_viewer);
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
    PyObject *ret = PyUnicode_New(ucs_len, get_max_char_from_ucs_kind(_Kind, obj_viewer->m_is_all_ascii));
    if (unlikely(!ret)) return NULL;
    uu *write_ptr = (uu *) PyUnicode_DATA(ret);
    uu *const end_ptr = write_ptr + ucs_len;
    while (1) {
        switch (view_obj(obj_viewer)) {
            case ViewObjType_Done: {
                goto success;
            }
            case ViewObjType_EmptyCtn: {
                bool is_dict = read_is_dict_from_view(obj_viewer);
                write_newline_and_indent_from_view<_IndentSetting, _Kind>(obj_viewer, write_ptr, end_ptr);
                if (is_dict) {
                    write_empty_ctn<true>(obj_viewer, write_ptr, end_ptr);
                } else {
                    write_empty_ctn<false>(obj_viewer, write_ptr, end_ptr);
                }
                break;
            }
            case ViewObjType_CtnBegin: {
                bool is_dict = read_is_dict_from_view(obj_viewer);
                write_newline_and_indent_from_view(obj_viewer, write_ptr, end_ptr);
                if (is_dict) {
                    write_ctn_begin<true>(obj_viewer, write_ptr, end_ptr);
                } else {
                    write_ctn_begin<false>(obj_viewer, write_ptr, end_ptr);
                }
                // then increase indent
                view_increase_indent(obj_viewer, is_dict);
                break;
            }
            case ViewObjType_CtnEnd: {
                // decrease indent first
                view_decrease_indent(obj_viewer);
                write_newline_and_indent_from_view(obj_viewer, write_ptr, end_ptr);
                write_ctn_end(obj_viewer, write_ptr, end_ptr);
                break;
            }
            case ViewObjType_Key: {
                write_newline_and_indent_from_view(obj_viewer, write_ptr, end_ptr);
                PyObject *key = read_view_data<PyObject *>(obj_viewer);
                write_key(obj_viewer, key, write_ptr, end_ptr);
                break;
            }
            case ViewObjType_Str: {
                write_newline_and_indent_from_view(obj_viewer, write_ptr, end_ptr);
                PyObject *str = read_view_data<PyObject *>(obj_viewer);
                write_str(obj_viewer, str, write_ptr, end_ptr);
                break;
            }
            case ViewObjType_Zero: {
                write_newline_and_indent_from_view(obj_viewer, write_ptr, end_ptr);
                write_zero(obj_viewer, write_ptr, end_ptr);
                break;
            }
            case ViewObjType_I64: {
                write_newline_and_indent_from_view(obj_viewer, write_ptr, end_ptr);
                i64 val = read_view_data<i64>(obj_viewer);
                write_i64(obj_viewer, val, write_ptr, end_ptr);
                break;
            }
            case ViewObjType_U64: {
                write_newline_and_indent_from_view(obj_viewer, write_ptr, end_ptr);
                u64 val = read_view_data<u64>(obj_viewer);
                write_u64(obj_viewer, val, write_ptr, end_ptr);
                break;
            }
            case ViewObjType_True: {
                write_newline_and_indent_from_view(obj_viewer, write_ptr, end_ptr);
                write_true(obj_viewer, write_ptr, end_ptr);
                break;
            }
            case ViewObjType_False: {
                write_newline_and_indent_from_view(obj_viewer, write_ptr, end_ptr);
                write_false(obj_viewer, write_ptr, end_ptr);
                break;
            }
            case ViewObjType_PyNull: {
                write_newline_and_indent_from_view(obj_viewer, write_ptr, end_ptr);
                write_null(obj_viewer, write_ptr, end_ptr);
                break;
            }
            case ViewObjType_Double: {
                write_newline_and_indent_from_view(obj_viewer, write_ptr, end_ptr);
                double val = read_view_data<double>(obj_viewer);
                write_double(obj_viewer, val, write_ptr, end_ptr);
                break;
            }
            case ViewObjType_PosNaN: {
                write_newline_and_indent_from_view(obj_viewer, write_ptr, end_ptr);
                write_pos_nan(obj_viewer, write_ptr, end_ptr);
                break;
            }
            case ViewObjType_NegNaN: {
                write_newline_and_indent_from_view(obj_viewer, write_ptr, end_ptr);
                write_neg_nan(obj_viewer, write_ptr, end_ptr);
                break;
            }
            case ViewObjType_PosInf: {
                write_newline_and_indent_from_view(obj_viewer, write_ptr, end_ptr);
                write_pos_inf(obj_viewer, write_ptr, end_ptr);
                break;
            }
            case ViewObjType_NegInf: {
                write_newline_and_indent_from_view(obj_viewer, write_ptr, end_ptr);
                write_neg_inf(obj_viewer, write_ptr, end_ptr);
                break;
            }
            //... TODO
            default: {
                Py_UNREACHABLE();
            }
        }
    }

success:;
    assert(write_ptr == end_ptr);
    // TODO add assertions
    return ret;
}

force_noinline PyObject *pyyjson_dumps_obj(PyObject *in_obj) {
    ObjViewer obj_viewer;
    init_viewer(&obj_viewer);
    PyObject *ret = NULL;
    //
    bool success = pyyjson_dumps_obj_to_view<IndentSetting::INDENT_4, X86SIMDLevel::AVX2>(in_obj, &obj_viewer);
    UCSKind kind;

    if (unlikely(!success)) goto finalize;

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
    clear_view(obj_viewer);

    return ret;
}

// force_noinline PyObject *pyyjson_dumps_obj_op(PyObject *in_obj, DumpOption *options) {
//     EncodeWriteOpConfig cfg;
//     ALIAS_NAMES();
//     EncodeOpDescriber op_desc = EncodeOpDescriber::init();

//     cur_obj = in_obj;
//     cur_pos = 0;

//     // final
//     PyObject *result;

//     if (PyDict_CheckExact(cur_obj)) {
//         if (unlikely(PyDict_Size(cur_obj) == 0)) {
//             op_desc.write_simple_op(EncodeOpType::DICT_EMPTY);
//             // APPEND_OP(DICT_EMPTY);
//             goto success;
//         }
//         op_desc.write_simple_op(EncodeOpType::DICT_BEGIN);
//         // APPEND_OP(DICT_BEGIN);
//         cur_pos = 0;
//         goto dict_pair_begin;
//     } else if (PyList_CheckExact(cur_obj)) {
//         cur_list_size = PyList_Size(cur_obj);
//         if (unlikely(cur_list_size == 0)) {
//             // APPEND_OP(ARRAY_EMPTY);
//             op_desc.write_simple_op(EncodeOpType::ARRAY_EMPTY);
//             goto success;
//         }
//         op_desc.write_simple_op(EncodeOpType::ARRAY_BEGIN);
//         // APPEND_OP(ARRAY_BEGIN);
//         GOTO_ARR_VAL_BEGIN_WITH_SIZE(cur_list_size); // result in a no-op
//     } else {
//         if (unlikely(!PyTuple_CheckExact(cur_obj))) {
//             goto fail_ctntype;
//         }
//         cur_list_size = PyTuple_Size(cur_obj);
//         if (unlikely(cur_list_size == 0)) {
//             op_desc.write_simple_op(EncodeOpType::ARRAY_EMPTY);
//             // APPEND_OP(ARRAY_EMPTY);
//             goto success;
//         }
//         op_desc.write_simple_op(EncodeOpType::ARRAY_BEGIN);
//         // APPEND_OP(ARRAY_BEGIN);
//         GOTO_TUPLE_VAL_BEGIN_WITH_SIZE(cur_list_size); // result in a no-op
//     }

// dict_pair_begin:;
//     assert(PyDict_Size(cur_obj) != 0);

//     if (PyDict_Next(cur_obj, &cur_pos, &key, &val)) {
//         if (unlikely(!PyUnicode_CheckExact(key))) {
//             goto fail_keytype;
//         }
//         update_string_info(cfg, key);
//         op_desc.write_op_with_data(EncodeOpType::KEY, key);
//         //
//         PROCESS_VAL(val);
//         //
//         goto dict_pair_begin;
//     } else {
//         // dict end
//         op_desc.write_simple_op(EncodeOpType::DICT_END);
//         if (unlikely(cur_nested_depth == -1)) {
//             goto success;
//         }
//         CtnType *last_pos = _CtnStack + cur_nested_depth--;
//         cur_obj = last_pos->ctn;
//         cur_pos = last_pos->index;
//         if (PyDict_CheckExact(cur_obj)) {
//             goto dict_pair_begin;
//         } else if (PyList_CheckExact(cur_obj)) {
//             GOTO_ARR_VAL_BEGIN_WITH_SIZE(PyList_GET_SIZE(cur_obj));
//         } else {
//             assert(PyTuple_CheckExact(cur_obj));
//             GOTO_TUPLE_VAL_BEGIN_WITH_SIZE(PyTuple_GET_SIZE(cur_obj));
//         }
//     }

//     Py_UNREACHABLE();
// arr_val_begin:;
//     assert(cur_list_size != 0);

//     if (cur_pos < cur_list_size) {
//         val = PyList_GET_ITEM(cur_obj, cur_pos);
//         cur_pos++;
//         //
//         PROCESS_VAL(val);
//         //
//         goto arr_val_begin;
//     } else {
//         // list end
//         op_desc.write_simple_op(EncodeOpType::ARRAY_END);
//         if (unlikely(cur_nested_depth == -1)) {
//             goto success;
//         }
//         CtnType *last_pos = _CtnStack + cur_nested_depth--;
//         cur_obj = last_pos->ctn;
//         cur_pos = last_pos->index;
//         if (PyDict_CheckExact(cur_obj)) {
//             goto dict_pair_begin;
//         } else if (PyList_CheckExact(cur_obj)) {
//             GOTO_ARR_VAL_BEGIN_WITH_SIZE(PyList_GET_SIZE(cur_obj));
//         } else {
//             assert(PyTuple_CheckExact(cur_obj));
//             GOTO_TUPLE_VAL_BEGIN_WITH_SIZE(PyTuple_GET_SIZE(cur_obj));
//         }
//     }
//     Py_UNREACHABLE();
// tuple_val_begin:;
//     assert(cur_list_size != 0);

//     if (cur_pos < cur_list_size) {
//         val = PyTuple_GET_ITEM(cur_obj, cur_pos);
//         cur_pos++;
//         //
//         PROCESS_VAL(val);
//         //
//         goto tuple_val_begin;
//     } else {
//         // list end
//         op_desc.write_simple_op(EncodeOpType::ARRAY_END);
//         if (unlikely(cur_nested_depth == -1)) {
//             goto success;
//         }
//         CtnType *last_pos = _CtnStack + cur_nested_depth--;
//         cur_obj = last_pos->ctn;
//         cur_pos = last_pos->index;
//         if (PyDict_CheckExact(cur_obj)) {
//             goto dict_pair_begin;
//         } else if (PyList_CheckExact(cur_obj)) {
//             GOTO_ARR_VAL_BEGIN_WITH_SIZE(PyList_GET_SIZE(cur_obj));
//         } else {
//             assert(PyTuple_CheckExact(cur_obj));
//             GOTO_TUPLE_VAL_BEGIN_WITH_SIZE(PyTuple_GET_SIZE(cur_obj));
//         }
//     }
//     Py_UNREACHABLE();
// success:
//     assert(cur_nested_depth == -1);
//     op_desc.write_simple_op(EncodeOpType::END);
//     result = pyyjson_dumps_read_op_write_str_call_jump(options, &cfg, &op_desc);

//     // cleanup. TODO
//     if (!result && !PyErr_Occurred()) {
//         PyErr_SetString(JSONEncodeError, "Unknown encode error");
//     }
//     op_desc.release();
//     return result;

// fail:
//     if (!PyErr_Occurred()) {
//         PyErr_SetString(JSONEncodeError, "Unknown encode error");
//     }
//     op_desc.release();
//     return nullptr;

// fail_ctntype:
//     PyErr_SetString(JSONEncodeError, "Unknown container type");
//     goto fail;

// fail_keytype:
//     PyErr_SetString(JSONEncodeError, "Key must be a string");
//     goto fail;
// }

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
