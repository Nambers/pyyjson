#include "commondef/i_in.inl.h"
#include "commondef/r_in.inl.h"
#include "commondef/w_in.inl.h"
#include "simd/simd_detect.h"
#include "simd/simd_impl.h"
#include "unicode/include/indent.h"
#include "unicode/include/reserve.h"
#include <immintrin.h>

// encode_simd_utils.inl
#define CHECK_ESCAPE_IMPL_GET_MASK PYYJSON_CONCAT2(check_escape_impl_get_mask, COMPILE_READ_UCS_LEVEL)
#define GET_DONE_COUNT_FROM_MASK PYYJSON_CONCAT2(get_done_count_from_mask, COMPILE_READ_UCS_LEVEL)
#define WRITE_SIMD_256_WITH_WRITEMASK PYYJSON_CONCAT2(write_simd_256_with_writemask, COMPILE_WRITE_UCS_LEVEL)
#define BACK_WRITE_SIMD256_WITH_TAIL_LEN PYYJSON_CONCAT3(back_write_simd256_with_tail_len, COMPILE_READ_UCS_LEVEL, COMPILE_WRITE_UCS_LEVEL)
// define here
#define _CONTROL_SEQ_TABLE PYYJSON_CONCAT2(_ControlSeqTable, COMPILE_WRITE_UCS_LEVEL)
// define here
#define VECTOR_WRITE_UNICODE_IMPL PYYJSON_CONCAT4(vector_write_unicode_impl, COMPILE_INDENT_LEVEL, COMPILE_READ_UCS_LEVEL, COMPILE_WRITE_UCS_LEVEL)
#define VECTOR_WRITE_UNICODE_TRAILING_IMPL PYYJSON_CONCAT4(vector_write_unicode_trailing_impl, COMPILE_INDENT_LEVEL, COMPILE_READ_UCS_LEVEL, COMPILE_WRITE_UCS_LEVEL)
#define VECTOR_WRITE_ESCAPE_IMPL PYYJSON_CONCAT4(vector_write_escape_impl, COMPILE_INDENT_LEVEL, COMPILE_READ_UCS_LEVEL, COMPILE_WRITE_UCS_LEVEL)
#define CHECK_ESCAPE_TAIL_IMPL_GET_MASK_512 PYYJSON_CONCAT2(check_escape_tail_impl_get_mask_512, COMPILE_READ_UCS_LEVEL)
#define WRITE_SIMD_IMPL PYYJSON_CONCAT3(write_simd_impl, COMPILE_READ_UCS_LEVEL, COMPILE_WRITE_UCS_LEVEL)
#define MASK_ELEVATE_WRITE_512 PYYJSON_CONCAT3(mask_elevate_write_512, COMPILE_READ_UCS_LEVEL, COMPILE_WRITE_UCS_LEVEL)

// forward declaration
force_inline SIMD_MASK_TYPE CHECK_ESCAPE_IMPL_GET_MASK(const _FROM_TYPE *restrict src, SIMD_TYPE *restrict SIMD_VAR);
force_inline u32 GET_DONE_COUNT_FROM_MASK(SIMD_MASK_TYPE mask);
#if SIMD_BIT_SIZE == 512
force_inline SIMD_MASK_TYPE CHECK_ESCAPE_TAIL_IMPL_GET_MASK_512(SIMD_512 z, SIMD_MASK_TYPE rw_mask);
#endif
force_inline void WRITE_SIMD_IMPL(_TARGET_TYPE *dst, SIMD_TYPE SIMD_VAR);


