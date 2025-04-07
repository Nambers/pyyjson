#include "commondef/i_in.inl.h"
#include "commondef/rw_in.inl.h"
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
#define VECTOR_WRITE_UNICODE_LOOPx4 PYYJSON_CONCAT4(vector_write_unicode_loopx4, COMPILE_INDENT_LEVEL, COMPILE_READ_UCS_LEVEL, COMPILE_WRITE_UCS_LEVEL)
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

force_inline bool VECTOR_WRITE_ESCAPE_AUTO_RESERVE(EncodeUnicodeBufferInfo *unicode_buffer_info, const _FROM_TYPE *restrict src, Py_ssize_t len, Py_ssize_t additional_len) {
    const _FROM_TYPE *src_end = src + len;
    while (src < src_end) {
        _TARGET_TYPE srcval = (_TARGET_TYPE)*src;
        usize unicode_point = (usize)srcval;
        Py_ssize_t copy_count = (unicode_point <= _Slash) ? _ControlJump[unicode_point] : 0;
        if (likely(!copy_count)) {
            *_WRITER(unicode_buffer_info)++ = srcval;
        } else {
            _TARGET_TYPE *copy_ptr = &_CONTROL_SEQ_TABLE[unicode_point * 8];
            if (copy_count == 2) {
                RETURN_ON_UNLIKELY_ERR(!VEC_RESERVE(unicode_buffer_info, 2 + len + TAIL_PADDING / sizeof(_TARGET_TYPE) + additional_len));
                memcpy((void *)_WRITER(unicode_buffer_info), (const void *)copy_ptr, 2 * sizeof(_TARGET_TYPE));
                _WRITER(unicode_buffer_info) += 2;
            } else {
                assert(6 == copy_count);
#if COMPILE_WRITE_UCS_LEVEL < 4 || SIZEOF_VOID_P == 8
                const usize _CopyLen = 8;
#else //  COMPILE_WRITE_UCS_LEVEL == 4 && SIZEOF_VOID_P < 8
                const usize _CopyLen = 6;
#endif
                RETURN_ON_UNLIKELY_ERR(!VEC_RESERVE(unicode_buffer_info, _CopyLen + len + TAIL_PADDING / sizeof(_TARGET_TYPE) + additional_len));
                memcpy(_WRITER(unicode_buffer_info), (const void *)copy_ptr, _CopyLen * sizeof(_TARGET_TYPE));
                _WRITER(unicode_buffer_info) += 6;
            }
        }
        src++;
    }
    return true;
}

#if PYYJSON_X86


#    if SIMD_BIT_SIZE == 512


force_inline bool VECTOR_WRITE_UNICODE_TRAILING_IMPL(const _FROM_TYPE *src, Py_ssize_t len, EncodeUnicodeBufferInfo *unicode_buffer_info) {
#        define _MASKZ_LOADU PYYJSON_SIMPLE_CONCAT2(_mm512_maskz_loadu_epi, READ_BIT_SIZE)
#        define _MASK_STOREU PYYJSON_SIMPLE_CONCAT2(_mm512_mask_storeu_epi, READ_BIT_SIZE)
    SIMD_512 z;
    u64 rw_mask, tail_mask;
    rw_mask = ((u64)1 << (usize)len) - 1;
    z = _MASKZ_LOADU(rw_mask, (const void *)src);
    tail_mask = CHECK_ESCAPE_TAIL_IMPL_GET_MASK_512(z, rw_mask);
    if (likely(check_mask_zero(tail_mask))) {
#        if COMPILE_READ_UCS_LEVEL == COMPILE_WRITE_UCS_LEVEL
        _MASK_STOREU((void *)_WRITER(unicode_buffer_info), rw_mask, z);
#        else
        MASK_ELEVATE_WRITE_512(_WRITER(unicode_buffer_info), z, len);
#        endif
        _WRITER(unicode_buffer_info) += len;
    } else {
#        if COMPILE_READ_UCS_LEVEL == 1
        usize tzcnt = (usize)u64_tz_bits(tail_mask);
#        else
        usize tzcnt = (usize)u32_tz_bits((u32)tail_mask);
#        endif
        assert(tzcnt < (usize)len);
#        if COMPILE_READ_UCS_LEVEL == COMPILE_WRITE_UCS_LEVEL
        _MASK_STOREU((void *)_WRITER(unicode_buffer_info), ((u64)1 << tzcnt) - 1, z);
#        else
        if (tzcnt) MASK_ELEVATE_WRITE_512(_WRITER(unicode_buffer_info), z, tzcnt);
#        endif
        _WRITER(unicode_buffer_info) += tzcnt;
        RETURN_ON_UNLIKELY_ERR(!VECTOR_WRITE_ESCAPE_AUTO_RESERVE(unicode_buffer_info, src + tzcnt, len - tzcnt, 0));
    }
    return true;
#        undef _MASKZ_LOADU
#        undef _MASK_STOREU
}


