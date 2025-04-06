#include "commondef/i_in.inl.h"
#include "commondef/r_in.inl.h"
#include "commondef/w_in.inl.h"
#include "simd/simd_detect.h"
#include "simd/simd_impl.h"
#include "unicode/include/indent.h"
#include "unicode/include/reserve.h"

// encode_simd_utils.inl
#define GET_DONE_COUNT_FROM_MASK PYYJSON_CONCAT2(get_done_count_from_mask, COMPILE_READ_UCS_LEVEL)
#define WRITE_SIMD_256_WITH_WRITEMASK PYYJSON_CONCAT2(write_simd_256_with_writemask, COMPILE_WRITE_UCS_LEVEL)
#define BACK_WRITE_SIMD256_WITH_TAIL_LEN PYYJSON_CONCAT3(back_write_simd256_with_tail_len, COMPILE_READ_UCS_LEVEL, COMPILE_WRITE_UCS_LEVEL)
#define _CONTROL_SEQ_TABLE PYYJSON_CONCAT2(_ControlSeqTable, COMPILE_WRITE_UCS_LEVEL)
#define VECTOR_WRITE_UNICODE_IMPL PYYJSON_CONCAT4(vector_write_unicode_impl, COMPILE_INDENT_LEVEL, COMPILE_READ_UCS_LEVEL, COMPILE_WRITE_UCS_LEVEL)
#define VECTOR_WRITE_UNICODE_LOOP PYYJSON_CONCAT4(vector_write_unicode_loop, COMPILE_INDENT_LEVEL, COMPILE_READ_UCS_LEVEL, COMPILE_WRITE_UCS_LEVEL)
#define VECTOR_WRITE_UNICODE_TRAILING_IMPL PYYJSON_CONCAT4(vector_write_unicode_trailing_impl, COMPILE_INDENT_LEVEL, COMPILE_READ_UCS_LEVEL, COMPILE_WRITE_UCS_LEVEL)
#define VECTOR_WRITE_ESCAPE_AUTO_RESERVE PYYJSON_CONCAT4(vector_write_escape_auto_reserve, COMPILE_INDENT_LEVEL, COMPILE_READ_UCS_LEVEL, COMPILE_WRITE_UCS_LEVEL)
#define VECTOR_WRITE_ESCAPE_NO_RESERVE PYYJSON_CONCAT4(vector_write_escape_no_reserve, COMPILE_INDENT_LEVEL, COMPILE_READ_UCS_LEVEL, COMPILE_WRITE_UCS_LEVEL)
#define CHECK_ESCAPE_TAIL_IMPL_GET_MASK_512 PYYJSON_CONCAT2(check_escape_tail_impl_get_mask_512, COMPILE_READ_UCS_LEVEL)
#define WRITE_SIMD_IMPL PYYJSON_CONCAT3(write_simd_impl, COMPILE_READ_UCS_LEVEL, COMPILE_WRITE_UCS_LEVEL)
#define MASK_ELEVATE_WRITE_512 PYYJSON_CONCAT3(mask_elevate_write_512, COMPILE_READ_UCS_LEVEL, COMPILE_WRITE_UCS_LEVEL)

// forward declaration
force_inline u32 GET_DONE_COUNT_FROM_MASK(SIMD_MASK_TYPE mask);
#if SIMD_BIT_SIZE == 512
force_inline SIMD_MASK_TYPE CHECK_ESCAPE_TAIL_IMPL_GET_MASK_512(SIMD_512 z, SIMD_MASK_TYPE rw_mask);
#endif
force_inline void WRITE_SIMD_IMPL(_TARGET_TYPE *dst, SIMD_TYPE SIMD_VAR);


extern _TARGET_TYPE _CONTROL_SEQ_TABLE[(_Slash + 1) * 8];

force_inline UnicodeVector *VECTOR_WRITE_ESCAPE_AUTO_RESERVE(UnicodeVector **restrict vec_addr, const _FROM_TYPE *restrict src, Py_ssize_t len, Py_ssize_t additional_len) {
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

force_inline UnicodeVector *VECTOR_WRITE_ESCAPE_NO_RESERVE(UnicodeVector **restrict vec_addr, const _FROM_TYPE *restrict src, Py_ssize_t len, Py_ssize_t additional_len) {
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
                memcpy((void *)writer, (const void *)copy_ptr, 2 * sizeof(_TARGET_TYPE));
                writer += 2;
            } else {
                assert(6 == copy_count);
#if COMPILE_WRITE_UCS_LEVEL < 4 || SIZEOF_VOID_P == 8
                const usize _CopyLen = 8;
#else //  COMPILE_WRITE_UCS_LEVEL == 4 && SIZEOF_VOID_P < 8
                const usize _CopyLen = 6;
#endif
                memcpy((void *)writer, (const void *)copy_ptr, _CopyLen * sizeof(_TARGET_TYPE));
                writer += 6;
            }
        }
        src++;
    }
    _WRITER(vec) = writer;
    return vec;
}