#if COMPILE_READ_UCS_LEVEL == 1 && COMPILE_INDENT_LEVEL == 0
static _TARGET_TYPE _CONTROL_SEQ_TABLE[(_Slash + 1) * 8] = {
        CONTROL_SEQ_ESCAPE_PREFIX, '0', '0', CONTROL_SEQ_ESCAPE_SUFFIX,  // 0
        CONTROL_SEQ_ESCAPE_PREFIX, '0', '1', CONTROL_SEQ_ESCAPE_SUFFIX,  // 1
        CONTROL_SEQ_ESCAPE_PREFIX, '0', '2', CONTROL_SEQ_ESCAPE_SUFFIX,  // 2
        CONTROL_SEQ_ESCAPE_PREFIX, '0', '3', CONTROL_SEQ_ESCAPE_SUFFIX,  // 3
        CONTROL_SEQ_ESCAPE_PREFIX, '0', '4', CONTROL_SEQ_ESCAPE_SUFFIX,  // 4
        CONTROL_SEQ_ESCAPE_PREFIX, '0', '5', CONTROL_SEQ_ESCAPE_SUFFIX,  // 5
        CONTROL_SEQ_ESCAPE_PREFIX, '0', '6', CONTROL_SEQ_ESCAPE_SUFFIX,  // 6
        CONTROL_SEQ_ESCAPE_PREFIX, '0', '7', CONTROL_SEQ_ESCAPE_SUFFIX,  // 7
        '\\', 'b', CONTROL_SEQ_ESCAPE_MIDDLE, CONTROL_SEQ_ESCAPE_SUFFIX, // 8
        '\\', 't', CONTROL_SEQ_ESCAPE_MIDDLE, CONTROL_SEQ_ESCAPE_SUFFIX, // 9
        '\\', 'n', CONTROL_SEQ_ESCAPE_MIDDLE, CONTROL_SEQ_ESCAPE_SUFFIX, // 10
        CONTROL_SEQ_ESCAPE_PREFIX, '0', 'b', CONTROL_SEQ_ESCAPE_SUFFIX,  // 11
        '\\', 'f', CONTROL_SEQ_ESCAPE_MIDDLE, CONTROL_SEQ_ESCAPE_SUFFIX, // 12
        '\\', 'r', CONTROL_SEQ_ESCAPE_MIDDLE, CONTROL_SEQ_ESCAPE_SUFFIX, // 13
        CONTROL_SEQ_ESCAPE_PREFIX, '0', 'e', CONTROL_SEQ_ESCAPE_SUFFIX,  // 14
        CONTROL_SEQ_ESCAPE_PREFIX, '0', 'f', CONTROL_SEQ_ESCAPE_SUFFIX,  // 15
        CONTROL_SEQ_ESCAPE_PREFIX, '1', '0', CONTROL_SEQ_ESCAPE_SUFFIX,  // 16
        CONTROL_SEQ_ESCAPE_PREFIX, '1', '1', CONTROL_SEQ_ESCAPE_SUFFIX,  // 17
        CONTROL_SEQ_ESCAPE_PREFIX, '1', '2', CONTROL_SEQ_ESCAPE_SUFFIX,  // 18
        CONTROL_SEQ_ESCAPE_PREFIX, '1', '3', CONTROL_SEQ_ESCAPE_SUFFIX,  // 19
        CONTROL_SEQ_ESCAPE_PREFIX, '1', '4', CONTROL_SEQ_ESCAPE_SUFFIX,  // 20
        CONTROL_SEQ_ESCAPE_PREFIX, '1', '5', CONTROL_SEQ_ESCAPE_SUFFIX,  // 21
        CONTROL_SEQ_ESCAPE_PREFIX, '1', '6', CONTROL_SEQ_ESCAPE_SUFFIX,  // 22
        CONTROL_SEQ_ESCAPE_PREFIX, '1', '7', CONTROL_SEQ_ESCAPE_SUFFIX,  // 23
        CONTROL_SEQ_ESCAPE_PREFIX, '1', '8', CONTROL_SEQ_ESCAPE_SUFFIX,  // 24
        CONTROL_SEQ_ESCAPE_PREFIX, '1', '9', CONTROL_SEQ_ESCAPE_SUFFIX,  // 25
        CONTROL_SEQ_ESCAPE_PREFIX, '1', 'a', CONTROL_SEQ_ESCAPE_SUFFIX,  // 26
        CONTROL_SEQ_ESCAPE_PREFIX, '1', 'b', CONTROL_SEQ_ESCAPE_SUFFIX,  // 27
        CONTROL_SEQ_ESCAPE_PREFIX, '1', 'c', CONTROL_SEQ_ESCAPE_SUFFIX,  // 28
        CONTROL_SEQ_ESCAPE_PREFIX, '1', 'd', CONTROL_SEQ_ESCAPE_SUFFIX,  // 29
        CONTROL_SEQ_ESCAPE_PREFIX, '1', 'e', CONTROL_SEQ_ESCAPE_SUFFIX,  // 30
        CONTROL_SEQ_ESCAPE_PREFIX, '1', 'f', CONTROL_SEQ_ESCAPE_SUFFIX,  // 31
        CONTROL_SEQ_ESCAPE_FULL_ZERO_REPEAT2,                            // 32 ~ 33
        '\\', '"', CONTROL_SEQ_ESCAPE_MIDDLE, CONTROL_SEQ_ESCAPE_SUFFIX, // 34
        CONTROL_SEQ_ESCAPE_FULL_ZERO_REPEAT32,                           // 35 ~ 66
        CONTROL_SEQ_ESCAPE_FULL_ZERO_REPEAT16,                           // 67 ~ 82
        CONTROL_SEQ_ESCAPE_FULL_ZERO_REPEAT8,                            // 83 ~ 90
        CONTROL_SEQ_ESCAPE_FULL_ZERO,                                    // 91
        '\\', '\\'                                                       // 92
};
#endif // COMPILE_READ_UCS_LEVEL == 1 && COMPILE_INDENT_LEVEL == 0


