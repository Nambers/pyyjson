#include "unicode/uvector.h"


#undef VECTOR_WRITE_INDENT
#undef INDENT_WRITER


/*
 * Write indents to unicode vector. Need to reserve space before calling this function.
 */
#define VECTOR_WRITE_INDENT PYYJSON_CONCAT3(vector_write_indent, COMPILE_INDENT_LEVEL, COMPILE_WRITE_UCS_LEVEL)


/*
 * Write indents to unicode vector. Will reserve space if needed.
 */
#define INDENT_WRITER PYYJSON_CONCAT3(indent_writer, COMPILE_INDENT_LEVEL, COMPILE_WRITE_UCS_LEVEL)


force_inline void VECTOR_WRITE_INDENT(UnicodeVector *restrict vec, Py_ssize_t _cur_nested_depth);
force_inline UnicodeVector *INDENT_WRITER(UnicodeVector **vec_addr, Py_ssize_t cur_nested_depth, bool is_in_obj, Py_ssize_t additional_reserve_count);