#if PYYJSON_X86


#    if SIMD_BIT_SIZE == 512


force_inline UnicodeVector *VECTOR_WRITE_UNICODE_TRAILING_IMPL(const _FROM_TYPE *src, Py_ssize_t len, UnicodeVector **vec_addr) {
#        define _MASKZ_LOADU PYYJSON_SIMPLE_CONCAT2(_mm512_maskz_loadu_epi, READ_BIT_SIZE)
#        define _MASK_STOREU PYYJSON_SIMPLE_CONCAT2(_mm512_mask_storeu_epi, READ_BIT_SIZE)
    assert(vec_addr);
    UnicodeVector *vec = *vec_addr;
    SIMD_512 z;
    u64 rw_mask, tail_mask;
    rw_mask = ((u64)1 << (usize)len) - 1;
    z = _MASKZ_LOADU(rw_mask, (const void *)src);
    tail_mask = CHECK_ESCAPE_TAIL_IMPL_GET_MASK_512(z, rw_mask);
    if (likely(check_mask_zero(tail_mask))) {
#        if COMPILE_READ_UCS_LEVEL == COMPILE_WRITE_UCS_LEVEL
        _MASK_STOREU((void *)_WRITER(vec), rw_mask, z);
#        else
        MASK_ELEVATE_WRITE_512(_WRITER(vec), z, len);
#        endif
        _WRITER(vec) += len;
    } else {
#        if COMPILE_READ_UCS_LEVEL == 1
        usize tzcnt = (usize)u64_tz_bits(tail_mask);
#        else
        usize tzcnt = (usize)u32_tz_bits((u32)tail_mask);
#        endif
        assert(tzcnt < (usize)len);
#        if COMPILE_READ_UCS_LEVEL == COMPILE_WRITE_UCS_LEVEL
        _MASK_STOREU((void *)_WRITER(vec), ((u64)1 << tzcnt) - 1, z);
#        else
        if (tzcnt) MASK_ELEVATE_WRITE_512(_WRITER(vec), z, tzcnt);
#        endif
        _WRITER(vec) += tzcnt;
        vec = VECTOR_WRITE_ESCAPE_AUTO_RESERVE(vec_addr, src + tzcnt, len - tzcnt, 0);
        RETURN_ON_UNLIKELY_ERR(!vec);
    }
    return vec;
#        undef _MASKZ_LOADU
#        undef _MASK_STOREU
}


#    elif SIMD_BIT_SIZE == 256


force_inline UnicodeVector *VECTOR_WRITE_UNICODE_TRAILING_IMPL(const _FROM_TYPE *src, Py_ssize_t len, UnicodeVector **vec_addr) {
    assert(vec_addr);
    UnicodeVector *vec = *vec_addr;
    VECTOR_TYPE y;
    const _FROM_TYPE *load_start = src + len - READ_BATCH_COUNT;
    _TARGET_TYPE *store_start = _WRITER(vec) + len - READ_BATCH_COUNT;
    __m256i mask, check_mask;
#        define MASK_READER PYYJSON_CONCAT2(read_tail_mask_table, READ_BIT_SIZE)
    mask = load_256_aligned(MASK_READER(READ_BATCH_COUNT - len));
#        undef MASK_READER
    check_mask = CHECK_ESCAPE_IMPL_GET_MASK(load_start, &y);
    check_mask = simd_and_256(check_mask, mask);
    if (likely(check_mask_zero(check_mask))) {
#        if COMPILE_READ_UCS_LEVEL == COMPILE_WRITE_UCS_LEVEL
        assert((Py_ssize_t)store_start >= (Py_ssize_t)vec);
        WRITE_SIMD_256_WITH_WRITEMASK(store_start, y, mask);
#        else
        BACK_WRITE_SIMD256_WITH_TAIL_LEN(store_start, y, len, vec);
#        endif
        _WRITER(vec) += len;
    } else {
        vec = VECTOR_WRITE_ESCAPE_AUTO_RESERVE(vec_addr, src, len, 0);
        RETURN_ON_UNLIKELY_ERR(!vec);
    }
    return vec;
}


#    else
// SIMD_BIT_SIZE == 128, x86

