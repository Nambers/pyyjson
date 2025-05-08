#include "i_in.inl.h"
#include "w_in.inl.h"
/*
 * Write indents to unicode buffer. Need to reserve space before calling this function.
 */
#define write_unicode_indent PYYJSON_CONCAT3(write_unicode_indent, COMPILE_INDENT_LEVEL, COMPILE_WRITE_UCS_LEVEL)
// force_inline void write_unicode_indent(_dst_t **writer, Py_ssize_t _cur_nested_depth);

/*
 * Write indents to unicode buffer. Will reserve space if needed.
 */
#define unicode_indent_writer PYYJSON_CONCAT3(unicode_indent_writer, COMPILE_INDENT_LEVEL, COMPILE_WRITE_UCS_LEVEL)
// force_inline bool unicode_indent_writer(EncodeUnicodeBufferInfo *unicode_buffer_info, Py_ssize_t cur_nested_depth, bool is_in_obj, Py_ssize_t additional_reserve_count);
