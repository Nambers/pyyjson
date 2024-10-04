#include "encode.hpp"
#include "encode_avx2.inl"
#include "encode_avx512.inl"
#include "encode_shared.hpp"
#include "encode_sse2.inl"
#include "encode_sse4.inl"
#include "pyyjson_config.h"
#include "yyjson.h"
#include <algorithm>
#include <array>
#include <stddef.h>

/*Macros*/


#define RETURN_ON_UNLIKELY_ERR(x) \
    do {                          \
        if (unlikely((x))) {      \
            return false;         \
        }                         \
    } while (0)

#define ENCODE_MOVE_SRC_DST(src, src_count, dst, dst_left, move_count) \
    do {                                                               \
        Py_ssize_t _temp__ = (move_count);                             \
        src += _temp__;                                                \
        dst += _temp__;                                                \
        src_count -= _temp__;                                          \
        dst_left -= _temp__;                                           \
    } while (0)

#define INDENT_SIZE(_IndentConfig) (static_cast<int>(_IndentConfig))
#define NEED_INDENT(_IndentConfig) (INDENT_SIZE(_IndentConfig) != 0)

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

/* Constexpr Utils */

force_inline constexpr bool size_is_pow2(usize size) {
    return (size & (size - 1)) == 0;
}

force_inline constexpr usize size_align_up(usize size, usize align) {
    if (size_is_pow2(align)) {
        return (size + (align - 1)) & ~(align - 1);
    } else {
        return size + align - (size + align - 1) % align - 1;
    }
}

template<typename T>
force_inline constexpr T *addr_align_up(void *p) {
    static_assert(size_is_pow2(sizeof(T)));
    return (T *) size_align_up((usize) p, sizeof(T));
}


/* Struct Defs */

template<UCSKind __kind>
struct EncodeDstBufferInfo {
    UCSType_t<__kind> *dst;
    UCSType_t<__kind> *const dst_buffer_end;
};

struct EncodeBuffer {
    u8 *m_buffer_start_ptr;
    u8 *m_read_valid_end_ptr;
    u8 *m_buffer_end_ptr;

    static EncodeBuffer init(Py_ssize_t size) {
        // EncodeBuffer ret;
        void *new_buffer = malloc((size_t) size);
        if (likely(new_buffer)) {
            return {
                    (u8 *) new_buffer,
                    (u8 *) new_buffer,
                    (u8 *) new_buffer + size,
            };
            // ret.m_buffer_start_ptr = (u8 *) new_buffer;
            // ret.m_read_valid_end_ptr = (u8 *) new_buffer;
            // ret.m_buffer_end_ptr = ret.m_buffer_start_ptr + size;
        } else {
            return {
                    nullptr,
                    nullptr,
                    nullptr,
            };
            // ret.m_buffer_start_ptr = nullptr;
            // ret.m_read_valid_end_ptr = nullptr;
            // ret.m_buffer_end_ptr = nullptr;
        }
        // return ret;
    }
};

typedef struct CtnType {
    PyObject *ctn;
    Py_ssize_t index;
} CtnType;

struct EncodeBufferDesc {
    size_t cur_buffer_index;
    union {
        u8 *cur_rw_addr;
        u16 *cur_rw_addr_16;
        u32 *cur_rw_addr_32;
    };
    size_t total_buffers_count; // ==0 iff the buffer is freed

    force_inline static EncodeBufferDesc init();

    force_inline void clear();

    force_inline void reset_for_read();

    // force_inline bool _write_A(u8 *src, Py_ssize_t count);

    // force_inline bool write_A(u8 *src, Py_ssize_t count);

    template<UCSKind __kind>
    force_inline UCSType_t<__kind> *&get_rw_ptr_ref();

    template<UCSKind __kind>
    force_inline Py_ssize_t get_write_count_left();

    template<UCSKind __kind, X86SIMDLevel __simd_level>
    force_inline bool write_unicode_impl(UCSType_t<__kind> *src, Py_ssize_t count);

    force_inline bool write_unicode(PyObject *unicode);

    // template<UCSKind __kind>
    // force_inline void _read(EncodeDstBufferInfo<__kind> &dst, Py_ssize_t read_count) {
    //     // TODO finish this
    //     if constexpr (__kind == UCSKind::UCS1) {
    //         memcpy(dst.dst, this->cur_rw_addr, (size_t) read_count);
    //         this->cur_rw_addr += read_count;
    //         dst.dst += read_count;
    //     } else {
    //         using uu = UCSType_t<__kind>;
    //         for (size_t i = 0; i < read_count; ++i) {
    //             *dst.dst++ = (uu) * this->cur_rw_addr++;
    //         }
    //     }
    // }
    // template<UCSKind __kind>
    // force_inline void read(EncodeDstBufferInfo<__kind> &dst, Py_ssize_t read_count) {
    //     assert(read_count > 0);
    //     const EncodeBuffer &cur_buffer = _EncodeBufferList[this->cur_buffer_index];
    //     assert(this->cur_rw_addr >= cur_buffer.m_buffer_start_ptr);
    //     assert(this->cur_rw_addr <= cur_buffer.m_buffer_end_ptr);
    //     const Py_ssize_t cur_can_read_size = cur_buffer.m_buffer_end_ptr - this->cur_rw_addr;
    //     if (unlikely(cur_can_read_size < read_count)) {
    //         this->_read(dst, cur_can_read_size);
    //         // read from next buffer
    //         this->cur_rw_addr = _EncodeBufferList[this->cur_buffer_index++].m_buffer_start_ptr;
    //         this->_read(dst, read_count - cur_can_read_size);
    //     } else {
    //         this->_read<__kind>(dst, read_count);
    //     }
    // }

    template<UCSKind __kind>
    force_inline bool resize(UCSType_t<__kind> *&dst, Py_ssize_t &write_count_left, Py_ssize_t need_reserve_count = 0);
};

struct DumpOption {
    IndentLevel indent_level;
};

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


typedef struct EncodeWriteOpConfig {
    Py_ssize_t last_pos_stack_index = -1;
    PyObject *cur_obj;
    Py_ssize_t cur_pos = 0;
    UCSKind cur_ucs_kind = UCSKind::UCS1;
    bool is_all_ascii = true;
    // temp variables storing key and value
    Py_ssize_t cur_list_size; // cache for each list, no need to check it repeatedly
    PyObject *key, *val;
} EncodeWriteOpConfig;

struct BufferAddrDesc {
    Py_ssize_t m_normal_len;
    Py_ssize_t m_unicode_size;
    Py_ssize_t m_unicode_kind;

    // force_inline bool is_unicode();

    force_inline Py_ssize_t unicode_size();

    force_inline UCSKind unicode_type();

    force_inline Py_ssize_t normal_buffer_len();

    force_inline Py_ssize_t unicode_u8_len();
};

struct AddrDescRW {
    BufferAddrDesc *addr_descs[64];
    usize addr_descs_count;
    usize addr_descs_read_index;
    //
    BufferAddrDesc *cur_rw_ptr;
    BufferAddrDesc *cur_end_ptr;

    force_inline static AddrDescRW init();

    force_inline void clear();

    force_inline constexpr static Py_ssize_t get_size_u8(size_t index);

    force_inline bool resize();

    force_inline void reset_for_read();

    force_inline bool write();

    force_inline void mark_read();
};

static_assert(sizeof(BufferAddrDesc) == sizeof(Py_ssize_t));
/*Static Variables*/

thread_local EncodeBuffer _EncodeBufferList[128];
thread_local yyjson_align(64) u8 _EncodeFirstBuffer[4096];
thread_local Py_ssize_t AddrDescFirstBuffer[3 * PYYJSON_ENCODE_ADDR_DESC_BUFFER_INIT_SIZE];

static_assert(PYYJSON_ENCODE_ADDR_DESC_BUFFER_INIT_SIZE > 0);
static_assert((Py_ssize_t) 3 * PYYJSON_ENCODE_ADDR_DESC_BUFFER_INIT_SIZE > 0);

/*SIMD Utils*/
/*check_escape*/
template<UCSKind __kind, X86SIMDLevel __simd_level>
force_inline bool check_escape_512bits(__m512i &u) {
    static_assert(__simd_level >= X86SIMDLevel::AVX512);
    if constexpr (__kind == UCSKind::UCS1) {
        return check_escape_u8_512_avx512(u);
    } else if constexpr (__kind == UCSKind::UCS2) {
        return check_escape_u16_512_avx512(u);
    } else {
        return check_escape_u32_512_avx512(u);
    }
}

