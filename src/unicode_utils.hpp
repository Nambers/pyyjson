#ifndef MY_PYUNICODE_UTILS
#define MY_PYUNICODE_UTILS
#include "encode_shared.hpp"
#include "pyyjson_config.h"

template<UCSKind _Kind>
force_inline PyObject *
My_PyUnicode_New(Py_ssize_t size, Py_UCS4 maxchar) {
    constexpr Py_ssize_t add_size = 64 / sizeof(UCSType_t<_Kind>);
    if (unlikely(size > PY_SSIZE_T_MAX - add_size)) {
        return PyErr_NoMemory();
    }
    PyObject* ret = PyUnicode_New(size + add_size, maxchar);
    if(unlikely(!ret)) return NULL;
    _PyASCIIObject_CAST(ret)->length = size;
    return ret;
}

template<UCSKind _Kind>
force_inline usize unicode_real_u8_size(Py_ssize_t size, bool is_all_ascii) {
    assert(size > 0);
    Py_ssize_t struct_size = is_all_ascii ? sizeof(PyASCIIObject) : sizeof(PyCompactUnicodeObject);
    return struct_size + ((usize)size + 1) * sizeof(UCSType_t<_Kind>);
}

force_inline PyObject *
My_Realloc_Unicode(PyObject* unicode, usize u8_size) {
    void* new_addr = PyObject_Realloc((void*)unicode, u8_size);
    return (PyObject*)new_addr;
}

#endif