force_inline UnicodeVector *VECTOR_WRITE_UNICODE_TRAILING_IMPL(const _FROM_TYPE *src, Py_ssize_t len, UnicodeVector **vec_addr) {
    assert(vec_addr);
    UnicodeVector *vec = *vec_addr;
    // TODO
    assert(len < READ_BATCH_COUNT);
    VECTOR_TYPE x, mask, check_mask;
    const _FROM_TYPE *load_start = src + len - READ_BATCH_COUNT;
    _TARGET_TYPE *store_start = _WRITER(vec) + len - READ_BATCH_COUNT;
#        define MASK_TABLE_READER PYYJSON_CONCAT2(read_tail_mask_table, READ_BIT_SIZE)
    mask = load_128_aligned(MASK_TABLE_READER(READ_BATCH_COUNT - len));
#        undef MASK_TABLE_READER
    check_mask = CHECK_ESCAPE_IMPL_GET_MASK(load_start, &x);
    check_mask = simd_and_128(check_mask, mask);
    if (likely(check_mask_zero(check_mask))) {
#        if COMPILE_READ_UCS_LEVEL == COMPILE_WRITE_UCS_LEVEL
#            if __SSE4_1__
        x = blendv_128(load_128((const void *)store_start), x, mask);
        write_128((void *)store_start, x);
#            else  // < __SSE4_1__
        x = runtime_right_shift_128bits(x, COMPILE_READ_UCS_LEVEL * (int)(READ_BATCH_COUNT - len));
        write_128(_WRITER(vec), x);
#            endif // __SSE4_1__
#        else      // COMPILE_READ_UCS_LEVEL != COMPILE_WRITE_UCS_LEVEL
        x = runtime_right_shift_128bits(x, COMPILE_READ_UCS_LEVEL * (int)(READ_BATCH_COUNT - len));
        WRITE_SIMD_IMPL(_WRITER(vec), x);
#        endif     // COMPILE_READ_UCS_LEVEL == COMPILE_WRITE_UCS_LEVEL
        _WRITER(vec) += len;
    } else {
        vec = VECTOR_WRITE_ESCAPE_AUTO_RESERVE(vec_addr, src, len, 0);
        RETURN_ON_UNLIKELY_ERR(!vec);
    }
    return vec;
}


#    endif

#elif PYYJSON_AARCH

#endif

force_inline UnicodeVector *VECTOR_WRITE_UNICODE_LOOP(UnicodeVector **restrict vec_addr, const _FROM_TYPE **src_addr, usize *len_addr) {
    VECTOR_TYPE SIMD_VAR;
    SIMD_MASK_TYPE mask;
    UnicodeVector *vec = *vec_addr;
    const _FROM_TYPE *src = *src_addr;
    mask = CHECK_ESCAPE_IMPL_GET_MASK(src, &SIMD_VAR);
    WRITE_SIMD_IMPL(_WRITER(vec), SIMD_VAR);
    if (likely(check_mask_zero(mask))) {
        src += READ_BATCH_COUNT;
        _WRITER(vec) += READ_BATCH_COUNT;
        *len_addr -= READ_BATCH_COUNT;
    } else {
        u32 done_count = GET_DONE_COUNT_FROM_MASK(mask);
        assert(*len_addr >= done_count);
        *len_addr -= done_count;
        src += done_count;
        _WRITER(vec) += done_count;
        Py_ssize_t process_escape_count = PYYJSON_ENCODE_ESCAPE_ONCE_BYTES / sizeof(_FROM_TYPE);
        process_escape_count = process_escape_count > *len_addr ? *len_addr : process_escape_count;
        vec = VECTOR_WRITE_ESCAPE_AUTO_RESERVE(vec_addr, src, process_escape_count, *len_addr - process_escape_count);
        RETURN_ON_UNLIKELY_ERR(!vec);
        *len_addr -= process_escape_count;
        src += process_escape_count;
    }
    *src_addr = src;
    return vec;
}

force_inline UnicodeVector *VECTOR_WRITE_UNICODE_IMPL(UnicodeVector **restrict vec_addr, const _FROM_TYPE *src, Py_ssize_t _len) {
    UnicodeVector *vec = *vec_addr;
    usize len = (usize)_len;
    bool _c;
    while (len >= READ_BATCH_COUNT) {
        vec = VECTOR_WRITE_UNICODE_LOOP(vec_addr, &src, &len);
        RETURN_ON_UNLIKELY_ERR(!vec);
    }
    if (!len) goto done;
    vec = VECTOR_WRITE_UNICODE_TRAILING_IMPL(src, len, vec_addr);
    RETURN_ON_UNLIKELY_ERR(!vec);
done:;
    assert(vec_in_boundary(vec));
    return vec;
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
#undef _CONTROL_SEQ_TABLE
#undef CHECK_ESCAPE_TAIL_IMPL_GET_MASK_512
#undef VECTOR_WRITE_ESCAPE_NO_RESERVE
#undef VECTOR_WRITE_ESCAPE_AUTO_RESERVE
#undef VECTOR_WRITE_UNICODE_TRAILING_IMPL
#undef VECTOR_WRITE_UNICODE_LOOP
#undef VECTOR_WRITE_UNICODE_IMPL
#undef _TARGET_TYPE
#undef _WRITER