#    elif SIMD_BIT_SIZE == 256


force_inline bool VECTOR_WRITE_UNICODE_TRAILING_IMPL(const _FROM_TYPE *src, Py_ssize_t len, EncodeUnicodeBufferInfo *unicode_buffer_info) {
    _VEC_A_ y;
    const _FROM_TYPE *load_start = src + len - READ_BATCH_COUNT;
    _TARGET_TYPE *store_start = _WRITER(unicode_buffer_info) + len - READ_BATCH_COUNT;
    __m256i mask, check_mask;
#        define MASK_READER PYYJSON_CONCAT2(read_tail_mask_table, READ_BIT_SIZE)
    mask = load_256_aligned(MASK_READER(READ_BATCH_COUNT - len));
#        undef MASK_READER
    check_mask = CHECK_ESCAPE_IMPL_GET_MASK(load_start, &y);
    check_mask = simd_and_256(check_mask, mask);
    if (likely(check_mask_zero(check_mask))) {
#        if COMPILE_READ_UCS_LEVEL == COMPILE_WRITE_UCS_LEVEL
        assert((Py_ssize_t)store_start >= (Py_ssize_t)unicode_buffer_info->head);
        WRITE_SIMD_256_WITH_WRITEMASK(store_start, y, mask);
#        else
        BACK_WRITE_SIMD256_WITH_TAIL_LEN(store_start, y, len, unicode_buffer_info->head);
#        endif
        _WRITER(unicode_buffer_info) += len;
    } else {
        RETURN_ON_UNLIKELY_ERR(!VECTOR_WRITE_ESCAPE_AUTO_RESERVE(unicode_buffer_info, src, len, 0));
    }
    return true;
}


#    else
// SIMD_BIT_SIZE == 128, x86

force_inline bool VECTOR_WRITE_UNICODE_TRAILING_IMPL(const _FROM_TYPE *src, Py_ssize_t len, EncodeUnicodeBufferInfo *unicode_buffer_info) {
    assert(len < READ_BATCH_COUNT);
    _VEC_A_ x, mask, check_mask;
    const _FROM_TYPE *load_start = src + len - READ_BATCH_COUNT;
    _TARGET_TYPE *store_start = _WRITER(unicode_buffer_info) + len - READ_BATCH_COUNT;
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
        write_128(_WRITER(unicode_buffer_info), x);
#            endif // __SSE4_1__
#        else      // COMPILE_READ_UCS_LEVEL != COMPILE_WRITE_UCS_LEVEL
        x = runtime_right_shift_128bits(x, COMPILE_READ_UCS_LEVEL * (int)(READ_BATCH_COUNT - len));
        WRITE_SIMD_IMPL(_WRITER(unicode_buffer_info), x);
#        endif     // COMPILE_READ_UCS_LEVEL == COMPILE_WRITE_UCS_LEVEL
        _WRITER(unicode_buffer_info) += len;
    } else {
        RETURN_ON_UNLIKELY_ERR(!VECTOR_WRITE_ESCAPE_AUTO_RESERVE(unicode_buffer_info, src, len, 0));
    }
    return true;
}


#    endif

#elif PYYJSON_AARCH

#endif