/*check_escape_tail*/

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

/* Copy Utils */

template<X86SIMDLevel __simd_level>
force_inline void cpy_once_512_impl(u8 *&ddst, u8 *&ssrc) {
    static_assert(__simd_level >= X86SIMDLevel::AVX512);
    _mm512_storeu_si512((void *) ddst, _mm512_loadu_si512((void *) ssrc)); // vmovdqu32, AVX512F
}

template<X86SIMDLevel __simd_level>
force_inline void cpy_fast_512_impl(u8 *&ddst, u8 *&ssrc, Py_ssize_t &count, Py_ssize_t &allow_write_size) {
    static_assert(__simd_level >= X86SIMDLevel::AVX512);
    while (allow_write_size >= 512 / 8 && count > 0) {
        cpy_once_512_impl<__simd_level>(ddst, ssrc); // AVX512F
        ENCODE_MOVE_SRC_DST(ssrc, count, ddst, allow_write_size, 512 / 8);
    }
}

template<X86SIMDLevel __simd_level>
force_inline void cpy_once_256_impl(u8 *&ddst, u8 *&ssrc) {
    static_assert(__simd_level >= X86SIMDLevel::AVX);
    _mm256_storeu_si256((__m256i *) ddst, _mm256_lddqu_si256((const __m256i_u *) ssrc)); // vmovdqu, vlddqu, AVX
}

template<X86SIMDLevel __simd_level>
force_inline void cpy_fast_256_impl(u8 *&ddst, u8 *&ssrc, Py_ssize_t &count, Py_ssize_t &allow_write_size) {
    static_assert(__simd_level >= X86SIMDLevel::AVX);
    while (allow_write_size >= 256 / 8 && count > 0) {
        cpy_once_256_impl<__simd_level>(ddst, ssrc); // AVX
        ENCODE_MOVE_SRC_DST(ssrc, count, ddst, allow_write_size, 256 / 8);
    }
}

template<X86SIMDLevel __simd_level>
force_inline void cpy_once_128_impl(u8 *&ddst, u8 *&ssrc) {
    if constexpr (__simd_level >= X86SIMDLevel::SSE3) {
        _mm_store_si128((__m128i *) ddst, _mm_lddqu_si128((const __m128i_u *) ssrc)); // lddqu, SSE3
    } else {
        _mm_store_si128((__m128i *) ddst, _mm_loadu_si128((const __m128i_u *) ssrc)); // movdqu, SSE2
    }
}

