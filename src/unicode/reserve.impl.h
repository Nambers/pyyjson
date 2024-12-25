#include "commondef/w_in.inl.h"
#include "include/reserve.h"

force_inline UnicodeVector *VEC_RESERVE(UnicodeVector **vec_addr, Py_ssize_t size) {
    assert(vec_addr);
    UnicodeVector *vec = *vec_addr;
    _TARGET_TYPE *target_ptr = _WRITER(vec) + size;
    if (unlikely(target_ptr > (_TARGET_TYPE *)VEC_END(vec))) {
        UnicodeVector *new_vec = unicode_vec_reserve(vec, (void *)target_ptr);
        vec = new_vec;
        if (new_vec) *vec_addr = new_vec;
    }
    return vec;
}

#include "commondef/w_out.inl.h"