force_inline bool VECTOR_WRITE_UNICODE_LOOPx4(EncodeUnicodeBufferInfo *unicode_buffer_info, const _FROM_TYPE **src_addr, usize *len_addr) {
#if WR_DIV == 1
#    define SRC_T _VECx4_A_
#    define SRC_T_U _VECx4_U_
#    define CHECKER CHECK_MASK_AND_GET_DONE_COUNTx4
#elif WR_DIV == 2
#    define SRC_T _VECx2_A_
#    define SRC_T_U _VECx2_U_
#    define CHECKER CHECK_MASK_AND_GET_DONE_COUNTx2
#else
#    define SRC_T _VEC_A_
#    define SRC_T_U _VEC_U_
#    define CHECKER CHECK_MASK_AND_GET_DONE_COUNT
#endif
    register SRC_T read_vec;
    read_vec = *(const SRC_T_U *)(*src_addr);

    {
#if WR_DIV == 1
        static_assert(sizeof(_WVECx4_A_) == sizeof(read_vec), "");
        *(_WVECx4_U_ *)_WRITER(unicode_buffer_info) = read_vec;
#else
        register _WVECx4_A_ write_vec = VECTOR_ELEVATE4(read_vec);
        static_assert(sizeof(read_vec) / sizeof(_FROM_TYPE) == WRITE_BATCH_COUNT * 4, "");
        static_assert(sizeof(_WVECx4_A_) / sizeof(_TARGET_TYPE) == WRITE_BATCH_COUNT * 4, "");
        *(_WVECx4_U_ *)_WRITER(unicode_buffer_info) = write_vec;
#endif
    }
    bool checked;
    usize done_count;
    CHECKER(read_vec, &checked, &done_count);
    if (likely(checked)) {
        *src_addr += WRITE_BATCH_COUNT * 4;
        _WRITER(unicode_buffer_info) += WRITE_BATCH_COUNT * 4;
        *len_addr -= WRITE_BATCH_COUNT * 4;
    } else {
        const _FROM_TYPE *escape_pos = (*src_addr) + done_count;
        *src_addr += done_count + 1;
        _FROM_TYPE escape_unicode = *escape_pos;
        assert(escape_unicode == _Quote || escape_unicode == _Slash || escape_unicode < ControlMax);
        _WRITER(unicode_buffer_info) += done_count;
        *len_addr -= done_count + 1;
        RETURN_ON_UNLIKELY_ERR(!VEC_RESERVE(unicode_buffer_info, 8 + (*len_addr) + TAIL_PADDING / sizeof(_TARGET_TYPE)));
        memcpy(_WRITER(unicode_buffer_info), &_CONTROL_SEQ_TABLE[escape_unicode * 8], 8 * sizeof(_TARGET_TYPE));
        _WRITER(unicode_buffer_info) += _ControlJump[escape_unicode];
    }

    return true;
#undef CHECKER
#undef SRC_T_U
#undef SRC_T
}

force_inline bool VECTOR_WRITE_UNICODE_LOOP(EncodeUnicodeBufferInfo *unicode_buffer_info, const _FROM_TYPE **src_addr, usize *len_addr) {
    _VEC_A_ SIMD_VAR;
    SIMD_MASK_TYPE mask;
    mask = CHECK_ESCAPE_IMPL_GET_MASK(*src_addr, &SIMD_VAR);
    WRITE_SIMD_IMPL(_WRITER(unicode_buffer_info), SIMD_VAR);
    if (likely(check_mask_zero(mask))) {
        *src_addr += READ_BATCH_COUNT;
        _WRITER(unicode_buffer_info) += READ_BATCH_COUNT;
        *len_addr -= READ_BATCH_COUNT;
    } else {
        u32 done_count = GET_DONE_COUNT_FROM_MASK(mask);
        const _FROM_TYPE *escape_pos = (*src_addr) + done_count;
        *src_addr += done_count + 1;
        _FROM_TYPE escape_unicode = *escape_pos;
        assert(escape_unicode == _Quote || escape_unicode == _Slash || escape_unicode < ControlMax);
        _WRITER(unicode_buffer_info) += done_count;
        *len_addr -= done_count + 1;
        RETURN_ON_UNLIKELY_ERR(!VEC_RESERVE(unicode_buffer_info, 8 + (*len_addr) + TAIL_PADDING / sizeof(_TARGET_TYPE)));
        memcpy(_WRITER(unicode_buffer_info), &_CONTROL_SEQ_TABLE[escape_unicode * 8], 8 * sizeof(_TARGET_TYPE));
        _WRITER(unicode_buffer_info) += _ControlJump[escape_unicode];
    }
    return true;
}

force_inline bool VECTOR_WRITE_UNICODE_IMPL(EncodeUnicodeBufferInfo *unicode_buffer_info, const _FROM_TYPE *src, Py_ssize_t _len) {
    usize len = (usize)_len;
    bool _c;
    while (len >= WRITE_BATCH_COUNT * 4) {
        RETURN_ON_UNLIKELY_ERR(!VECTOR_WRITE_UNICODE_LOOPx4(unicode_buffer_info, &src, &len));
    }
    while (len >= READ_BATCH_COUNT) {
        RETURN_ON_UNLIKELY_ERR(!VECTOR_WRITE_UNICODE_LOOP(unicode_buffer_info, &src, &len));
    }
    if (!len) goto done;
    RETURN_ON_UNLIKELY_ERR(!VECTOR_WRITE_UNICODE_TRAILING_IMPL(src, len, unicode_buffer_info));
done:;
    assert(vec_in_boundary(unicode_buffer_info));
    return true;
}