force_noinline UnicodeVector *VECTOR_WRITE_ESCAPE_IMPL(UnicodeVector **restrict vec_addr, const _FROM_TYPE *restrict src, Py_ssize_t len, Py_ssize_t additional_len) {
    UnicodeVector *vec = *vec_addr;
    _TARGET_TYPE *writer = _WRITER(vec);
    const _FROM_TYPE *src_end = src + len;
    while (src < src_end) {
        _TARGET_TYPE srcval = (_TARGET_TYPE)*src;
        usize unicode_point = (usize)srcval;
        Py_ssize_t copy_count = (unicode_point <= _Slash) ? _ControlJump[unicode_point] : 0;
        if (likely(!copy_count)) {
            *writer++ = srcval;
        } else {
            _TARGET_TYPE *copy_ptr = &_CONTROL_SEQ_TABLE[unicode_point * 8];
            if (copy_count == 2) {
                _WRITER(vec) = writer;
                vec = VEC_RESERVE(vec_addr, 2 + len + TAIL_PADDING / sizeof(_TARGET_TYPE) + additional_len);
                RETURN_ON_UNLIKELY_ERR(!vec);
                writer = _WRITER(vec);
                memcpy((void *)writer, (const void *)copy_ptr, 2 * sizeof(_TARGET_TYPE));
                writer += 2;
            } else {
                assert(6 == copy_count);
                _WRITER(vec) = writer;
#if COMPILE_WRITE_UCS_LEVEL < 4 || SIZEOF_VOID_P == 8
                const usize _CopyLen = 8;
#else //  COMPILE_WRITE_UCS_LEVEL == 4 && SIZEOF_VOID_P < 8
                const usize _CopyLen = 6;
#endif
                vec = VEC_RESERVE(vec_addr, _CopyLen + len + TAIL_PADDING / sizeof(_TARGET_TYPE) + additional_len);
                RETURN_ON_UNLIKELY_ERR(!vec);
                writer = _WRITER(vec);
                memcpy((void *)writer, (const void *)copy_ptr, _CopyLen * sizeof(_TARGET_TYPE));
                writer += 6;
            }
        }
        src++;
    }
    _WRITER(vec) = writer;
    return vec;
}

