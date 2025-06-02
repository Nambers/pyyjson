#ifdef PYYJSON_CLANGD_DUMMY
#    include "unicode/unicode.h"
#endif

#include "compile_context/w_in.inl.h"

force_inline bool unicode_buffer_reserve(_dst_t **writer_addr, EncodeUnicodeBufferInfo *unicode_buffer_info, usize size) {
    _dst_t *target_ptr = *writer_addr + size;
    if (unlikely(target_ptr > PYYJSON_CAST(_dst_t *, unicode_buffer_info->end))) {
        u8 *old_head = (u8 *)unicode_buffer_info->head;
        _dst_t *cur_writer = *writer_addr;
        usize target_size = PYYJSON_CAST(u8 *, target_ptr) - PYYJSON_CAST(u8 *, unicode_buffer_info->head);
        bool ret = _unicode_buffer_reserve(unicode_buffer_info, target_size);
        if (unlikely(!ret)) return false;
        usize u8offset = PYYJSON_CAST(u8 *, cur_writer) - old_head;
        *writer_addr = PYYJSON_CAST(_dst_t *, PYYJSON_CAST(u8 *, unicode_buffer_info->head) + u8offset);
    }
    return true;
}

#include "compile_context/w_out.inl.h"
