#include "commondef/w_in.inl.h"
#include "include/indent.h"
#include "include/reserve.h"

force_inline void VECTOR_WRITE_INDENT(UnicodeVector *restrict vec, Py_ssize_t _cur_nested_depth) {
#if COMPILE_INDENT_LEVEL > 0
    _TARGET_TYPE *writer = _WRITER(vec);
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
    _WRITER(vec) += COMPILE_INDENT_LEVEL * cur_nested_depth + 1;
#endif // COMPILE_INDENT_LEVEL > 0
}

force_inline UnicodeVector *INDENT_WRITER(UnicodeVector **vec_addr, Py_ssize_t cur_nested_depth, bool is_in_obj, Py_ssize_t additional_reserve_count) {
    UnicodeVector *vec;
    if (!is_in_obj && COMPILE_INDENT_LEVEL) {
        vec = VEC_RESERVE(vec_addr, get_indent_char_count(cur_nested_depth, COMPILE_INDENT_LEVEL) + additional_reserve_count);
        RETURN_ON_UNLIKELY_ERR(!vec);
        VECTOR_WRITE_INDENT(vec, cur_nested_depth);
    } else {
        vec = VEC_RESERVE(vec_addr, additional_reserve_count);
        RETURN_ON_UNLIKELY_ERR(!vec);
    }
    return vec;
}

#include "commondef/w_out.inl.h"