template<X86SIMDLevel __simd_level>
force_inline void cpy_fast_128_impl(u8 *&ddst, u8 *&ssrc, Py_ssize_t &count, Py_ssize_t &allow_write_size) {
    while (allow_write_size >= 128 / 8 && count > 0) {
        cpy_once_128_impl<__simd_level>(ddst, ssrc);
        ENCODE_MOVE_SRC_DST(ssrc, count, ddst, allow_write_size, 128 / 8);
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

/*copy_escape*/

template<UCSKind __kind, X86SIMDLevel __simd_level>
force_inline bool copy_slash_quote_impl(const UCSType_t<__kind> *_ToWrite, UCSType_t<__kind> *&dst, Py_ssize_t &write_count_left, EncodeBufferDesc &desc) {
    using uu = UCSType_t<__kind>;
    if (unlikely(write_count_left < 2)) {
        if (write_count_left == 1) {
            *dst = _ToWrite[0];
            dst++;
            write_count_left--;
            bool _c = desc.resize<__kind>(dst, write_count_left);
            RETURN_ON_UNLIKELY_ERR(!_c);
            assert(write_count_left >= 2);
            *dst = _ToWrite[1];
            dst++;
            write_count_left--;
            goto success;
        } else {
            assert(0 == write_count_left);
            bool _c = desc.resize<__kind>(dst, write_count_left);
            RETURN_ON_UNLIKELY_ERR(!_c);
            assert(write_count_left >= 2);
        }
    }
    memcpy((void *) dst, (void *) _ToWrite, 2 * sizeof(uu));
    dst += 2;
    write_count_left -= 2;
success:
    return true;
}

template<UCSKind __kind, X86SIMDLevel __simd_level>
force_inline bool copy_escape_impl(
        UCSType_t<__kind> val,
        UCSType_t<__kind> *&dst,
        Py_ssize_t &write_count_left,
        EncodeBufferDesc &desc) {
    using uu = UCSType_t<__kind>;
    switch (val) {
        case _Quote: {
            constexpr uu _ToWrite[2] = {_Slash, _Quote};
            bool _c = copy_slash_quote_impl<__kind, __simd_level>(_ToWrite, dst, write_count_left, desc);
            RETURN_ON_UNLIKELY_ERR(!_c);
            break;
        }
        case _Slash: {
            constexpr uu _ToWrite[2] = {_Slash, _Slash};
            bool _c = copy_slash_quote_impl<__kind, __simd_level>(_ToWrite, dst, write_count_left, desc);
            RETURN_ON_UNLIKELY_ERR(!_c);
            break;
        }
        default: {
            if (unlikely(val < ControlMax)) {
                Py_ssize_t need_to_copy = _ControlJump[(size_t) val];
                uu *copy_from = &_ControlSeqTable<__kind>[(size_t) val * 6];
                if (unlikely(write_count_left < need_to_copy)) {
                    memcpy((void *) dst, (void *) copy_from, write_count_left * sizeof(uu));
                    ENCODE_MOVE_SRC_DST(copy_from, need_to_copy, dst, write_count_left, write_count_left);
                    assert(!write_count_left);
                    bool _c = desc.resize<__kind>(dst, write_count_left);
                    RETURN_ON_UNLIKELY_ERR(!_c);
                    assert(write_count_left >= need_to_copy);
                }
                cpy_fast<__simd_level>((void *) dst, (void *) copy_from, need_to_copy * sizeof(uu), write_count_left * sizeof(uu));
                dst += need_to_copy;
                write_count_left -= need_to_copy;
            } else {
                if (unlikely(!write_count_left)) {
                    bool _c = desc.resize<__kind>(dst, write_count_left);
                    RETURN_ON_UNLIKELY_ERR(!_c);
                }
                assert(write_count_left);
                *dst++ = val;
                write_count_left--;
            }
        }
    }
    return true;
}

template<UCSKind __kind, X86SIMDLevel __simd_level>
force_inline bool copy_escape(
        UCSType_t<__kind> *&src,
        Py_ssize_t &count,
        UCSType_t<__kind> *&dst,
        Py_ssize_t &write_count_left,
        EncodeBufferDesc &desc) {
    using uu = UCSType_t<__kind>;

    assert(count);

    while (count) {
        uu val = *src;
        bool _c = copy_escape_impl<__kind, __simd_level>(val, dst, write_count_left, desc);
        RETURN_ON_UNLIKELY_ERR(!_c);
        count--;
        src++;
    }
    return true;
}

template<UCSKind __kind, X86SIMDLevel __simd_level>
force_inline bool copy_escape_until_resize(
        UCSType_t<__kind> *&src,
        Py_ssize_t &count,
        UCSType_t<__kind> *&dst,
        Py_ssize_t &write_count_left,
        EncodeBufferDesc &desc) {
    using uu = UCSType_t<__kind>;
    while (true) {
        Py_ssize_t old_size = write_count_left;
        uu val = *src;
        bool _c = copy_escape_impl<__kind, __simd_level>(val, dst, write_count_left, desc);
        RETURN_ON_UNLIKELY_ERR(!_c);
        count--;
        src++;
        if (old_size < write_count_left) break;
    }
    return true;
}

/*copy_ucs_same*/

template<UCSKind __kind, X86SIMDLevel __simd_level>
force_inline bool copy_ucs_same_128bits(UCSType_t<__kind> *&src, Py_ssize_t &count, UCSType_t<__kind> *&dst, Py_ssize_t &write_count_left, EncodeBufferDesc &desc) {
    using uu = UCSType_t<__kind>;

    constexpr std::ptrdiff_t _Process128OnceCount = 128 / 8 / sizeof(uu);

    Py_ssize_t temp_src_count;
    __m128i x;
    bool v;

back_128:;
    while (count >= _Process128OnceCount && write_count_left >= _Process128OnceCount) {
        x = _mm_lddqu_si128((const __m128i_u *) src); // lddqu, SSE3
        v = check_and_write_128<__kind, __simd_level>((void *) dst, x);
        if (likely(v)) {
            ENCODE_MOVE_SRC_DST(src, count, dst, write_count_left, _Process128OnceCount);
        } else {
            temp_src_count = _Process128OnceCount;
            v = copy_escape<__kind, __simd_level>(src, temp_src_count, dst, write_count_left, desc);
            RETURN_ON_UNLIKELY_ERR(!v);
            count -= _Process128OnceCount - temp_src_count;
            continue;
        }
    }
    //
    if (count >= _Process128OnceCount) {
        assert(write_count_left < _Process128OnceCount);
        copy_escape_until_resize<__kind, __simd_level>(src, count, dst, write_count_left, desc);
        goto back_128;
    }
    //
    assert(count < _Process128OnceCount);
    if (!count) return true;
    if (likely(write_count_left >= _Process128OnceCount)) {
        // try a tail check
        x = _mm_lddqu_si128((const __m128i_u *) src); // lddqu, SSE3
        _mm_storeu_si128((void *) dst, x);            // movdqu, SSE2
        v = check_escape_tail_128bits<__kind, __simd_level>(x, count);
        if (likely(v)) {
            ENCODE_MOVE_SRC_DST(src, count, dst, write_count_left, count);
            return true;
        }
    }
    return copy_escape<__kind, __simd_level>(src, count, dst, write_count_left, desc);
}

template<UCSKind __kind, X86SIMDLevel __simd_level>
force_inline bool copy_ucs_same_256bits(UCSType_t<__kind> *&src, Py_ssize_t &count, UCSType_t<__kind> *&dst, Py_ssize_t &write_count_left, EncodeBufferDesc &desc) {
    static_assert(__simd_level >= X86SIMDLevel::AVX2, "Only AVX2 or above can use this");
    using uu = UCSType_t<__kind>;

    constexpr std::ptrdiff_t _Process256OnceCount = 256 / 8 / sizeof(uu);
    constexpr std::ptrdiff_t _Process128OnceCount = 128 / 8 / sizeof(uu);

    Py_ssize_t temp_src_count;
    __m256i y;
    __m128i x;
    bool v;

back_256:;
    while (count >= _Process256OnceCount && write_count_left >= _Process256OnceCount) {
        y = _mm256_lddqu_si256((const __m256i_u *) src); // vlddqu, AVX
        v = check_and_write_256<__kind, __simd_level>((void *) dst, y);
        if (likely(v)) {
            ENCODE_MOVE_SRC_DST(src, count, dst, write_count_left, _Process256OnceCount);
        } else {
            x = _mm256_castsi256_si128(y); // no-op, AVX
            v = check_and_write_128<__kind, __simd_level>((void *) dst, x);
            if (v) {
                // .x
                ENCODE_MOVE_SRC_DST(src, count, dst, write_count_left, _Process128OnceCount);
                // moved: x
            }
            temp_src_count = _Process128OnceCount;
            v = copy_escape<__kind, __simd_level>(src, temp_src_count, dst, write_count_left, desc);
            RETURN_ON_UNLIKELY_ERR(!v);
            count -= _Process128OnceCount - temp_src_count;
            continue;
        }
    }
    //
    if (count >= _Process256OnceCount) {
        assert(write_count_left < _Process256OnceCount);
        if (write_count_left >= _Process128OnceCount) {
            x = _mm_lddqu_si128((const __m128i_u *) src); // lddqu, SSE3
            v = check_and_write_128<__kind, __simd_level>((void *) dst, x);
            if (likely(v)) {
                ENCODE_MOVE_SRC_DST(src, count, dst, write_count_left, _Process128OnceCount);
            }
        }
        copy_escape_until_resize<__kind, __simd_level>(src, count, dst, write_count_left, desc);
        goto back_256;
    }
    //
    assert(count < _Process256OnceCount);
    if (!count) return true;
    if (likely(write_count_left >= _Process256OnceCount)) {
        // try a tail check
        y = _mm256_lddqu_si256((const __m256i_u *) src); // vlddqu, AVX
        _mm256_storeu_si256((void *) dst, y);            // vmovdqu, AVX
        v = check_escape_tail_256bits<__kind, __simd_level>(y, count);
        if (likely(v)) {
            ENCODE_MOVE_SRC_DST(src, count, dst, write_count_left, count);
            return true;
        }
    }
    return copy_ucs_same_128bits<__kind, __simd_level>(src, count, dst, write_count_left, desc);
}

template<UCSKind __kind, X86SIMDLevel __simd_level>
force_inline bool copy_ucs_same_512bits(UCSType_t<__kind> *&src, Py_ssize_t &count, UCSType_t<__kind> *&dst, Py_ssize_t &write_count_left, EncodeBufferDesc &desc) {
    static_assert(__simd_level >= X86SIMDLevel::AVX512, "Only AVX512 or above can use this");
    using uu = UCSType_t<__kind>;

    constexpr std::ptrdiff_t _Process512OnceCount = 512 / 8 / sizeof(uu);
    constexpr std::ptrdiff_t _Process256OnceCount = 256 / 8 / sizeof(uu);
    constexpr std::ptrdiff_t _Process128OnceCount = 128 / 8 / sizeof(uu);

    Py_ssize_t temp_src_count;
    __m512i z;
    __m256i y;
    __m128i x;
    bool v;

back_512:;
    while (count >= _Process512OnceCount && write_count_left >= _Process512OnceCount) {
        z = _mm512_loadu_si512((void *) src); // vmovdqu32, AVX512F
        v = check_and_write_512<__kind, __simd_level>((void *) dst, z);
        if (likely(v)) {
            ENCODE_MOVE_SRC_DST(src, count, dst, write_count_left, _Process512OnceCount);
        } else {
            y = _mm512_castsi512_si256(z); // no-op, AVX512F
            v = check_and_write_256<__kind, __simd_level>((void *) dst, y);
            if (v) {
                // ..??
                ENCODE_MOVE_SRC_DST(src, count, dst, write_count_left, _Process256OnceCount);
                // moved: ??
                x = _mm512_extracti32x4_epi32(z, 2); // vextracti32x4, AVX512F
                v = check_and_write_128<__kind, __simd_level>((void *) dst, x);
                if (v) {
                    // .x
                    ENCODE_MOVE_SRC_DST(src, count, dst, write_count_left, _Process128OnceCount);
                    // moved: x
                }
                temp_src_count = _Process128OnceCount;
                v = copy_escape<__kind, __simd_level>(src, temp_src_count, dst, write_count_left, desc);
                RETURN_ON_UNLIKELY_ERR(!v);
                count -= _Process128OnceCount - temp_src_count;
                continue;
            } else {
                // ??
                x = _mm512_castsi512_si128(z); // no-op, AVX512F
                v = check_and_write_128<__kind, __simd_level>((void *) dst, x);
                if (v) {
                    // .x
                    ENCODE_MOVE_SRC_DST(src, count, dst, write_count_left, _Process128OnceCount);
                    // moved: x
                }
                // x
                temp_src_count = _Process128OnceCount;
                v = copy_escape<__kind, __simd_level>(src, temp_src_count, dst, write_count_left, desc);
                RETURN_ON_UNLIKELY_ERR(!v);
                count -= _Process128OnceCount - temp_src_count;
                continue;
            }
        }
    }
    //
    if (unlikely(count >= _Process512OnceCount)) {
        assert(write_count_left < _Process512OnceCount);
        if (write_count_left >= _Process256OnceCount) {
            y = _mm256_lddqu_si256((const __m256i_u *) src); // vlddqu, AVX
            v = check_and_write_256<__kind, __simd_level>((void *) dst, y);
            if (likely(v)) {
                ENCODE_MOVE_SRC_DST(src, count, dst, write_count_left, _Process256OnceCount);
            }
        }
        if (write_count_left >= _Process128OnceCount) {
            x = _mm_lddqu_si128((const __m128i_u *) src); // lddqu, SSE3
            v = check_and_write_128<__kind, __simd_level>((void *) dst, x);
            if (likely(v)) {
                ENCODE_MOVE_SRC_DST(src, count, dst, write_count_left, _Process128OnceCount);
            }
        }
        copy_escape_until_resize<__kind, __simd_level>(src, count, dst, write_count_left, desc);
        goto back_512;
    }
    //
    assert(count < _Process512OnceCount);
    if (!count) return true;
    if (likely(write_count_left >= _Process512OnceCount)) {
        // try a tail check
        z = _mm512_loadu_si512((void *) src); // vmovdqu32, AVX512F
        _mm512_storeu_si512((void *) dst, z); // vmovdqu32, AVX512F
        v = check_escape_tail_512bits<__kind, __simd_level>(z, count);
        if (likely(v)) {
            ENCODE_MOVE_SRC_DST(src, count, dst, write_count_left, count);
            return true;
        }
    }
    return copy_ucs_same_256bits<__kind, __simd_level>(src, count, dst, write_count_left, desc);
}

/*Struct Impl*/
force_inline EncodeBufferDesc EncodeBufferDesc::init() {
    EncodeBufferDesc ret;
    ret.cur_buffer_index = 0;
    ret.cur_rw_addr = &_EncodeFirstBuffer[0];
    ret.total_buffers_count = 1;
    // also init first EncodeBuffer
    _EncodeBufferList[0].m_buffer_start_ptr = &_EncodeFirstBuffer[0];
    _EncodeBufferList[0].m_read_valid_end_ptr = &_EncodeFirstBuffer[0];
    _EncodeBufferList[0].m_buffer_end_ptr = &_EncodeFirstBuffer[4096];
    return ret;
}

template<UCSKind __kind>
force_inline bool EncodeBufferDesc::resize(UCSType_t<__kind> *&dst, Py_ssize_t &write_count_left, Py_ssize_t need_reserve_count) {
    using uu = UCSType_t<__kind>;

    // assert(write_count_left == 0);

    u8 *const valid_read_end = this->cur_rw_addr;

    EncodeBuffer &cur_buffer = _EncodeBufferList[this->cur_buffer_index];
    assert((need_reserve_count % (512 / 8)) == 0);
    assert(need_reserve_count >= 0);
    assert(this->cur_rw_addr == cur_buffer.m_buffer_end_ptr);
    // dst must be a ref to this->cur_rw_addr
    assert((void *) &dst == (void *) &this->cur_rw_addr);

    Py_ssize_t cur_size = cur_buffer.m_buffer_end_ptr - cur_buffer.m_buffer_start_ptr;
    const Py_ssize_t new_size_half = size_align_up(cur_size, 512 / 8 / 2);
    // constexpr Py_ssize_t MAX_BUFFER_COUNT = ~((Py_ssize_t) 1 << (sizeof(Py_ssize_t) * 8 - 1));
    // constexpr Py_ssize_t MAX_BUFFER_COUNT_HALF = MAX_BUFFER_COUNT / 2;
    if (unlikely((new_size_half << 1) <= 0 || this->cur_buffer_index + 1 == 128)) {
        PyErr_NoMemory();
        return false;
    }
    const Py_ssize_t need_size = std::max(need_reserve_count, new_size_half << 1);
    EncodeBuffer &next_buffer = _EncodeBufferList[this->cur_buffer_index + 1];
    next_buffer = EncodeBuffer::init(need_size);
    if (unlikely(!next_buffer.m_buffer_start_ptr)) {
        PyErr_NoMemory();
        return false;
    }
    // update this
    this->cur_rw_addr = next_buffer.m_buffer_start_ptr;
    this->cur_buffer_index++;
    this->total_buffers_count++;
    // update last encode buffer
    cur_buffer.m_read_valid_end_ptr = valid_read_end;
    assert(0 == ((next_buffer.m_buffer_end_ptr - next_buffer.m_buffer_start_ptr) % sizeof(uu)));
    // update in-out param. dst is a ref of this->cur_rw_addr so no need to update again
    write_count_left = (next_buffer.m_buffer_end_ptr - next_buffer.m_buffer_start_ptr) / sizeof(uu);
    return true;
}

force_inline void EncodeBufferDesc::clear() {
    for (size_t i = 1; i < this->total_buffers_count; ++i) {
        free(_EncodeBufferList[i].m_buffer_start_ptr);
    }
    this->total_buffers_count = 0;
}

force_inline void EncodeBufferDesc::reset_for_read() {
    this->cur_buffer_index = 0;
    this->cur_rw_addr = &_EncodeFirstBuffer[0];
}

template<UCSKind __kind>
force_inline UCSType_t<__kind> *&EncodeBufferDesc::get_rw_ptr_ref() {
    if constexpr (__kind == UCSKind::UCS1) {
        return this->cur_rw_addr;
    } else if constexpr (__kind == UCSKind::UCS2) {
        return this->cur_rw_addr_16;
    } else {
        static_assert(__kind == UCSKind::UCS4);
        return this->cur_rw_addr_32;
    }
}

template<UCSKind __kind>
force_inline Py_ssize_t EncodeBufferDesc::get_write_count_left() {
    using uu = UCSType_t<__kind>;

    Py_ssize_t write_count_left;
    {
        const EncodeBuffer &cur_buffer = _EncodeBufferList[this->cur_buffer_index];
        write_count_left = cur_buffer.m_buffer_end_ptr - this->cur_rw_addr;
        assert((write_count_left % sizeof(uu)) == 0);
        write_count_left /= sizeof(uu);
    }
    return write_count_left;
}

template<UCSKind __kind, X86SIMDLevel __simd_level>
force_inline bool EncodeBufferDesc::write_unicode_impl(UCSType_t<__kind> *src, Py_ssize_t count) {
    using uu = UCSType_t<__kind>;
    this->cur_rw_addr = addr_align_up<uu>((void *) this->cur_rw_addr);
    Py_ssize_t write_count_left = this->get_write_count_left<__kind>();
    //
    bool _ret;
    uu *&dst = this->get_rw_ptr_ref<__kind>();
    if constexpr (__simd_level >= X86SIMDLevel::AVX512) {
        _ret = copy_ucs_same_512bits<__kind, __simd_level>(src, count, dst, write_count_left, *this);
    } else if constexpr (__simd_level >= X86SIMDLevel::AVX2) {
        _ret = copy_ucs_same_256bits<__kind, __simd_level>(src, count, dst, write_count_left, *this);
    } else {
        _ret = copy_ucs_same_128bits<__kind, __simd_level>(src, count, dst, write_count_left, *this);
    }
    return _ret;
}

force_inline bool EncodeBufferDesc::write_unicode(PyObject *unicode) {
    if (PyUnicode_KIND(unicode) == PyUnicode_1BYTE_KIND) {
        return this->write_unicode_impl<UCSKind::UCS1>((u8 *) PyUnicode_DATA(unicode), PyUnicode_GET_LENGTH(unicode));
    } else if (PyUnicode_KIND(unicode) == PyUnicode_2BYTE_KIND) {
        return this->write_unicode_impl<UCSKind::UCS2>((u16 *) PyUnicode_DATA(unicode), PyUnicode_GET_LENGTH(unicode));
    } else {
        return this->write_unicode_impl<UCSKind::UCS4>((u32 *) PyUnicode_DATA(unicode), PyUnicode_GET_LENGTH(unicode));
    }
}

// force_inline bool EncodeBufferDesc::_write_A(u8 *src, Py_ssize_t count) {
//     assert(this->total_buffers_count);
//     // TODO
//     memcpy(this->cur_rw_addr, src, (size_t) count);
//     this->cur_rw_addr += count;
// }

// force_inline bool EncodeBufferDesc::write_A(u8 *src, Py_ssize_t count) {
//     assert(count > 0);
//     assert(this->cur_buffer_index < 128);
//     //
//     const EncodeBuffer &cur_buffer = _EncodeBufferList[this->cur_buffer_index];
//     assert(this->cur_rw_addr >= cur_buffer.m_buffer_start_ptr);
//     assert(this->cur_rw_addr <= cur_buffer.m_buffer_end_ptr);
//     //
//     const Py_ssize_t cur_can_write_size = cur_buffer.m_buffer_end_ptr - this->cur_rw_addr;
//     if (unlikely(cur_can_write_size < count)) {
//         // write to next buffer
//         const Py_ssize_t cur_buffer_size = cur_buffer.m_buffer_end_ptr - cur_buffer.m_buffer_start_ptr;
//         constexpr Py_ssize_t MAX_BUFFER_COUNT = ~((Py_ssize_t) 1 << (sizeof(Py_ssize_t) * 8 - 1));
//         static_assert(MAX_BUFFER_COUNT > 0);
//         constexpr Py_ssize_t MAX_BUFFER_COUNT_HALF = MAX_BUFFER_COUNT / 2;
//         const Py_ssize_t new_size_half = size_align_up(cur_buffer_size, SIZEOF_VOID_P);
//         if (unlikely(new_size_half > MAX_BUFFER_COUNT_HALF || this->cur_buffer_index + 1 == 128)) {
//             PyErr_NoMemory();
//             return false;
//         }
//         const Py_ssize_t need_size = count - cur_can_write_size;
//         Py_ssize_t new_size = std::max<Py_ssize_t>(new_size_half << 1, need_size);
//         assert((new_size) > 0);
//         EncodeBuffer &next_buffer = _EncodeBufferList[this->cur_buffer_index + 1];
//         next_buffer = EncodeBuffer::init(new_size);
//         if (unlikely(!next_buffer.m_buffer_start_ptr)) {
//             PyErr_NoMemory();
//             return false;
//         }
//         // new buffer created, write content
//         this->_write_A(src, cur_can_write_size);
//         src += cur_can_write_size;
//         this->cur_buffer_index++;
//         this->cur_rw_addr = next_buffer.m_buffer_start_ptr;
//         this->total_buffers_count++;
//         this->cur_rw_addr = next_buffer.m_buffer_start_ptr;
//         this->_write_A(src, need_size);
//     } else {
//         this->_write_A(src, count);
//     }
//     return true;
// }

constexpr Py_ssize_t PY_SSIZE_T_HIGHEST_BIT = ((Py_ssize_t) 1) << (sizeof(Py_ssize_t) * 8 - 1);

force_inline Py_ssize_t BufferAddrDesc::unicode_size() {
    return this->m_unicode_size;
}

force_inline UCSKind BufferAddrDesc::unicode_type() {
    return (UCSKind) this->m_unicode_kind;
}

force_inline Py_ssize_t BufferAddrDesc::normal_buffer_len() {
    return this->m_normal_len;
}

force_inline Py_ssize_t BufferAddrDesc::unicode_u8_len() {
    return this->unicode_size() * this->unicode_type();
}

force_inline AddrDescRW AddrDescRW::init() {
    AddrDescRW ret;
    ret.addr_descs[0] = (BufferAddrDesc *) &AddrDescFirstBuffer[0];
    ret.addr_descs_count = 1;
    ret.addr_descs_read_index = 0;
    ret.cur_rw_ptr = (BufferAddrDesc *) &AddrDescFirstBuffer[0];
    ret.cur_end_ptr = (BufferAddrDesc *) &AddrDescFirstBuffer[PYYJSON_ENCODE_ADDR_DESC_BUFFER_INIT_SIZE];
    return ret;
}

force_inline void AddrDescRW::clear() {
    for (usize i = 1; i < this->addr_descs_count; i++) {
        free(this->addr_descs[i]);
    }
}

force_inline constexpr Py_ssize_t AddrDescRW::get_size_u8(size_t index) {
    Py_ssize_t size = 3 * (Py_ssize_t) sizeof(Py_ssize_t) * ((Py_ssize_t) (PYYJSON_ENCODE_ADDR_DESC_BUFFER_INIT_SIZE) << index);
    return size;
}

static_assert(AddrDescRW::get_size_u8(0) == (Py_ssize_t) sizeof(AddrDescFirstBuffer));

force_inline bool AddrDescRW::resize() {
    assert(this->addr_descs_read_index == 0);
    assert(this->addr_descs_count);
    Py_ssize_t old_size = get_size_u8(this->addr_descs_count - 1);
    Py_ssize_t new_size = old_size << 1;

    if (unlikely(new_size < 0)) {
        PyErr_NoMemory();
        return false;
    }
    assert(new_size);
    void *new_buffer = malloc((size_t) new_size);
    if (unlikely(!new_buffer)) {
        PyErr_NoMemory();
        return false;
    }

    this->addr_descs[this->addr_descs_count++] = (BufferAddrDesc *) new_buffer;
    this->cur_rw_ptr = (BufferAddrDesc *) new_buffer;
    this->cur_end_ptr = (BufferAddrDesc *) ((u8 *) new_buffer + new_size);
    return true;
}

force_inline void AddrDescRW::reset_for_read() {
    assert(this->addr_descs_read_index == 0);
    this->cur_rw_ptr = this->addr_descs[0];
    this->cur_end_ptr = BufferAddrDesc * ((u8 *) this->cur_rw_ptr + sizeof(AddrDescFirstBuffer));
}

force_inline bool AddrDescRW::write(Py_ssize_t normal_len, Py_ssize_t unicode_len, Py_ssize_t unicode_kind) {
    if (unlikely(this->cur_rw_ptr >= this->cur_end_ptr)) {
        assert(this->cur_end_ptr == this->cur_rw_ptr);
        bool _c = this->resize();
        RETURN_ON_UNLIKELY_ERR(!_c);
    }
    assert(this->cur_rw_ptr < this->cur_end_ptr);
    this->cur_rw_ptr->m_normal_len = normal_len;
    this->cur_rw_ptr->m_unicode_size = unicode_len;
    this->cur_rw_ptr->m_unicode_kind = unicode_kind;
    this->cur_rw_ptr++;
    return true;
}

force_inline void AddrDescRW::mark_read() {
    if (unlikely(this->cur_rw_ptr == this->cur_end_ptr)) {
        this->addr_descs_read_index++;
        this->cur_rw_ptr = this->addr_descs[this->addr_descs_read_index];
        this->cur_end_ptr = BufferAddrDesc * ((u8 *) this->cur_rw_ptr + get_size_u8(this->addr_descs_read_index));
    }
    assert(this->cur_rw_ptr < this->cur_end_ptr);
    this->cur_rw_ptr++;
}

/*Writers*/

template<IndentLevel _IndentConfig, X86SIMDLevel __simd_level>
force_inline bool write_line_and_indent2(u8 *&dst, Py_ssize_t &write_count_left, EncodeBufferDesc &desc, Py_ssize_t indent_count) {
    static_assert(NEED_INDENT(_IndentConfig));

    Py_ssize_t need_space_count = INDENT_SIZE(_IndentConfig) * indent_count;
    Py_ssize_t desired_mem_count = (Py_ssize_t) size_align_up((usize) need_space_count + 1, 512 / 8);
    if (unlikely(desired_mem_count > write_count_left)) {
        bool _c = desc.resize<UCSKind::UCS1>(dst, write_count_left, desired_mem_count);
        RETURN_ON_UNLIKELY_ERR(!_c);
    }
    assert(desired_mem_count <= write_count_left);
    *dst++ = '\n';
    write_count_left--;
    constexpr u8 _FillCountOnce = 128 / 8;
    constexpr u8 _ToFill[_FillCountOnce] = {
            ' ',
            ' ',
            ' ',
            ' ', // 4
            ' ',
            ' ',
            ' ',
            ' ', // 8
            ' ',
            ' ',
            ' ',
            ' ', // 12
            ' ',
            ' ',
            ' ',
            ' ', // 16
    };
    static_assert(_ToFill[_FillCountOnce - 1] == ' ');
jump_fill:;
    while (need_space_count >= _FillCountOnce && write_count_left >= _FillCountOnce) {
        cpy_once_128_impl<__simd_level>(dst, _ToFill);
        dst += _FillCountOnce;
        write_count_left -= _FillCountOnce;
        need_space_count -= _FillCountOnce;
    }
    if (likely(need_space_count <= 0)) return true;
    // below is very unlikely.
    if (need_space_count <= write_count_left) {
        memcpy((void *) dst, (void *) _ToFill, need_space_count);
        dst += need_space_count;
        write_count_left -= need_space_count;
        return true;
    }
    assert(need_space_count > write_count_left && write_count_left < _FillCountOnce);
    memcpy((void *) dst, (void *) _ToFill, write_count_left);
    dst += write_count_left;
    need_space_count -= write_count_left;
    write_count_left = 0;
    if (!need_space_count) return true;
    desc.resize<UCSKind::UCS1>(dst, write_count_left, need_space_count);
    assert(write_count_left >= need_space_count);
    goto jump_fill;
}

template<IndentLevel _IndentConfig, bool _IsWriteObj, X86SIMDLevel __simd_level>
force_inline bool write_empty_ctn2(u8 *&dst, Py_ssize_t &write_count_left, EncodeBufferDesc &desc, Py_ssize_t indent_count, bool is_in_obj) {
    if constexpr (NEED_INDENT(_IndentConfig)) {
        if (!is_in_obj) {
            bool _c = write_line_and_indent2<_IndentConfig, __simd_level>(dst, write_count_left, desc, indent_count);
            RETURN_ON_UNLIKELY_ERR(!_c);
        }
    }
    constexpr u8 _Template[128 / 8] = {
            ('[' | ((u8) _IsWriteObj << 5)),
            (']' | ((u8) _IsWriteObj << 5)),
            ',',
    };
    constexpr Py_ssize_t _ToCopyCount = 3;
    if (unlikely(write_count_left < 128 / 8)) {
        bool _c = desc.resize<UCSKind::UCS1>(dst, write_count_left);
        RETURN_ON_UNLIKELY_ERR(!_c);
    }
    assert(write_count_left >= 128 / 8);
    cpy_once_128_impl<__simd_level>(dst, _Template);
    dst += _ToCopyCount;
    write_count_left -= _ToCopyCount;
    // if (likely(write_count_left >= 128 / 8)) {
    //     cpy_once_128_impl<__simd_level>(dst, _Template);
    //     dst += _ToCopyCount;
    //     write_count_left -= _ToCopyCount;
    // } else if (likely(write_count_left >= _ToCopyCount)) {
    //     memcpy((void *) dst, (void *) _Template, _ToCopyCount);
    //     dst += _ToCopyCount;
    //     write_count_left -= _ToCopyCount;
    // } else {
    //     Py_ssize_t second_part = _ToCopyCount - write_count_left;
    //     assert(second_part > 0 && second_part <= _ToCopyCount);
    //     Py_ssize_t i = 0;
    //     for (; i < write_count_left; ++i) {
    //         *dst++ = _Template[i];
    //     }
    //     write_count_left = 0;
    //     desc.resize<UCSKind::UCS1>(dst, write_count_left);
    //     assert(write_count_left >= 128 / 8);
    //     cpy_once_128_impl<__simd_level>(dst, _Template + i);
    //     dst += second_part;
    //     write_count_left -= second_part;
    // }
    return true;
}

template<IndentLevel _IndentConfig, bool _IsWriteObj, X86SIMDLevel __simd_level>
force_inline bool write_ctn_begin2(u8 *&dst, Py_ssize_t &write_count_left, EncodeBufferDesc &desc, Py_ssize_t &indent_count, bool &is_in_obj, bool *obj_stack) {
    // using uu = UCSType_t<__kind>;
    // ut *dst = (ut *) *dst_raw;
    if constexpr (NEED_INDENT(_IndentConfig)) {
        if (!is_in_obj) {
            bool _c = write_line_and_indent2<_IndentConfig, __simd_level>(dst, write_count_left, desc, indent_count);
            RETURN_ON_UNLIKELY_ERR(!_c);
        }
    }
    if (unlikely(!write_count_left)) {
        bool _c = desc.resize<UCSKind::UCS1>(dst, write_count_left);
        RETURN_ON_UNLIKELY_ERR(!_c);
    }
    assert(write_count_left > 0);
    constexpr u8 to_write = ('[' | ((u8) _IsWriteObj << 5));
    *dst++ = to_write;
    write_count_left--;
    // write current ctn type
    // new line, increase indent
    if (unlikely(indent_count >= PYYJSON_ENCODE_MAX_RECURSION)) {
        PyErr_SetString(JSONEncodeError, "Max recursion exceeded");
        return false;
    }
    obj_stack[indent_count++] = is_in_obj;
    is_in_obj = _IsWriteObj;
    return true;
}

template<IndentLevel _IndentConfig, bool _IsWriteObj, X86SIMDLevel __simd_level>
force_inline bool write_ctn_end2(u8 *&dst, Py_ssize_t &write_count_left, EncodeBufferDesc &desc, Py_ssize_t &indent_count, bool &is_in_obj, bool *obj_stack) {
    // indent_count--;
    erase_last();
    is_in_obj = obj_stack[--indent_count];
    if constexpr (NEED_INDENT(_IndentConfig)) {
        bool _c = write_line_and_indent2<_IndentConfig, __simd_level>(dst, write_count_left, desc, indent_count);
        RETURN_ON_UNLIKELY_ERR(!_c);
    }
    if (unlikely(write_count_left < 2)) {
        bool _c = desc.resize<UCSKind::UCS1>(dst, write_count_left);
        RETURN_ON_UNLIKELY_ERR(!_c);
    }
    assert(write_count_left >= 2);
    constexpr u8 _Template[2] = {
            (u8) (']' | ((u8) _IsWriteObj << 5)),
            (u8) ',',
    };
    memcpy(dst, &_Template[0], 2);
    dst += 2;
    write_count_left -= 2;
    return true;
}

template<IndentLevel _IndentConfig, X86SIMDLevel __simd_level>
force_inline write_true2(u8 *&dst, Py_ssize_t &write_count_left, EncodeBufferDesc &desc, Py_ssize_t &indent_count, bool is_in_obj) {
    if constexpr (NEED_INDENT(_IndentConfig)) {
        if (!is_in_obj) {
            bool _c = write_line_and_indent2<_IndentConfig, __simd_level>(dst, write_count_left, desc, indent_count);
            RETURN_ON_UNLIKELY_ERR(!_c);
        }
    }
    if (unlikely(write_count_left < 8)) {
        bool _c = desc.resize<UCSKind::UCS1>(dst, write_count_left);
        RETURN_ON_UNLIKELY_ERR(!_c);
    }
    assert(write_count_left >= 8);

    constexpr u8 _Template[8] = {
            (uu) 't',
            (uu) 'r',
            (uu) 'u',
            (uu) 'e',
            (uu) ',',
    };
    memcpy(dst, &_Template[0], 8);
    dst += 5;
    write_count_left -= 5;
    return true;
}


template<IndentLevel _IndentConfig, X86SIMDLevel __simd_level>
force_inline write_false2(u8 *&dst, Py_ssize_t &write_count_left, EncodeBufferDesc &desc, Py_ssize_t &indent_count, bool is_in_obj) {
    if constexpr (NEED_INDENT(_IndentConfig)) {
        if (!is_in_obj) {
            bool _c = write_line_and_indent2<_IndentConfig, __simd_level>(dst, write_count_left, desc, indent_count);
            RETURN_ON_UNLIKELY_ERR(!_c);
        }
    }
    if (unlikely(write_count_left < 8)) {
        bool _c = desc.resize<UCSKind::UCS1>(dst, write_count_left);
        RETURN_ON_UNLIKELY_ERR(!_c);
    }
    assert(write_count_left >= 8);

    constexpr u8 _Template[8] = {
            'f',
            'a',
            'l',
            's',
            'e',
            ','};
    memcpy(dst, &_Template[0], 8);
    dst += 6;
    write_count_left -= 6;
    return true;
}

template<IndentLevel _IndentConfig, X86SIMDLevel __simd_level>
force_inline write_null2(u8 *&dst, Py_ssize_t &write_count_left, EncodeBufferDesc &desc, Py_ssize_t &indent_count, bool is_in_obj) {
    if constexpr (NEED_INDENT(_IndentConfig)) {
        if (!is_in_obj) {
            bool _c = write_line_and_indent2<_IndentConfig, __simd_level>(dst, write_count_left, desc, indent_count);
            RETURN_ON_UNLIKELY_ERR(!_c);
        }
    }
    if (unlikely(write_count_left < 8)) {
        bool _c = desc.resize<UCSKind::UCS1>(dst, write_count_left);
        RETURN_ON_UNLIKELY_ERR(!_c);
    }
    assert(write_count_left >= 8);

    constexpr u8 _Template[8] = {
            'n',
            'u',
            'l',
            'l',
            ','};
    memcpy(dst, &_Template[0], 8);
    dst += 5;
    write_count_left -= 5;
    return true;
}

template<IndentLevel _IndentConfig, X86SIMDLevel __simd_level>
force_inline write_zero2(u8 *&dst, Py_ssize_t &write_count_left, EncodeBufferDesc &desc, Py_ssize_t &indent_count, bool is_in_obj) {
    if constexpr (NEED_INDENT(_IndentConfig)) {
        if (!is_in_obj) {
            bool _c = write_line_and_indent2<_IndentConfig, __simd_level>(dst, write_count_left, desc, indent_count);
            RETURN_ON_UNLIKELY_ERR(!_c);
        }
    }
    if (unlikely(write_count_left < 2)) {
        bool _c = desc.resize<UCSKind::UCS1>(dst, write_count_left);
        RETURN_ON_UNLIKELY_ERR(!_c);
    }
    assert(write_count_left >= 2);

    constexpr u8 _Template[2] = {
            '0',
            ',',
    };
    memcpy(dst, &_Template[0], 2);
    dst += 2;
    write_count_left -= 2;
    return true;
}

template<IndentLevel _IndentConfig, X86SIMDLevel __simd_level, bool _IsSigned>
force_inline write_nan2(u8 *&dst, Py_ssize_t &write_count_left, EncodeBufferDesc &desc, Py_ssize_t &indent_count, bool is_in_obj) {
    if constexpr (NEED_INDENT(_IndentConfig)) {
        if (!is_in_obj) {
            bool _c = write_line_and_indent2<_IndentConfig, __simd_level>(dst, write_count_left, desc, indent_count);
            RETURN_ON_UNLIKELY_ERR(!_c);
        }
    }
    if (unlikely(write_count_left < 8)) {
        bool _c = desc.resize<UCSKind::UCS1>(dst, write_count_left);
        RETURN_ON_UNLIKELY_ERR(!_c);
    }
    assert(write_count_left >= 8);

    if constexpr (_IsSigned) {
        constexpr u8 _Template[8] = {
                '-',
                'n',
                'a',
                'n',
                ',',
        };
        memcpy(dst, &_Template[0], 8);
        dst += 5;
        write_count_left -= 5;
    } else {
        constexpr u8 _Template[8] = {
                'n',
                'a',
                'n',
                ',',
        };
        memcpy(dst, &_Template[0], 8);
        dst += 4;
        write_count_left -= 4;
    }

    return true;
}


template<IndentLevel _IndentConfig, X86SIMDLevel __simd_level, bool _IsSigned>
force_inline write_inf2(u8 *&dst, Py_ssize_t &write_count_left, EncodeBufferDesc &desc, Py_ssize_t &indent_count, bool is_in_obj) {
    if constexpr (NEED_INDENT(_IndentConfig)) {
        if (!is_in_obj) {
            bool _c = write_line_and_indent2<_IndentConfig, __simd_level>(dst, write_count_left, desc, indent_count);
            RETURN_ON_UNLIKELY_ERR(!_c);
        }
    }
    if (unlikely(write_count_left < 8)) {
        bool _c = desc.resize<UCSKind::UCS1>(dst, write_count_left);
        RETURN_ON_UNLIKELY_ERR(!_c);
    }
    assert(write_count_left >= 8);

    if constexpr (_IsSigned) {
        constexpr u8 _Template[8] = {
                '-',
                'i',
                'n',
                'f',
                ',',
        };
        memcpy(dst, &_Template[0], 8);
        dst += 5;
        write_count_left -= 5;
    } else {
        constexpr u8 _Template[8] = {
                'i',
                'n',
                'f',
                ',',
        };
        memcpy(dst, &_Template[0], 8);
        dst += 4;
        write_count_left -= 4;
    }

    return true;
}

force_inline bool write_i642() {
    assert(false);
    return false;
}

force_inline bool write_f642() {
    assert(false);
    return false;
}

force_inline bool write_u642() {
    assert(false);
    return false;
}

/*Utils*/
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

force_inline void update_string_info2(UCSKind &cur_ucs_kind, bool &is_all_ascii, PyObject *val) {
    cur_ucs_kind = std::max(cur_ucs_kind, static_cast<UCSKind>(PyUnicode_KIND(val)));
    is_all_ascii = is_all_ascii && PyUnicode_IS_ASCII(val);
}

/*Encode*/

force_noinline PyObject *
pyyjson_dumps_read_AB_buffers_write_unicode() {
}


#define PROCESS_VAL(val)                                                                   \
    PyFastTypes fast_type = fast_type_check(val);                                          \
    switch (fast_type) {                                                                   \
        case PyFastTypes::T_Unicode: {                                                     \
            update_string_info2(cur_ucs_kind, is_all_ascii, val);                          \
            bool _c = (write_str_prepare() && desc.write_unicode(val) && write_str_end()); \
            if (unlikely(!_c)) {                                                           \
                goto fail;                                                                 \
            }                                                                              \
            break;                                                                         \
        }                                                                                  \
        case PyFastTypes::T_Long: {                                                        \
            if (unlikely(!write_PyLongOp(val, op_desc))) {                                 \
                goto fail;                                                                 \
            }                                                                              \
            break;                                                                         \
        }                                                                                  \
        case PyFastTypes::T_False: {                                                       \
            op_desc.write_simple_op(EncodeOpType::FALSE_VALUE);                            \
            break;                                                                         \
        }                                                                                  \
        case PyFastTypes::T_True: {                                                        \
            op_desc.write_simple_op(EncodeOpType::TRUE_VALUE);                             \
            break;                                                                         \
        }                                                                                  \
        case PyFastTypes::T_None: {                                                        \
            op_desc.write_simple_op(EncodeOpType::NULL_VALUE);                             \
            break;                                                                         \
        }                                                                                  \
        case PyFastTypes::T_Float: {                                                       \
            if (unlikely(!write_PyFloatOp(val, op_desc))) {                                \
                goto fail;                                                                 \
            }                                                                              \
            break;                                                                         \
        }                                                                                  \
        case PyFastTypes::T_List: {                                                        \
            Py_ssize_t this_list_size = PyList_Size(val);                                  \
            if (unlikely(cur_list_size == 0)) {                                            \
                op_desc.write_simple_op(EncodeOpType::ARRAY_EMPTY);                        \
            } else {                                                                       \
                CTN_SIZE_GROW();                                                           \
                CtnType *cur_write_ctn = ctn_stack + ++last_pos_stack_index;               \
                cur_write_ctn->ctn = cur_obj;                                              \
                cur_write_ctn->index = cur_pos;                                            \
                cur_obj = val;                                                             \
                cur_pos = 0;                                                               \
                op_desc.write_simple_op(EncodeOpType::ARRAY_BEGIN);                        \
                GOTO_ARR_VAL_BEGIN_WITH_SIZE(this_list_size);                              \
            }                                                                              \
            break;                                                                         \
        }                                                                                  \
        case PyFastTypes::T_Dict: {                                                        \
            if (unlikely(PyDict_Size(val) == 0)) {                                         \
                op_desc.write_simple_op(EncodeOpType::DICT_EMPTY);                         \
            } else {                                                                       \
                CTN_SIZE_GROW();                                                           \
                CtnType *cur_write_ctn = ctn_stack + ++last_pos_stack_index;               \
                cur_write_ctn->ctn = cur_obj;                                              \
                cur_write_ctn->index = cur_pos;                                            \
                cur_obj = val;                                                             \
                cur_pos = 0;                                                               \
                op_desc.write_simple_op(EncodeOpType::DICT_BEGIN);                         \
                goto dict_pair_begin;                                                      \
            }                                                                              \
            break;                                                                         \
        }                                                                                  \
        case PyFastTypes::T_Tuple: {                                                       \
            Py_ssize_t this_list_size = PyTuple_Size(val);                                 \
            if (unlikely(this_list_size == 0)) {                                           \
                op_desc.write_simple_op(EncodeOpType::ARRAY_EMPTY);                        \
            } else {                                                                       \
                CTN_SIZE_GROW();                                                           \
                CtnType *cur_write_ctn = ctn_stack + ++last_pos_stack_index;               \
                cur_write_ctn->ctn = cur_obj;                                              \
                cur_write_ctn->index = cur_pos;                                            \
                cur_obj = val;                                                             \
                cur_pos = 0;                                                               \
                op_desc.write_simple_op(EncodeOpType::ARRAY_BEGIN);                        \
                GOTO_TUPLE_VAL_BEGIN_WITH_SIZE(this_list_size);                            \
            }                                                                              \
            break;                                                                         \
        }                                                                                  \
        default: {                                                                         \
            PyErr_SetString(JSONEncodeError, "Unsupported type");                          \
            goto fail;                                                                     \
        }                                                                                  \
    }

template<IndentLevel _IndentConfig, X86SIMDLevel __simd_level>
force_noinline PyObject *pyyjson_dumps_obj_AB_buffers(PyObject *in_obj, DumpOption *options) {
    //EncodeWriteOpConfig cfg;
    //ALIAS_NAMES2();
    // EncodeOpDescriber op_desc = EncodeOpDescriber::init();
    Py_ssize_t last_pos_stack_index = -1;
    PyObject *cur_obj;
    Py_ssize_t cur_pos;
    UCSKind cur_ucs_kind = UCSKind::UCS1;
    bool is_all_ascii = true;
    Py_ssize_t indent_count = 0;
    // temp variables storing key and value
    Py_ssize_t cur_list_size; // cache for each list, no need to check it repeatedly
    PyObject *key, *val;
    // buffer description
    EncodeBufferDesc desc;
    AddrDescRW addr_desc;
    bool is_cur_obj = false;
    bool obj_stack[PYYJSON_ENCODE_MAX_RECURSION];
    CtnType ctn_stack[PYYJSON_ENCODE_MAX_RECURSION];
    // final return value
    PyObject *result;
    // initialize
    cur_obj = in_obj;
    cur_pos = 0;
    desc = EncodeBufferDesc::init();
    addr_desc = AddrDescRW::init();

    // begin encoding

    if (PyDict_CheckExact(cur_obj)) {
        if (unlikely(PyDict_Size(cur_obj) == 0)) {
            auto write_left = desc.get_write_count_left<UCSKind::UCS1>();
            bool _c = write_empty_ctn2<_IndentConfig, true, __simd_level>(desc.cur_rw_addr, write_left, desc, 0, false);
            assert(_c);
            // op_desc.write_simple_op(EncodeOpType::DICT_EMPTY);
            // APPEND_OP(DICT_EMPTY);
            goto success;
        }
        {
            auto write_left = desc.get_write_count_left<UCSKind::UCS1>();
            bool _c = write_ctn_begin2<_IndentConfig, true, __simd_level>(desc.cur_rw_addr, write_left, desc, indent_count, is_cur_obj, obj_stack);
            assert(_c);
        }
        // op_desc.write_simple_op(EncodeOpType::DICT_BEGIN);
        // APPEND_OP(DICT_BEGIN);
        cur_pos = 0;
        goto dict_pair_begin;
    } else if (PyList_CheckExact(cur_obj)) {
        cur_list_size = PyList_Size(cur_obj);
        if (unlikely(cur_list_size == 0)) {
            auto write_left = desc.get_write_count_left<UCSKind::UCS1>();
            bool _c = write_empty_ctn2<_IndentConfig, false, __simd_level>(desc.cur_rw_addr, write_left, desc, 0, false);
            assert(_c);
            // APPEND_OP(ARRAY_EMPTY);
            // op_desc.write_simple_op(EncodeOpType::ARRAY_EMPTY);
            goto success;
        }
        {
            auto write_left = desc.get_write_count_left<UCSKind::UCS1>();
            bool _c = write_ctn_begin2<_IndentConfig, false, __simd_level>(desc.cur_rw_addr, write_left, desc, indent_count, is_cur_obj, obj_stack);
            assert(_c);
        }
        // op_desc.write_simple_op(EncodeOpType::ARRAY_BEGIN);
        // APPEND_OP(ARRAY_BEGIN);
        GOTO_ARR_VAL_BEGIN_WITH_SIZE(cur_list_size); // result in a no-op
    } else {
        if (unlikely(!PyTuple_CheckExact(cur_obj))) {
            goto fail_ctntype;
        }
        cur_list_size = PyTuple_Size(cur_obj);
        if (unlikely(cur_list_size == 0)) {
            // op_desc.write_simple_op(EncodeOpType::ARRAY_EMPTY);
            auto write_left = desc.get_write_count_left<UCSKind::UCS1>();
            bool _c = write_empty_ctn2<_IndentConfig, false, __simd_level>(desc.cur_rw_addr, write_left, desc, 0, false);
            assert(_c);
            // APPEND_OP(ARRAY_EMPTY);
            goto success;
        }
        {
            auto write_left = desc.get_write_count_left<UCSKind::UCS1>();
            bool _c = write_ctn_begin2<_IndentConfig, false, __simd_level>(desc.cur_rw_addr, write_left, desc, indent_count, is_cur_obj, obj_stack);
            assert(_c);
        }
        // op_desc.write_simple_op(EncodeOpType::ARRAY_BEGIN);
        // APPEND_OP(ARRAY_BEGIN);
        GOTO_TUPLE_VAL_BEGIN_WITH_SIZE(cur_list_size); // result in a no-op
    }

dict_pair_begin:;
    assert(PyDict_Size(cur_obj) != 0);

    if (PyDict_Next(cur_obj, &cur_pos, &key, &val)) {
        if (unlikely(!PyUnicode_CheckExact(key))) {
            goto fail_keytype;
        }
        update_string_info2(cur_ucs_kind, is_all_ascii, key);
        // update_string_info(cfg, key);
        write_key_prepare();
        desc.write_unicode(key);
        write_key_end();
        // op_desc.write_op_with_data(EncodeOpType::KEY, key);
        //
        PROCESS_VAL(val);
        //
        goto dict_pair_begin;
    } else {
        // dict end
        op_desc.write_simple_op(EncodeOpType::DICT_END);
        if (unlikely(last_pos_stack_index == -1)) {
            goto success;
        }
        CtnType *last_pos = ctn_stack + last_pos_stack_index--;
        cur_obj = last_pos->ctn;
        cur_pos = last_pos->index;
        if (PyDict_CheckExact(cur_obj)) {
            goto dict_pair_begin;
        } else if (PyList_CheckExact(cur_obj)) {
            GOTO_ARR_VAL_BEGIN_WITH_SIZE(PyList_GET_SIZE(cur_obj));
        } else {
            assert(PyTuple_CheckExact(cur_obj));
            GOTO_TUPLE_VAL_BEGIN_WITH_SIZE(PyTuple_GET_SIZE(cur_obj));
        }
    }

    Py_UNREACHABLE();
arr_val_begin:;
    assert(cur_list_size != 0);

    if (cur_pos < cur_list_size) {
        val = PyList_GET_ITEM(cur_obj, cur_pos);
        cur_pos++;
        //
        PROCESS_VAL(val);
        //
        goto arr_val_begin;
    } else {
        // list end
        op_desc.write_simple_op(EncodeOpType::ARRAY_END);
        if (unlikely(last_pos_stack_index == -1)) {
            goto success;
        }
        CtnType *last_pos = ctn_stack + last_pos_stack_index--;
        cur_obj = last_pos->ctn;
        cur_pos = last_pos->index;
        if (PyDict_CheckExact(cur_obj)) {
            goto dict_pair_begin;
        } else if (PyList_CheckExact(cur_obj)) {
            GOTO_ARR_VAL_BEGIN_WITH_SIZE(PyList_GET_SIZE(cur_obj));
        } else {
            assert(PyTuple_CheckExact(cur_obj));
            GOTO_TUPLE_VAL_BEGIN_WITH_SIZE(PyTuple_GET_SIZE(cur_obj));
        }
    }
    Py_UNREACHABLE();
tuple_val_begin:;
    assert(cur_list_size != 0);

    if (cur_pos < cur_list_size) {
        val = PyTuple_GET_ITEM(cur_obj, cur_pos);
        cur_pos++;
        //
        PROCESS_VAL(val);
        //
        goto tuple_val_begin;
    } else {
        // list end
        op_desc.write_simple_op(EncodeOpType::ARRAY_END);
        if (unlikely(last_pos_stack_index == -1)) {
            goto success;
        }
        CtnType *last_pos = ctn_stack + last_pos_stack_index--;
        cur_obj = last_pos->ctn;
        cur_pos = last_pos->index;
        if (PyDict_CheckExact(cur_obj)) {
            goto dict_pair_begin;
        } else if (PyList_CheckExact(cur_obj)) {
            GOTO_ARR_VAL_BEGIN_WITH_SIZE(PyList_GET_SIZE(cur_obj));
        } else {
            assert(PyTuple_CheckExact(cur_obj));
            GOTO_TUPLE_VAL_BEGIN_WITH_SIZE(PyTuple_GET_SIZE(cur_obj));
        }
    }
    Py_UNREACHABLE();
success:
    assert(last_pos_stack_index == -1);
    op_desc.write_simple_op(EncodeOpType::END);
    result = pyyjson_dumps_read_op_write_str_call_jump(options, &cfg, &op_desc);

    // cleanup. TODO
    if (!result && !PyErr_Occurred()) {
        PyErr_SetString(JSONEncodeError, "Unknown encode error");
    }
    op_desc.release();
    return result;

fail:
    if (!PyErr_Occurred()) {
        PyErr_SetString(JSONEncodeError, "Unknown encode error");
    }
    op_desc.release();
    return nullptr;

fail_ctntype:
    PyErr_SetString(JSONEncodeError, "Unknown container type");
    goto fail;

fail_keytype:
    PyErr_SetString(JSONEncodeError, "Key must be a string");
    goto fail;
}

#undef ENCODE_MOVE_SRC_DST
#undef RETURN_ON_UNLIKELY_ERR
