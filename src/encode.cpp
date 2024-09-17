#include "encode.hpp"
#include "encode_avx2.inl"
#include "encode_avx512.inl"
#include "encode_sse2.inl"
#include "encode_sse4.inl"
#include "pyyjson_config.h"
#include "yyjson.h"
#include <stddef.h>

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
#define INDENT_SIZE(__indent) (static_cast<int>(__indent))
#define IDENT_WITH_NEWLINE_SIZE(__indent, _indent_level) ((INDENT_SIZE(__indent) != 0) ? (_indent_level * INDENT_SIZE(__indent) + 1) : 0)

template<UCSKind __kind>
struct SrcInfo {
    UCSType_t<__kind> *src_start;
    Py_ssize_t src_size;
};


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
            bool _c = copy_escape_fixsize<UCSKind::UCS2, UCSKind::UCS4, 128>(src, dst)
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
    } else {
        // AVX2 does not have better `check_escape` than SSE4, for 128bits.
        static_assert(__simd_level >= X86SIMDLevel::SSE4);
        if constexpr (__kind == UCSKind::UCS1) {
            return check_escape_u8_128_sse4_1(u);
        } else if constexpr (__kind == UCSKind::UCS2) {
            return check_escape_u16_128_sse4_1(u);
        } else {
            static_assert(__kind == UCSKind::UCS4);
            return check_escape_u32_128_sse4_1(u);
        }
    }
}


template<UCSKind __from, UCSKind __to>
force_inline bool copy_ucs_with_runtime_resize(const SrcInfo<__from> &src_info, UCSType_t<__to> *&dst, BufferInfo &buffer_info) {
    // alright, no simd support
    using usrc = UCSType_t<__from>;
    usrc *src = src_info.src_start;
    usrc *const src_end = src + static_cast<std::ptrdiff_t>(src_info.src_size);

    constexpr std::ptrdiff_t _ProcessCountOnce = 512 / 8 / sizeof(usrc);
    while (src_end - src >= _ProcessCountOnce) {
        auto additional_needed_count = (src_end - src - _ProcessCountOnce) + _ProcessCountOnce * 6;
        auto required_len_u8 = buffer_info.get_required_len_u8<__to>(dst, additional_needed_count);

        RETURN_ON_UNLIKELY_RESIZE_FAIL(__to, dst, buffer_info, required_len_u8);
        // if (unlikely(required_len_u8 > buffer_info.m_size)) {
        //     if (unlikely(!buffer_info.resizer<__to>(dst, required_len_u8))) {
        //         return false;
        //     }
        // }
        bool _c = copy_escape_fixsize<__from, __to, 512>(src, dst);
        RETURN_ON_UNLIKELY_ERR(!_c);
    }
    const auto left_count = src_end - src;
    assert(left_count < _ProcessCountOnce);
    auto required_len_u8 = buffer_info.get_required_len_u8<__to>(dst, left_count * 6);

    RETURN_ON_UNLIKELY_RESIZE_FAIL(__to, dst, buffer_info, required_len_u8);
    // if (unlikely(required_len_u8 > buffer_info.m_size)) {
    //     if (unlikely(!buffer_info.resizer<__to>(dst, required_len_u8))) {
    //         return false;
    //     }
    // }
    return copy_escape_impl<__from, __to>(src, dst, left_count);
}


