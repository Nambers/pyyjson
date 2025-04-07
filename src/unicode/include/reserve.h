#include "unicode/uvector.h"

#undef VEC_RESERVE

/*
 * Reserve space for the vector.
 */
#define VEC_RESERVE PYYJSON_CONCAT2(vec_reserve, COMPILE_WRITE_UCS_LEVEL)

force_inline bool VEC_RESERVE(EncodeUnicodeBufferInfo *unicode_buffer_info, Py_ssize_t size);
