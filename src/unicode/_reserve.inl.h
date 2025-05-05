#include "commondef/w_in.inl.h"

// #include "include/reserve.h"

force_inline bool UNICODE_BUFFER_RESERVE(EncodeUnicodeBufferInfo *unicode_buffer_info, Py_ssize_t size) {
    _dst_t *target_ptr = _WRITER(unicode_buffer_info) + size;
    if (unlikely(target_ptr > PYYJSON_CAST(_dst_t *, unicode_buffer_info->end))) {
        u8 *old_head = (u8 *)unicode_buffer_info->head;
        return unicode_buffer_reserve(unicode_buffer_info, (void *)target_ptr);
    }
    return true;
}

#include "commondef/w_out.inl.h"
