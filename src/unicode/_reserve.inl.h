#include "commondef/w_in.inl.h"
// #include "include/reserve.h"

force_inline bool VEC_RESERVE(EncodeUnicodeBufferInfo *unicode_buffer_info, Py_ssize_t size) {
    _TARGET_TYPE *target_ptr = _WRITER(unicode_buffer_info) + size;
    if (unlikely(target_ptr > PYYJSON_CAST(_TARGET_TYPE *, unicode_buffer_info->end))) {
        return unicode_vec_reserve(unicode_buffer_info, (void *)target_ptr);
    }
    return true;
}

#include "commondef/w_out.inl.h"
