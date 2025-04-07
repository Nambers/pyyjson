
#ifndef UVECTOR_H
#define UVECTOR_H


#include "pyyjson.h"

typedef struct EncodeUnicodeBufferInfo {
    union {
        u8 *writer_u8;
        u16 *writer_u16;
        u32 *writer_u32;
        void *writer_void;
    } writer;

    void *head;
    void *end;
} EncodeUnicodeBufferInfo;

#define U8_WRITER(_unicode_buffer_info_) ((_unicode_buffer_info_)->writer.writer_u8)
#define U16_WRITER(_unicode_buffer_info_) ((_unicode_buffer_info_)->writer.writer_u16)
#define U32_WRITER(_unicode_buffer_info_) ((_unicode_buffer_info_)->writer.writer_u32)

#define GET_VEC_ASCII_START(_unicode_buffer_info_) (_Py_CAST(PyASCIIObject *, (_unicode_buffer_info_)->head) + 1)
#define GET_VEC_COMPACT_START(_unicode_buffer_info_) (_Py_CAST(PyCompactUnicodeObject *, (_unicode_buffer_info_)->head) + 1)

#define VEC_END(_unicode_buffer_info_) ((_unicode_buffer_info_)->end)


force_noinline bool unicode_vec_reserve(EncodeUnicodeBufferInfo *unicode_buffer_info, void *target_ptr);


force_noinline void init_py_unicode(void *, Py_ssize_t size, int kind);

force_inline bool vec_in_boundary(EncodeUnicodeBufferInfo *unicode_buffer_info) {
    return unicode_buffer_info->writer.writer_u8 <= (u8 *)unicode_buffer_info->end && unicode_buffer_info->writer.writer_u8 >= (u8 *)unicode_buffer_info->head;
}

/* Resize the vector pointed by `vec_addr`.
 * If resize succeed, the vector will be updated to the new address and return true.
 * Otherwise, `vec_addr` left unchanged and returns false.
 * Args:
 *     vec_addr: The address of the vector.
 *     len: Count of valid unicode points in the vector.
 *     ucs_type: The unicode type of the vector (0 stands for ascii).
 */
force_noinline bool vector_resize_to_fit(EncodeUnicodeBufferInfo *unicode_buffer_info, Py_ssize_t len, int ucs_type);


#endif
