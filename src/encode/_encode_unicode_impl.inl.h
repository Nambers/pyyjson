#ifdef PYYJSON_CLANGD_DUMMY
#    include "encode_shared.h"
#    include "simd/simd_detect.h"
#    include "simd/simd_impl.h"
#    include "unicode/indent_wrap.h"
#    ifndef COMPILE_READ_UCS_LEVEL
#        define COMPILE_READ_UCS_LEVEL 1
#    endif
#    ifndef COMPILE_WRITE_UCS_LEVEL
#        define COMPILE_WRITE_UCS_LEVEL 1
#    endif
#    ifndef COMPILE_INDENT_LEVEL
#        define COMPILE_INDENT_LEVEL 2
#    endif
#    ifndef COMPILE_SIMD_BITS
#        define COMPILE_SIMD_BITS 256
#    endif
#endif

#include "commondef/iw_in.inl.h"
#include "commondef/rw_in.inl.h"


/* Macro IN */
#include "commondef/iw_in.inl.h"
//
#include "commondef/w_out.inl.h"
//
#include "commondef/srw_in.inl.h"


#if COMPILE_READ_UCS_LEVEL > 1
static force_noinline
#else
force_inline
#endif
        bool
        PYYJSON_CONCAT4(unicode_buffer_append_key_internal, COMPILE_INDENT_LEVEL, COMPILE_READ_UCS_LEVEL, COMPILE_WRITE_UCS_LEVEL)(PyObject *key, Py_ssize_t len, EncodeUnicodeBufferInfo *unicode_buffer_info, Py_ssize_t cur_nested_depth) {
    static_assert(COMPILE_READ_UCS_LEVEL <= COMPILE_WRITE_UCS_LEVEL, "COMPILE_READ_UCS_LEVEL <= COMPILE_WRITE_UCS_LEVEL");
    assert(PyUnicode_GET_LENGTH(key) == len);
    RETURN_ON_UNLIKELY_ERR(!UNICODE_BUFFER_RESERVE(unicode_buffer_info, get_indent_char_count(cur_nested_depth, COMPILE_INDENT_LEVEL) + 5 + 6 * len + TAIL_PADDING));
    write_unicode_indent(&_WRITER(unicode_buffer_info), cur_nested_depth);
    *_WRITER(unicode_buffer_info)++ = '"';
    encode_unicode_impl(&_WRITER(unicode_buffer_info), (_src_t *)get_unicode_data(key), (usize)len);
    _dst_t *writer = _WRITER(unicode_buffer_info);
    *writer++ = '"';
    *writer++ = ':';
#if COMPILE_INDENT_LEVEL > 0
    *writer++ = ' ';
#    if SIZEOF_VOID_P == 8 || COMPILE_WRITE_UCS_LEVEL != 4
    *writer = 0;
#    endif // SIZEOF_VOID_P == 8 || COMPILE_WRITE_UCS_LEVEL != 4
#endif     // COMPILE_INDENT_LEVEL > 0
    _WRITER(unicode_buffer_info) += (COMPILE_INDENT_LEVEL > 0) ? 3 : 2;
    assert(check_unicode_writer_valid(unicode_buffer_info));
    return true;
}

#if COMPILE_READ_UCS_LEVEL > 1
static force_noinline
#else
force_inline
#endif
        bool
        PYYJSON_CONCAT4(unicode_buffer_append_str_internal, COMPILE_INDENT_LEVEL, COMPILE_READ_UCS_LEVEL, COMPILE_WRITE_UCS_LEVEL)(PyObject *restrict str, Py_ssize_t len, EncodeUnicodeBufferInfo *unicode_buffer_info, Py_ssize_t cur_nested_depth, bool is_in_obj) {
    static_assert(COMPILE_READ_UCS_LEVEL <= COMPILE_WRITE_UCS_LEVEL, "COMPILE_READ_UCS_LEVEL <= COMPILE_WRITE_UCS_LEVEL");
    assert(PyUnicode_GET_LENGTH(str) == len);
    if (is_in_obj) {
        RETURN_ON_UNLIKELY_ERR(!UNICODE_BUFFER_RESERVE(unicode_buffer_info, 3 + 6 * len + TAIL_PADDING));
    } else {
        RETURN_ON_UNLIKELY_ERR(!UNICODE_BUFFER_RESERVE(unicode_buffer_info, get_indent_char_count(cur_nested_depth, COMPILE_INDENT_LEVEL) + 3 + 6 * len + TAIL_PADDING));
        write_unicode_indent(&_WRITER(unicode_buffer_info), cur_nested_depth);
    }
    *_WRITER(unicode_buffer_info)++ = '"';
    encode_unicode_impl(&_WRITER(unicode_buffer_info), (_src_t *)get_unicode_data(str), (usize)len);
    _dst_t *writer = _WRITER(unicode_buffer_info);
    *writer++ = '"';
    *writer++ = ',';
    _WRITER(unicode_buffer_info) += 2;
    assert(check_unicode_writer_valid(unicode_buffer_info));
    return true;
}

#include "commondef/iw_out.inl.h"
#include "commondef/srw_out.inl.h"