template<UCSKind __from, UCSKind __to>
force_inline bool copy_ucs_elevate_avx512(const SrcInfo<__from> &src_info, UCSType_t<__to> *&dst, BufferInfo &buffer_info) {
    COMMON_TYPE_DEF();
    UCSType_t<__from> *src = src_info.src_start;
    usrc *const src_end = src + static_cast<std::ptrdiff_t>(src_info.src_size);
    // process 256 bits once
    constexpr std::ptrdiff_t _ProcessCountOnce = 256 / 8 / sizeof(usrc);
    while (src_end - src >= _ProcessCountOnce) {
        __m256i u;
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
    assert(left_count < _ProcessCountOnce);
    auto required_len_u8 = buffer_info.get_required_len_u8<__to>(dst, left_count * 6);
    RETURN_ON_UNLIKELY_RESIZE_FAIL(__to, dst, buffer_info, required_len_u8);
    // if (unlikely(required_len_u8 > buffer_info.m_size)) {
    //     if (unlikely(!buffer_info.resizer<__to>(dst, required_len_u8))) {
    //         return false;
    //     }
    // }
    return copy_escape_impl<__from, __to>(src, dst, left_count);
}

template<UCSKind __from, UCSKind __to, X86SIMDLevel __simd_level>
force_inline bool copy_ucs_elevate_sse4_or_avx2(const SrcInfo<__from> &src_info, UCSType_t<__to> *&dst, BufferInfo &buffer_info) {
    static_assert(__simd_level == X86SIMDLevel::AVX2 || __simd_level == X86SIMDLevel::SSE4);
    COMMON_TYPE_DEF();
    UCSType_t<__from> *src = src_info.src_start;
    usrc *const src_end = src + static_cast<std::ptrdiff_t>(src_info.src_size);
    // process 128 bits once
    constexpr std::ptrdiff_t _ProcessCountOnce = 128 / 8 / sizeof(usrc);
    while (src_end - src >= _ProcessCountOnce) {
        __m128i u;
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
    assert(left_count < _ProcessCountOnce);
    auto required_len_u8 = buffer_info.get_required_len_u8<__to>(dst, left_count * 6);
    RETURN_ON_UNLIKELY_RESIZE_FAIL(__to, dst, buffer_info, required_len_u8);
    // if (unlikely(required_len_u8 > buffer_info.m_size)) {
    //     if (unlikely(!buffer_info.resizer<__to>(dst, required_len_u8))) {
    //         return false;
    //     }
    // }
    return copy_escape_impl<__from, __to>(src, dst, left_count);
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

    while (src_end - src >= _ProcessCountOnce) {
        __m256i u;
        u = _mm256_lddqu_si256((__m256i *) src); // vlddqu, AVX
        bool v = check_escape_256bits<__kind, __simd_level>(u);
        if (likely(v)) {
            _mm256_storeu_si256((__m256i *) dst, u); // vmovdqu, AVX
            ptr_move_bits<__kind>(src, 256);
            ptr_move_bits<__kind>(dst, 256);
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
                ptr_move_bits<__kind>(src, 128);
                ptr_move_bits<__kind>(dst, 128);
            } else {
                bool _c = copy_escape_fixsize<__kind, __kind, 128>(src, dst);
                RETURN_ON_UNLIKELY_ERR(!_c);
            }
            u1 = _mm256_extracti128_si256(u, 1); // vextracti128, AVX2
            if (check_escape_128bits<__kind, __simd_level>(u1)) {
                _mm_storeu_si128((__m128i *) dst, u1); // vmovdqu, AVX
                ptr_move_bits<__kind>(src, 128);
                ptr_move_bits<__kind>(dst, 128);
            } else {
                bool _c = copy_escape_fixsize<__kind, __kind, 128>(src, dst);
                RETURN_ON_UNLIKELY_ERR(!_c);
            }
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
            ptr_move_bits<__kind>(src, 128);
            ptr_move_bits<__kind>(dst, 128);
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

thread_local CtnType ctn_stack[1024];
thread_local EncodeOpBufferLinkedList op_buffer_head[128];

template<IndentLevel __indent, UCSKind __kind, size_t __additional_reserve>
force_inline bool write_line_and_indent(UCSType_t<__kind> *&dst, BufferInfo &buffer_info, const EncodeConfig &cfg) {
    static_assert(NEED_INDENT(__indent));
    using uu = UCSType_t<__kind>;
    Py_ssize_t need_count = IDENT_WITH_NEWLINE_SIZE(__indent, cfg.indent_level) + __additional_reserve;
    Py_ssize_t required_len_u8 = buffer_info.get_required_len_u8<__kind>(dst, need_count);
    RETURN_ON_UNLIKELY_RESIZE_FAIL(__kind, dst, buffer_info, required_len_u8);

    *dst++ = '\n';
    need_count--;
    while (need_count >= 4) {
        constexpr uu _Template[4] = {' ', ' ', ' ', ' '};
        memcpy((void *) dst, (void *) &_Template[0], sizeof(_Template));
        dst += 4;
        need_count -= 4;
    }
    while (need_count > 0) {
        *dst++ = ' ';
        need_count--;
    }
}

template<UCSKind __kind>
force_inline bool write_reserve_impl(UCSType_t<__kind> *&dst, BufferInfo &buffer_info, size_t reserve_count) {
    using uu = UCSType_t<__kind>;
    auto required_len_u8 = buffer_info.get_required_len_u8<__kind>(dst, reserve_count);
    RETURN_ON_UNLIKELY_RESIZE_FAIL(__kind, dst, buffer_info, required_len_u8);
    return true;
}

template<UCSKind __kind, size_t __reserve>
force_inline bool write_reserve(UCSType_t<__kind> *&dst, BufferInfo &buffer_info) {
    return write_reserve_impl<__kind>(dst, buffer_info, __reserve);
}

template<IndentLevel __indent, UCSKind __kind, bool is_obj>
force_inline bool write_empty_ctn(UCSType_t<__kind> *&dst, BufferInfo &buffer_info, const EncodeConfig &cfg) {
    using uu = UCSType_t<__kind>;
    // buffer size check
    bool _c;
    if constexpr (!is_obj && NEED_INDENT(__indent)) {
        _c = write_line_and_indent<__indent, __kind, 4>(dst, buffer_info, cfg);
    } else {
        _c = write_reserve<__kind, 4>(dst, buffer_info, cfg);
    }
    RETURN_ON_UNLIKELY_ERR(!_c);
    //
    constexpr uu _Template[4] = {
            (uu) ('[' | ((u8) is_obj << 5)),
            (uu) (']' | ((u8) is_obj << 5)),
            (uu) ',',
            0,
    };
    memcpy(dst, _Template, sizeof(_Template));
    dst += 3;
    return true;
}

template<IndentLevel __indent, UCSKind __kind, bool is_obj>
force_inline bool write_ctn_begin(UCSType_t<__kind> *&dst, BufferInfo &buffer_info, const EncodeConfig &cfg) {
    using uu = UCSType_t<__kind>;
    // ut *dst = (ut *) *dst_raw;
    bool _c;
    if constexpr (!is_obj && NEED_INDENT(__indent)) {
        _c = write_line_and_indent<__indent, __kind, 1>(dst, buffer_info, cfg);
    } else {
        _c = write_reserve<__kind, 1>(dst, buffer_info, cfg);
    }
    RETURN_ON_UNLIKELY_ERR(!_c);
    constexpr uu to_write = (uu) ('[' | ((u8) is_obj << 5));
    *dst++ = to_write;
    // write current ctn type
    cfg.set_cur_ctn(is_obj);
    // SET_CUR_CTN(is_obj);
    // new line, increase indent
    cfg.increase_indent();
    // INCREASE_INDENT();
    // *dst_raw = (void *) dst;
}

template<IndentLevel __indent, UCSKind __kind, bool is_obj, X86SIMDLevel __simd_level>
force_inline bool write_key(PyObject *key, UCSType_t<__kind> *&dst, BufferInfo &buffer_info, const EncodeConfig &cfg) {
    using uu = UCSType_t<__kind>;
    assert(PyUnicode_Check(key));
    ut *dst = (ut *) *dst_raw;
    if constexpr (NEED_INDENT(__indent)) {
        //     WRITE_LINE();
        // WRITE_INDENT();
        bool _c;
        _c = write_line_and_indent<__indent, __kind, 1>(dst, buffer_info, cfg);
        RETURN_ON_UNLIKELY_ERR(!_c);
    }


    *(dst++) = (ut) '"';
    write_raw_string(dst_raw, data_raw, ucs_kind, len);
    *(dst++) = (ut) '"';
    *(dst++) = (ut) ':';
    *(dst++) = (ut) ' ';
    *dst_raw = (void *) dst;
}


#define GOTO_ARR_VAL_BEGIN_WITH_SIZE(in_size) \
    do {                                      \
        cur_list_size = (in_size);            \
        goto arr_val_begin;                   \
    } while (0)
#define GOTO_TUPLE_VAL_BEGIN_WITH_SIZE(in_size) \
    do {                                        \
        cur_list_size = (in_size);              \
        goto tuple_val_begin;                   \
    } while (0)


// Py_ssize_t last_pos_stack_index = -1;
//     PyObject *cur_obj;
//     Py_ssize_t cur_pos;
//     Py_ssize_t cur_list_size; // cache for each list, no need to check it repeatedly
//     UCSKind cur_ucs_kind = UCSKind::UCS1;
//     u8 *dst1 = (u8 *) buffer_info.m_buffer_start;
//     u16 *dst2 = NULL;         // enabled if kind >= 2
//     u32 *dst4 = NULL;         // enabled if kind == 4
//     Py_ssize_t dst1_size = 0; // valid if kind >= 2
//     Py_ssize_t dst2_size = 0; // valid if kind == 4

#define ALIAS_NAMES()                                      \
    auto &last_pos_stack_index = cfg.last_pos_stack_index; \
    auto &cur_obj = cfg.cur_obj;                           \
    auto &cur_pos = cfg.cur_pos;                           \
    auto &cur_list_size = cfg.cur_list_size;               \
    auto &cur_ucs_kind = cfg.cur_ucs_kind;                 \
    auto &dst1 = cfg.dst1;                                 \
    auto &dst2 = cfg.dst2;                                 \
    auto &dst4 = cfg.dst4;                                 \
    auto &dst1_size = cfg.dst1_size;                       \
    auto &dst2_size = cfg.dst2_size;                       \
    auto &key = cfg.key;                                   \
    auto &val = cfg.val

template<IndentLevel __indent, UCSKind __kind>
force_inline PyObject *pyyjson_dump_impl(EncodeConfig &cfg, BufferInfo &buffer_info, int jump_code = 0) {
    ALIAS_NAMES();
    if (jump_code) {
        // TODO
    }

    // if not ucs1, should jump to another place
    assert(cfg.cur_ucs_kind == UCSKind::UCS1);
    if (__kind != UCSKind::UCS1) {
        Py_UNREACHABLE();
    }

    if (PyDict_CheckExact(cur_obj)) {
        if (unlikely(PyDict_Size(cur_obj) == 0)) {
            write_empty_ctn<__indent, UCSKind::UCS1, true>(dst1, buffer_info, cfg);
            // APPEND_OP(DICT_EMPTY);
            goto success;
        }
        write_ctn_begin<__indent, UCSKind::UCS1, true>(dst1, buffer_info, cfg);
        // APPEND_OP(DICT_BEGIN);
        cur_pos = 0;
        goto dict_pair_begin;
    } else if (PyList_CheckExact(cur_obj)) {
        cur_list_size = PyList_Size(cur_obj);
        if (unlikely(cur_list_size == 0)) {
            write_empty_ctn<__indent, UCSKind::UCS1, false>(dst1, buffer_info, cfg);
            // APPEND_OP(ARRAY_EMPTY);
            goto success;
        }
        write_ctn_begin<__indent, UCSKind::UCS1, false>(dst1, buffer_info, cfg);
        // APPEND_OP(ARRAY_BEGIN);
        GOTO_ARR_VAL_BEGIN_WITH_SIZE(cur_list_size); // result in a no-op
    } else {
        if (unlikely(!PyTuple_CheckExact(cur_obj))) {
            goto fail_ctntype;
        }
        cur_list_size = PyTuple_Size(cur_obj);
        if (unlikely(cur_list_size == 0)) {
            write_empty_ctn<__indent, UCSKind::UCS1, false>(dst1, buffer_info, cfg);
            // APPEND_OP(ARRAY_EMPTY);
            goto success;
        }
        write_ctn_begin<__indent, UCSKind::UCS1, false>(dst1, buffer_info, cfg);
        // APPEND_OP(ARRAY_BEGIN);
        GOTO_TUPLE_VAL_BEGIN_WITH_SIZE(cur_list_size); // result in a no-op
    }

dict_pair_begin:;
    assert(PyDict_Size(cur_obj) != 0);

    if (PyDict_Next(cur_obj, &cur_pos, &key, &val)) {
        if (unlikely(__kind < UCSKind::UCS4 && __kind < PyUnicode_KIND(key))) {

            if (PyUnicode_KIND(key) == UCSKind::UCS4) {
                static_assert(__kind < UCSKind::UCS4);
                elevate_state<UCSKind::UCS4>(cfg, buffer_info);
                return pyyjson_dump_impl<__indent, UCSKind::UCS4>(cfg, buffer_info, 1);
            } else if (PyUnicode_KIND(key) == UCSKind::UCS2) {
                static_assert(__kind == UCSKind::UCS1);
                elevate_state<UCSKind::UCS2>(cfg, buffer_info);
                return pyyjson_dump_impl<__indent, UCSKind::UCS2>(cfg, buffer_info, 1);
            } else {
                Py_UNREACHABLE();
            }
        }
    jump_label_1:;
        APPEND_KEY(key); // TODO
        if (PyUnicode_CheckExact(val)) {
        jump_label_2:;
            APPEND_STR(val);
        } else if (PyLong_CheckExact(val)) {
            write_PyLong(val);
        } else if (PyFloat_CheckExact(val)) {
            write_PyFloat(val);
        } else if (PyDict_CheckExact(val)) {
            // TODO size grow
            if (unlikely(PyDict_Size(val) == 0)) {
                WRITE_EMPTY_DICT();
                // APPEND_OP(DICT_EMPTY);
            } else {
                CtnType *cur_write_ctn = ctn_stack + ++last_pos_stack_index;
                cur_write_ctn->ctn = cur_pos;
                cur_obj = val;
                cur_pos = 0;
                WRITE_DICT_BEGIN();
                // APPEND_OP(DICT_BEGIN);
                goto dict_pair_begin;
            }
        } else if (PyList_CheckExact(val)) {
            // TODO size grow
            Py_ssize_t this_list_size = PyList_Size(val);
            if (unlikely(cur_list_size == 0)) {
                WRITE_EMPTY_ARRAY();
                // APPEND_OP(ARRAY_EMPTY);
            } else {
                CtnType *cur_write_ctn = ctn_stack + ++last_pos_stack_index;
                cur_write_ctn->ctn = cur_pos;
                cur_obj = val;
                cur_pos = 0;
                WRITE_ARRAY_BEGIN();
                // APPEND_OP(LIST_BEGIN);
                GOTO_ARR_VAL_BEGIN_WITH_SIZE(this_list_size);
            }
        } else if (PyTuple_CheckExact(val)) {
            // TODO size grow
            Py_ssize_t this_list_size = PyTuple_Size(val);
            if (unlikely(this_list_size == 0)) {
                WRITE_EMPTY_ARRAY();
                // APPEND_OP(ARRAY_EMPTY);
            } else {
                CtnType *cur_write_ctn = ctn_stack + ++last_pos_stack_index;
                cur_write_ctn->ctn = cur_pos;
                cur_obj = val;
                cur_pos = 0;
                WRITE_ARRAY_BEGIN();
                // APPEND_OP(LIST_BEGIN);
                GOTO_TUPLE_VAL_BEGIN_WITH_SIZE(this_list_size);
            }
        } else if ((val) == Py_True) {
            APPEND_OP(TRUE);
        } else if (val == Py_False) {
            APPEND_OP(FALSE);
        } else if (Py_IS_NAN(val)) {
            APPEND_OP(NAN);
        } else {
            // TODO
            goto fail;
        }
        goto dict_pair_begin;
    } else {
        // dict end
        APPEND_OP(DICT_END);
        if (unlikely(last_pos_stack_index == -1)) {
            goto success;
        }
        CtnType *last_pos = ctn_stack + last_pos_stack_index--;
        cur_obj = last_pos->ctn;
        cur_pos = last_pos->index;
        if (IS_DICT(cur_obj)) {
            goto dict_pair_begin;
        } else if (IS_LIST(cur_obj)) {
            GOTO_ARR_VAL_BEGIN_WITH_SIZE(PyList_GET_SIZE(cur_obj));
        } else {
            assert(IS_TUPLE(cur_obj));
            GOTO_TUPLE_VAL_BEGIN_WITH_SIZE(PyTuple_GET_SIZE(cur_obj));
        }
    }

    Py_UNREACHABLE();
arr_val_begin:;
    assert(cur_list_size != 0);

    if (cur_pos < cur_list_size) {
        val = PyList_GET_ITEM(cur_obj, cur_pos);
        // TODO same as above...
        if () {
            //...
        } else if () {
            //...
        }
        //...
        else {
            // TODO
            goto fail;
        }
        goto arr_val_begin;
    } else {
        // list end
        APPEND_OP(ARRAY_END);
        if (unlikely(last_pos_stack_index == -1)) {
            goto success;
        }
        // TODO same as above...
    }
    Py_UNREACHABLE();
tuple_val_begin:;
    assert(cur_list_size != 0);

    if (cur_pos < cur_list_size) {
        val = PyTuple_GET_ITEM(cur_obj, cur_pos);
        // TODO same as above...
        if () {
            //...
        } else if () {
            //...
        }
        //...
        else {
            // TODO
            goto fail;
        }
        goto tuple_val_begin;
    } else {
        // list end
        APPEND_OP(ARRAY_END);
        if (unlikely(last_pos_stack_index == -1)) {
            goto success;
        }
        // TODO same as above...
    }
    Py_UNREACHABLE();
success:
    assert(last_pos_stack_index == -1);

    PyObject *result = pyyjson_write_str();

    // cleanup. TODO

    if (!result && !PyErr_Occurred()) {
        PyErr_SetString(JSONEncodeError, "Unknown encode error");
    }
    return result;

fail:
    // cleanup. TODO
    if (!PyErr_Occurred()) {
        PyErr_SetString(JSONEncodeError, "Unknown encode error");
    }
    return NULL;
}

template<IndentLevel __indent>
PyObject *pyyjson_dump_obj(PyObject *in_obj) {
    EncodeConfig cfg;

    cfg.cur_obj = in_obj;

    BufferInfo buffer_info;
    buffer_info.m_buffer_start = (void *) &_ThreadLocal_DstBuffer[0];
    buffer_info.m_size = PYYJSON_ENCODE_DST_BUFFER_INIT_SIZE;

#undef GOTO_TUPLE_VAL_BEGIN_WITH_SIZE
#undef GOTO_ARR_VAL_BEGIN_WITH_SIZE
}

extern "C" {
PyObject *pyyjson_Encode(PyObject *self, PyObject *args, PyObject *kwargs) {
}
}
