#ifdef PYYJSON_CLANGD_DUMMY
#    ifndef COMPILE_CONTEXT_ENCODE
#        define COMPILE_CONTEXT_ENCODE
#    endif
#    ifndef COMPILE_INDENT_LEVEL
#        include "encode/indent_writer.h"
#        include "encode_shared.h"
#        include "simd/simd_detect.h"
#        include "simd/simd_impl.h"
#        include "unicode/unicode_buffer.h"
#        define COMPILE_INDENT_LEVEL 0
#        define COMPILE_READ_UCS_LEVEL 1
#        define COMPILE_WRITE_UCS_LEVEL 1
#        include "simd/compile_feature_check.h"
#    endif
#endif

/* Macro IN */
#include "compile_context/sirw_in.inl.h"

#if COMPILE_READ_UCS_LEVEL == 1
#    if PYYJSON_ENCODE_UCS1_IMPL_INLINE
#        define _IMPL_INLINE_SPECIFIER force_inline
#    else
#        define _IMPL_INLINE_SPECIFIER static force_noinline
#    endif
#elif COMPILE_READ_UCS_LEVEL == 2
#    if PYYJSON_ENCODE_UCS2_IMPL_INLINE
#        define _IMPL_INLINE_SPECIFIER force_inline
#    else
#        define _IMPL_INLINE_SPECIFIER static force_noinline
#    endif
#elif COMPILE_READ_UCS_LEVEL == 4
#    if PYYJSON_ENCODE_UCS4_IMPL_INLINE
#        define _IMPL_INLINE_SPECIFIER force_inline
#    else
#        define _IMPL_INLINE_SPECIFIER static force_noinline
#    endif
#endif

_IMPL_INLINE_SPECIFIER
bool unicode_buffer_append_key_internal(PyObject *key, Py_ssize_t len, _dst_t **writer_addr, EncodeUnicodeBufferInfo *unicode_buffer_info, Py_ssize_t cur_nested_depth) {
    static_assert(COMPILE_READ_UCS_LEVEL <= COMPILE_WRITE_UCS_LEVEL, "COMPILE_READ_UCS_LEVEL <= COMPILE_WRITE_UCS_LEVEL");
    assert(PyUnicode_GET_LENGTH(key) == len);
    RETURN_ON_UNLIKELY_ERR(!unicode_buffer_reserve(writer_addr, unicode_buffer_info, get_indent_char_count(cur_nested_depth, COMPILE_INDENT_LEVEL) + 5 + 6 * len + TAIL_PADDING));
    write_unicode_indent(writer_addr, cur_nested_depth);
    _dst_t *writer = *writer_addr;
    *writer++ = '"';
    encode_unicode_impl(&writer, (_src_t *)get_unicode_data(key), (usize)len);
    *writer++ = '"';
    *writer++ = ':';
#if COMPILE_INDENT_LEVEL > 0
    *writer++ = ' ';
#    if SIZEOF_VOID_P == 8 || COMPILE_WRITE_UCS_LEVEL != 4
    *writer = 0;
#    endif // SIZEOF_VOID_P == 8 || COMPILE_WRITE_UCS_LEVEL != 4
#endif     // COMPILE_INDENT_LEVEL > 0
    *writer_addr = writer;
    assert(check_unicode_writer_valid(writer, unicode_buffer_info));
    return true;
}

_IMPL_INLINE_SPECIFIER
bool unicode_buffer_append_str_internal(PyObject *str, Py_ssize_t len, _dst_t **writer_addr,
                                        EncodeUnicodeBufferInfo *unicode_buffer_info, Py_ssize_t cur_nested_depth, bool is_in_obj) {
    static_assert(COMPILE_READ_UCS_LEVEL <= COMPILE_WRITE_UCS_LEVEL, "COMPILE_READ_UCS_LEVEL <= COMPILE_WRITE_UCS_LEVEL");
    assert(PyUnicode_GET_LENGTH(str) == len);
    if (is_in_obj) {
        RETURN_ON_UNLIKELY_ERR(!unicode_buffer_reserve(writer_addr, unicode_buffer_info, 3 + 6 * len + TAIL_PADDING));
    } else {
        RETURN_ON_UNLIKELY_ERR(!unicode_buffer_reserve(writer_addr, unicode_buffer_info, get_indent_char_count(cur_nested_depth, COMPILE_INDENT_LEVEL) + 3 + 6 * len + TAIL_PADDING));
        write_unicode_indent(writer_addr, cur_nested_depth);
    }
    _dst_t *writer = *writer_addr;
    *writer++ = '"';
    encode_unicode_impl(&writer, (_src_t *)get_unicode_data(str), (usize)len);
    *writer++ = '"';
    *writer++ = ',';
    *writer_addr = writer;
    assert(check_unicode_writer_valid(writer, unicode_buffer_info));
    return true;
}

#undef _IMPL_INLINE_SPECIFIER

#include "compile_context/sirw_out.inl.h"