force_inline UnicodeVector *VECTOR_WRITE_UNICODE_TRAILING_IMPL(const _FROM_TYPE *src, Py_ssize_t len, UnicodeVector **vec_addr) {
    assert(vec_addr);
    UnicodeVector *vec = *vec_addr;
#if SIMD_BIT_SIZE == 512
    SIMD_512 z;
#    if COMPILE_READ_UCS_LEVEL == 1
#        define _MASKZ_LOADU _mm512_maskz_loadu_epi8
#        define _MASK_STOREU _mm512_mask_storeu_epi8
#    elif COMPILE_READ_UCS_LEVEL == 2
#        define _MASKZ_LOADU _mm512_maskz_loadu_epi16
#        define _MASK_STOREU _mm512_mask_storeu_epi16
#    else // COMPILE_READ_UCS_LEVEL == 4
#        define _MASKZ_LOADU _mm512_maskz_loadu_epi32
#        define _MASK_STOREU _mm512_mask_storeu_epi32
#    endif
    u64 rw_mask, tail_mask;
    rw_mask = ((u64)1 << (usize)len) - 1;
    z = _MASKZ_LOADU(rw_mask, (const void *)src);
    tail_mask = CHECK_ESCAPE_TAIL_IMPL_GET_MASK_512(z, rw_mask);
    if (likely(check_mask_zero(tail_mask))) {
#    if COMPILE_READ_UCS_LEVEL == COMPILE_WRITE_UCS_LEVEL
        _MASK_STOREU((void *)_WRITER(vec), rw_mask, z);
#    else
        MASK_ELEVATE_WRITE_512(_WRITER(vec), z, len);
#    endif
        _WRITER(vec) += len;
    } else {
#    if COMPILE_READ_UCS_LEVEL == 1
        usize tzcnt = (usize)u64_tz_bits(tail_mask);
#    else
        usize tzcnt = (usize)u32_tz_bits((u32)tail_mask);
#    endif
        assert(tzcnt < (usize)len);
#    if COMPILE_READ_UCS_LEVEL == COMPILE_WRITE_UCS_LEVEL
        _MASK_STOREU((void *)_WRITER(vec), ((u64)1 << tzcnt) - 1, z);
#    else
        if (tzcnt) MASK_ELEVATE_WRITE_512(_WRITER(vec), z, tzcnt);
#    endif
        _WRITER(vec) += tzcnt;
        vec = VECTOR_WRITE_ESCAPE_IMPL(vec_addr, src + tzcnt, len - tzcnt, 0);
        RETURN_ON_UNLIKELY_ERR(!vec);
    }
#    undef _MASKZ_LOADU
#    undef _MASK_STOREU
#elif SIMD_BIT_SIZE == 256
    __m256i y;
    const _FROM_TYPE *load_start = src + len - CHECK_COUNT_MAX;
    _TARGET_TYPE *store_start = _WRITER(vec) + len - CHECK_COUNT_MAX;
    __m256i mask, check_mask;
#    define MASK_READER PYYJSON_CONCAT2(read_tail_mask_table, READ_BIT_SIZE)
    mask = load_256_aligned(MASK_READER(CHECK_COUNT_MAX - len));
#    undef MASK_READER
    check_mask = CHECK_ESCAPE_IMPL_GET_MASK(load_start, &y);
    check_mask = simd_and_256(check_mask, mask);
    if (likely(check_mask_zero(check_mask))) {
#    if COMPILE_READ_UCS_LEVEL == COMPILE_WRITE_UCS_LEVEL
        WRITE_SIMD_256_WITH_WRITEMASK(store_start, y, mask);
#    else
        BACK_WRITE_SIMD256_WITH_TAIL_LEN(store_start, y, len);
#    endif
        _WRITER(vec) += len;
    } else {
        vec = VECTOR_WRITE_ESCAPE_IMPL(vec_addr, src, len, 0);
        RETURN_ON_UNLIKELY_ERR(!vec);
    }
#else // SIMD_BIT_SIZE == 128
    // TODO
    assert(len < CHECK_COUNT_MAX);
    SIMD_128 x, mask, check_mask;
    const _FROM_TYPE *load_start = src + len - CHECK_COUNT_MAX;
    _TARGET_TYPE *store_start = _WRITER(vec) + len - CHECK_COUNT_MAX;
#    define MASK_TABLE_READER PYYJSON_CONCAT2(read_tail_mask_table, READ_BIT_SIZE)
    mask = load_128_aligned(MASK_TABLE_READER(CHECK_COUNT_MAX - len));
#    undef MASK_TABLE_READER
    check_mask = CHECK_ESCAPE_IMPL_GET_MASK(load_start, &x);
    check_mask = simd_and_128(check_mask, mask);
    if (likely(check_mask_zero(check_mask))) {
#    if COMPILE_READ_UCS_LEVEL == COMPILE_WRITE_UCS_LEVEL
#        if __SSE4_1__
        x = blendv_128(load_128((const void *)store_start), x, mask);
        write_128((void *)store_start, x);
#        else  // < __SSE4_1__
        x = runtime_right_shift_128bits(x, COMPILE_READ_UCS_LEVEL * (int)(CHECK_COUNT_MAX - len));
        write_128(_WRITER(vec), x);
#        endif // __SSE4_1__
#    else      // COMPILE_READ_UCS_LEVEL != COMPILE_WRITE_UCS_LEVEL
        x = runtime_right_shift_128bits(x, COMPILE_READ_UCS_LEVEL * (int)(CHECK_COUNT_MAX - len));
        WRITE_SIMD_IMPL(_WRITER(vec), x);
#    endif     // COMPILE_READ_UCS_LEVEL == COMPILE_WRITE_UCS_LEVEL
        _WRITER(vec) += len;
    } else {
        vec = VECTOR_WRITE_ESCAPE_IMPL(vec_addr, src, len, 0);
        RETURN_ON_UNLIKELY_ERR(!vec);
    }
#endif
    return vec;
}

