#include "commondef/iw_in.inl.h"
#define unicode_indent_writer PYYJSON_CONCAT3(unicode_indent_writer, COMPILE_INDENT_LEVEL, COMPILE_WRITE_UCS_LEVEL)
#define write_unicode_indent PYYJSON_CONCAT3(write_unicode_indent, COMPILE_INDENT_LEVEL, COMPILE_WRITE_UCS_LEVEL)

force_inline void write_unicode_indent(_dst_t **writer_addr, Py_ssize_t _cur_nested_depth) {
#if COMPILE_INDENT_LEVEL > 0
    _dst_t *writer = *writer_addr;
    *writer++ = '\n';
    usize cur_nested_depth = (usize)_cur_nested_depth;
    for (usize i = 0; i < cur_nested_depth; i++) {
        *writer++ = ' ';
        *writer++ = ' ';
#    if COMPILE_INDENT_LEVEL == 4
        *writer++ = ' ';
        *writer++ = ' ';
#    endif // COMPILE_INDENT_LEVEL == 4
    }
    *writer_addr = writer;
#endif // COMPILE_INDENT_LEVEL > 0
}

force_inline bool unicode_indent_writer(EncodeUnicodeBufferInfo *unicode_buffer_info, Py_ssize_t cur_nested_depth, bool is_in_obj, Py_ssize_t additional_reserve_count) {
    if (!is_in_obj && COMPILE_INDENT_LEVEL != 0) {
        RETURN_ON_UNLIKELY_ERR(!UNICODE_BUFFER_RESERVE(unicode_buffer_info, get_indent_char_count(cur_nested_depth, COMPILE_INDENT_LEVEL) + additional_reserve_count));
        write_unicode_indent(&_WRITER(unicode_buffer_info), cur_nested_depth);
    } else {
        RETURN_ON_UNLIKELY_ERR(!UNICODE_BUFFER_RESERVE(unicode_buffer_info, additional_reserve_count));
    }
    return true;
}

#undef write_unicode_indent
#undef unicode_indent_writer
#include "commondef/iw_out.inl.h"
