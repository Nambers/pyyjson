#include "unicode/reserve_wrap.h"
#include "commondef/iw_in.inl.h"


force_inline void VECTOR_WRITE_INDENT(_TARGET_TYPE **writer_addr, Py_ssize_t _cur_nested_depth) {
#if COMPILE_INDENT_LEVEL > 0
    _TARGET_TYPE *writer = *writer_addr;
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

force_inline bool INDENT_WRITER(EncodeUnicodeBufferInfo *unicode_buffer_info, Py_ssize_t cur_nested_depth, bool is_in_obj, Py_ssize_t additional_reserve_count) {
    if (!is_in_obj && COMPILE_INDENT_LEVEL != 0) {
        RETURN_ON_UNLIKELY_ERR(!VEC_RESERVE(unicode_buffer_info, get_indent_char_count(cur_nested_depth, COMPILE_INDENT_LEVEL) + additional_reserve_count));
         VECTOR_WRITE_INDENT(&_WRITER(unicode_buffer_info), cur_nested_depth);
    } else {
        RETURN_ON_UNLIKELY_ERR(!VEC_RESERVE(unicode_buffer_info, additional_reserve_count));
    }
    return true;
}

#include "commondef/iw_out.inl.h"