static_assert(sizeof(PyASCIIObject) >= 16, "sizeof(PyASCIIObject) == ?");
#if SIZEOF_VOID_P == 8
// avx and above may be enabled.
static_assert(sizeof(PyASCIIObject) >= 32, "sizeof(PyASCIIObject) == ?");
#endif

force_inline UnicodeVector *VECTOR_WRITE_UNICODE_IMPL(UnicodeVector **restrict vec_addr, _FROM_TYPE *src, Py_ssize_t len) {
    UnicodeVector *vec = *vec_addr;
    usize total_size = (usize)len;
    __m128i x;
#if SIMD_BIT_SIZE >= 256
    __m256i y;
#endif
#if SIMD_BIT_SIZE >= 512
    __m512i z;
#endif
    SIMD_MASK_TYPE mask;
    // SIMD_BIT_MASK_TYPE bit_mask;
    bool _c;
    while (len >= CHECK_COUNT_MAX) {
        mask = CHECK_ESCAPE_IMPL_GET_MASK(src, &SIMD_VAR);
        WRITE_SIMD_IMPL(_WRITER(vec), SIMD_VAR);
        if (likely(check_mask_zero(mask))) {
            src += CHECK_COUNT_MAX;
            _WRITER(vec) += CHECK_COUNT_MAX;
            len -= CHECK_COUNT_MAX;
        } else {
            u32 done_count = GET_DONE_COUNT_FROM_MASK(mask);
            assert(len >= done_count);
            len -= done_count;
            src += done_count;
            _WRITER(vec) += done_count;
            Py_ssize_t process_escape_count = PYYJSON_ENCODE_ESCAPE_ONCE_BYTES / sizeof(_FROM_TYPE);
            process_escape_count = process_escape_count > len ? len : process_escape_count;
            vec = VECTOR_WRITE_ESCAPE_IMPL(vec_addr, src, process_escape_count, len - process_escape_count);
            RETURN_ON_UNLIKELY_ERR(!vec);
            len -= process_escape_count;
            src += process_escape_count;
        }
    }
    if (!len) goto done;
    vec = VECTOR_WRITE_UNICODE_TRAILING_IMPL(src, len, vec_addr);
    RETURN_ON_UNLIKELY_ERR(!vec);
done:;
    assert(vec_in_boundary(vec));
    return vec;
    // #undef SIMD_VAR
}

