#include "pyyjson.h"
#include "uvector.h"


#define VEC_MEM_U8_DIFF(vec, ptr) ((Py_ssize_t) ptr - (Py_ssize_t) vec)


force_inline void vec_set_rwptr_and_size(UnicodeVector *vec, Py_ssize_t rw_diff, Py_ssize_t target_size) {
    vec->head.write_u8 = (u8 *) (((Py_ssize_t) vec) + rw_diff);
    vec->head.write_end = (void *) (((Py_ssize_t) vec) + target_size);
}


force_noinline UnicodeVector *unicode_vec_reserve(UnicodeVector *vec, void *target_ptr) {
    const Py_ssize_t u8_diff = VEC_MEM_U8_DIFF(vec, target_ptr);
    assert(u8_diff >= 0);
    // Py_ssize_t target_size = u8_diff;
    Py_ssize_t target_size = VEC_MEM_U8_DIFF(vec, VEC_END(vec));
    assert(target_size >= 0);
#if PYYJSON_ASAN_CHECK
    // for sanitize=address build, only resize to the *just enough* size.
    Py_ssize_t inc_size = 0;
#else
    Py_ssize_t inc_size = target_size;
#endif
    if (unlikely(target_size > (PY_SSIZE_T_MAX - inc_size))) {
        PyErr_NoMemory();
        return NULL;
    }
    target_size = target_size + inc_size;
    target_size = (target_size > u8_diff) ? target_size : u8_diff;
    Py_ssize_t w_diff = VEC_MEM_U8_DIFF(vec, vec->head.write_u8);
    void *new_ptr = PyObject_Realloc((void *) vec, (size_t) target_size);
    if (unlikely(!new_ptr)) {
        PyErr_NoMemory();
        return NULL;
    }
    vec = (UnicodeVector *) new_ptr;
    vec_set_rwptr_and_size(vec, w_diff, target_size);
#ifndef NDEBUG
    memset((void *) vec->head.write_u8, 0, (usize) ((u8 *) VEC_END(vec) - vec->head.write_u8));
#endif
    return vec;
}