force_inline bool PYYJSON_CONCAT4(vec_write_key, COMPILE_INDENT_LEVEL, COMPILE_READ_UCS_LEVEL, COMPILE_WRITE_UCS_LEVEL)(PyObject *key, Py_ssize_t len, EncodeUnicodeBufferInfo *unicode_buffer_info, Py_ssize_t cur_nested_depth) {
    static_assert(COMPILE_READ_UCS_LEVEL <= COMPILE_WRITE_UCS_LEVEL, "COMPILE_READ_UCS_LEVEL <= COMPILE_WRITE_UCS_LEVEL");
    assert(PyUnicode_GET_LENGTH(key) == len);
    RETURN_ON_UNLIKELY_ERR(!VEC_RESERVE(unicode_buffer_info, get_indent_char_count(cur_nested_depth, COMPILE_INDENT_LEVEL) + 4 + len + TAIL_PADDING));
    VECTOR_WRITE_INDENT(&_WRITER(unicode_buffer_info), cur_nested_depth);
    *_WRITER(unicode_buffer_info)++ = '"';
    RETURN_ON_UNLIKELY_ERR(!VECTOR_WRITE_UNICODE_IMPL(unicode_buffer_info, (_FROM_TYPE *)get_unicode_data(key), len));
    RETURN_ON_UNLIKELY_ERR(!VEC_RESERVE(unicode_buffer_info, 3 + TAIL_PADDING));
    _TARGET_TYPE *writer = _WRITER(unicode_buffer_info);
    *writer++ = '"';
    *writer++ = ':';
#if COMPILE_INDENT_LEVEL > 0
    *writer++ = ' ';
#    if SIZEOF_VOID_P == 8 || COMPILE_WRITE_UCS_LEVEL != 4
    *writer = 0;
#    endif // SIZEOF_VOID_P == 8 || COMPILE_WRITE_UCS_LEVEL != 4
#endif     // COMPILE_INDENT_LEVEL > 0
    _WRITER(unicode_buffer_info) += (COMPILE_INDENT_LEVEL > 0) ? 3 : 2;
    assert(vec_in_boundary(unicode_buffer_info));
    return true;
}

force_inline bool PYYJSON_CONCAT4(vec_write_str, COMPILE_INDENT_LEVEL, COMPILE_READ_UCS_LEVEL, COMPILE_WRITE_UCS_LEVEL)(PyObject *restrict str, Py_ssize_t len, EncodeUnicodeBufferInfo *unicode_buffer_info, Py_ssize_t cur_nested_depth, bool is_in_obj) {
    static_assert(COMPILE_READ_UCS_LEVEL <= COMPILE_WRITE_UCS_LEVEL, "COMPILE_READ_UCS_LEVEL <= COMPILE_WRITE_UCS_LEVEL");
    assert(PyUnicode_GET_LENGTH(str) == len);
    if (is_in_obj) {
        RETURN_ON_UNLIKELY_ERR(!VEC_RESERVE(unicode_buffer_info, 3 + len + TAIL_PADDING));
    } else {
        RETURN_ON_UNLIKELY_ERR(!VEC_RESERVE(unicode_buffer_info, get_indent_char_count(cur_nested_depth, COMPILE_INDENT_LEVEL) + 3 + len + TAIL_PADDING));
        VECTOR_WRITE_INDENT(&_WRITER(unicode_buffer_info), cur_nested_depth);
    }
    *_WRITER(unicode_buffer_info)++ = '"';
    RETURN_ON_UNLIKELY_ERR(!VECTOR_WRITE_UNICODE_IMPL(unicode_buffer_info, (_FROM_TYPE *)get_unicode_data(str), len));
    RETURN_ON_UNLIKELY_ERR(!VEC_RESERVE(unicode_buffer_info, 2 + TAIL_PADDING));
    _TARGET_TYPE *writer = _WRITER(unicode_buffer_info);
    *writer++ = '"';
    *writer++ = ',';
    _WRITER(unicode_buffer_info) += 2;
    assert(vec_in_boundary(unicode_buffer_info));
    return true;
}

#include "commondef/i_out.inl.h"
#include "commondef/rw_out.inl.h"

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
#undef VECTOR_WRITE_UNICODE_LOOPx4
#undef VECTOR_WRITE_UNICODE_IMPL
#undef _TARGET_TYPE
#undef _WRITER