force_inline bool PYYJSON_CONCAT4(vec_write_key, COMPILE_INDENT_LEVEL, COMPILE_READ_UCS_LEVEL, COMPILE_WRITE_UCS_LEVEL)(PyObject *key, Py_ssize_t len, UnicodeVector **restrict vec_addr, Py_ssize_t cur_nested_depth) {
    static_assert(COMPILE_READ_UCS_LEVEL <= COMPILE_WRITE_UCS_LEVEL, "COMPILE_READ_UCS_LEVEL <= COMPILE_WRITE_UCS_LEVEL");
    UnicodeVector *vec = *vec_addr;
    assert(PyUnicode_GET_LENGTH(key) == len);
    vec = VEC_RESERVE(vec_addr, get_indent_char_count(cur_nested_depth, COMPILE_INDENT_LEVEL) + 4 + len + TAIL_PADDING);
    RETURN_ON_UNLIKELY_ERR(!vec);
    VECTOR_WRITE_INDENT(vec, cur_nested_depth);
    *_WRITER(vec)++ = '"';
    vec = VECTOR_WRITE_UNICODE_IMPL(vec_addr, (_FROM_TYPE *)get_unicode_data(key), len);
    RETURN_ON_UNLIKELY_ERR(!vec);
    vec = VEC_RESERVE(vec_addr, 3 + TAIL_PADDING);
    RETURN_ON_UNLIKELY_ERR(!vec);
    _TARGET_TYPE *writer = _WRITER(vec);
    *writer++ = '"';
    *writer++ = ':';
#if COMPILE_INDENT_LEVEL > 0
    *writer++ = ' ';
#    if SIZEOF_VOID_P == 8 || COMPILE_WRITE_UCS_LEVEL != 4
    *writer = 0;
#    endif // SIZEOF_VOID_P == 8 || COMPILE_WRITE_UCS_LEVEL != 4
#endif     // COMPILE_INDENT_LEVEL > 0
    _WRITER(vec) += (COMPILE_INDENT_LEVEL > 0) ? 3 : 2;
    assert(vec_in_boundary(vec));
    return true;
}

force_inline bool PYYJSON_CONCAT4(vec_write_str, COMPILE_INDENT_LEVEL, COMPILE_READ_UCS_LEVEL, COMPILE_WRITE_UCS_LEVEL)(PyObject *restrict str, Py_ssize_t len, UnicodeVector **restrict vec_addr, Py_ssize_t cur_nested_depth, bool is_in_obj) {
    static_assert(COMPILE_READ_UCS_LEVEL <= COMPILE_WRITE_UCS_LEVEL, "COMPILE_READ_UCS_LEVEL <= COMPILE_WRITE_UCS_LEVEL");
    assert(vec_addr);
    UnicodeVector *vec = *vec_addr;
    assert(PyUnicode_GET_LENGTH(str) == len);
    if (is_in_obj) {
        vec = VEC_RESERVE(vec_addr, 3 + len + TAIL_PADDING);
        RETURN_ON_UNLIKELY_ERR(!vec);
    } else {
        vec = VEC_RESERVE(vec_addr, get_indent_char_count(cur_nested_depth, COMPILE_INDENT_LEVEL) + 3 + len + TAIL_PADDING);
        RETURN_ON_UNLIKELY_ERR(!vec);
        VECTOR_WRITE_INDENT(vec, cur_nested_depth);
    }
    *_WRITER(vec)++ = '"';
    vec = VECTOR_WRITE_UNICODE_IMPL(vec_addr, (_FROM_TYPE *)get_unicode_data(str), len);
    RETURN_ON_UNLIKELY_ERR(!vec);
    vec = VEC_RESERVE(vec_addr, 2 + TAIL_PADDING);
    RETURN_ON_UNLIKELY_ERR(!vec);
    _TARGET_TYPE *writer = _WRITER(vec);
    *writer++ = '"';
    *writer++ = ',';
    _WRITER(vec) += 2;
    assert(vec_in_boundary(vec));
    return true;
}

#include "commondef/i_out.inl.h"
#include "commondef/r_out.inl.h"
#include "commondef/w_out.inl.h"

#undef MASK_ELEVATE_WRITE_512
#undef WRITE_SIMD_IMPL
#undef BACK_WRITE_SIMD256_WITH_TAIL_LEN
#undef WRITE_SIMD_256_WITH_WRITEMASK
#undef GET_DONE_COUNT_FROM_MASK
#undef CHECK_ESCAPE_IMPL_GET_MASK
#undef _CONTROL_SEQ_TABLE
#undef CHECK_ESCAPE_TAIL_IMPL_GET_MASK_512
#undef VECTOR_WRITE_ESCAPE_IMPL
#undef VECTOR_WRITE_UNICODE_TRAILING_IMPL
#undef VECTOR_WRITE_UNICODE_IMPL
#undef _TARGET_TYPE
#undef _WRITER
