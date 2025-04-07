#include "pyyjson.h"
#include "uvector.h"


#define VEC_MEM_U8_DIFF(_start_, _end_) (_Py_CAST(uintptr_t, (_end_)) - _Py_CAST(uintptr_t, (_start_)))


// _PyUnicode_CheckConsistency is hidden in Python 3.13
#if PY_MINOR_VERSION >= 13
extern int _PyUnicode_CheckConsistency(PyObject *op, int check_content);
#endif


force_inline void vec_set_rwptr_and_size(EncodeUnicodeBufferInfo *unicode_buffer_info, Py_ssize_t rw_diff, Py_ssize_t target_size) {
    unicode_buffer_info->writer.writer_u8 = _Py_CAST(u8 *, unicode_buffer_info->head) + rw_diff;
    unicode_buffer_info->end = _Py_CAST(u8 *, unicode_buffer_info->head) + target_size;
}

force_noinline bool unicode_vec_reserve(EncodeUnicodeBufferInfo *unicode_buffer_info, void *target_ptr) {
    const usize u8_diff = VEC_MEM_U8_DIFF(unicode_buffer_info->head, target_ptr);
    assert(u8_diff >= 0);
    usize target_size = VEC_MEM_U8_DIFF(unicode_buffer_info->head, unicode_buffer_info->end);
    assert(target_size >= 0);
#if PYYJSON_ASAN_CHECK
    // for sanitize=address build, only resize to the *just enough* size.
    usize inc_size = 0;
#else
    usize inc_size = target_size;
#endif
    if (unlikely(target_size > (PY_SSIZE_T_MAX - inc_size))) {
        PyErr_NoMemory();
        return false;
    }
    target_size = target_size + inc_size;
    target_size = (target_size > u8_diff) ? target_size : u8_diff;
    Py_ssize_t w_diff = VEC_MEM_U8_DIFF(unicode_buffer_info->head, unicode_buffer_info->writer.writer_u8);
    void *new_ptr = PyObject_Realloc(unicode_buffer_info->head, target_size);
    if (unlikely(!new_ptr)) {
        PyErr_NoMemory();
        return false;
    }
    unicode_buffer_info->head = new_ptr;
    vec_set_rwptr_and_size(unicode_buffer_info, w_diff, target_size);
#ifndef NDEBUG
    memset(unicode_buffer_info->writer.writer_u8, 0, _Py_CAST(u8 *, unicode_buffer_info->end) - unicode_buffer_info->writer.writer_u8);
#endif
    return true;
}

force_noinline void init_py_unicode(void *head, Py_ssize_t size, int kind) {
    PyCompactUnicodeObject *unicode = _Py_CAST(PyCompactUnicodeObject *, head);
    PyASCIIObject *ascii = _Py_CAST(PyASCIIObject *, head);
    PyObject_Init((PyObject *)unicode, &PyUnicode_Type);
    void *data = kind ? _Py_CAST(void *, unicode + 1) : _Py_CAST(void *, ascii + 1);
    //
    ascii->length = size;
    ascii->hash = -1;
    ascii->state.interned = 0;
    ascii->state.kind = kind ? kind : 1;
    ascii->state.compact = 1;
    ascii->state.ascii = kind ? 0 : 1;

#if PY_MINOR_VERSION >= 12
    // statically_allocated appears in 3.12
    ascii->state.statically_allocated = 0;
#else
    bool is_sharing = false;
    // ready is dropped in 3.12
    ascii->state.ready = 1;
#endif

    if (kind <= 1) {
        ((u8 *)data)[size] = 0;
    } else if (kind == 2) {
        ((u16 *)data)[size] = 0;
#if PY_MINOR_VERSION < 12
        is_sharing = sizeof(wchar_t) == 2;
#endif
    } else if (kind == 4) {
        ((u32 *)data)[size] = 0;
#if PY_MINOR_VERSION < 12
        is_sharing = sizeof(wchar_t) == 4;
#endif
    } else {
        assert(false);
        Py_UNREACHABLE();
    }
    if (kind) {
        unicode->utf8 = NULL;
        unicode->utf8_length = 0;
    }
#if PY_MINOR_VERSION < 12
    if (kind > 1) {
        if (is_sharing) {
            unicode->wstr_length = size;
            ascii->wstr = (wchar_t *)data;
        } else {
            unicode->wstr_length = 0;
            ascii->wstr = NULL;
        }
    } else {
        ascii->wstr = NULL;
        if (kind) unicode->wstr_length = 0;
    }
#endif
    assert(_PyUnicode_CheckConsistency((PyObject *)unicode, 0));
    assert(ascii->ob_base.ob_refcnt == 1);
}

force_noinline bool vector_resize_to_fit(EncodeUnicodeBufferInfo *unicode_buffer_info, Py_ssize_t len, int ucs_type) {
    Py_ssize_t char_size = ucs_type ? ucs_type : 1;
    Py_ssize_t struct_size = ucs_type ? sizeof(PyCompactUnicodeObject) : sizeof(PyASCIIObject);
    assert(len <= ((PY_SSIZE_T_MAX - struct_size) / char_size - 1));
    // Resizes to a smaller size. It *should* always be successful
    void *new_ptr = PyObject_Realloc(unicode_buffer_info->head, struct_size + (len + 1) * char_size);
    if (unlikely(!new_ptr)) {
        return false;
    } else {
        unicode_buffer_info->head = new_ptr;
    }
    return true;
}
