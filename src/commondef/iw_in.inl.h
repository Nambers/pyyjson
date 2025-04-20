#include "iw_out.inl.h"
//
#include "i_in.inl.h"
#include "w_in.inl.h"
/*
 * Write indents to unicode buffer. Need to reserve space before calling this function.
 */
#define WRITE_UNICODE_INDENT PYYJSON_CONCAT3(write_unicode_indent, COMPILE_INDENT_LEVEL, COMPILE_WRITE_UCS_LEVEL)
force_inline void WRITE_UNICODE_INDENT(_TARGET_TYPE **writer, Py_ssize_t _cur_nested_depth);

/*
 * Write indents to unicode buffer. Will reserve space if needed.
 */
#define UNICODE_INDENT_WRITER PYYJSON_CONCAT3(unicode_indent_writer, COMPILE_INDENT_LEVEL, COMPILE_WRITE_UCS_LEVEL)
force_inline bool UNICODE_INDENT_WRITER(EncodeUnicodeBufferInfo *unicode_buffer_info, Py_ssize_t cur_nested_depth, bool is_in_obj, Py_ssize_t additional_reserve_count);
