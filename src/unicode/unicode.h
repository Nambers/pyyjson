#ifndef PYYJSON_UNICODE_UNICODE_H
#define PYYJSON_UNICODE_UNICODE_H

#include "pyyjson.h"

#define PYUNICODE_ASCII_START(_obj_) PYYJSON_CAST(u8 *, PYYJSON_CAST(PyASCIIObject *, (_obj_)) + 1)
#define PYUNICODE_UCS1_START(_obj_) PYYJSON_CAST(u8 *, PYYJSON_CAST(PyCompactUnicodeObject *, (_obj_)) + 1)
#define PYUNICODE_UCS2_START(_obj_) PYYJSON_CAST(u16 *, PYYJSON_CAST(PyCompactUnicodeObject *, (_obj_)) + 1)
#define PYUNICODE_UCS4_START(_obj_) PYYJSON_CAST(u32 *, PYYJSON_CAST(PyCompactUnicodeObject *, (_obj_)) + 1)

force_noinline void init_pyunicode_noinline(void *, Py_ssize_t size, int kind);

#endif // PYYJSON_UNICODE_UNICODE_H
