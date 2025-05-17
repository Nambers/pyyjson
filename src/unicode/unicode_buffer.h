
#ifndef PYYJSON_UNICODE_BUFFER_H
#define PYYJSON_UNICODE_BUFFER_H


#include "pyyjson.h"

typedef struct UnicodeInfo {
    Py_ssize_t ascii_size;
    Py_ssize_t u8_size;
    Py_ssize_t u16_size;
    Py_ssize_t u32_size;
    int cur_ucs_type;
} UnicodeInfo;

typedef struct EncodeUnicodeBufferInfo {
    void *head;
    void *end;
} EncodeUnicodeBufferInfo;

typedef union {
    u8 *writer_u8;
    u16 *writer_u16;
    u32 *writer_u32;
    void *writer_void;
} EncodeUnicodeWriter;

#define U8_WRITER(_writer_addr_) ((_writer_addr_)->writer_u8)
#define U16_WRITER(_writer_addr_) ((_writer_addr_)->writer_u16)
#define U32_WRITER(_writer_addr_) ((_writer_addr_)->writer_u32)

#define GET_VEC_ASCII_START(_unicode_buffer_info_) (PYYJSON_CAST(PyASCIIObject *, (_unicode_buffer_info_)->head) + 1)
#define GET_VEC_COMPACT_START(_unicode_buffer_info_) (PYYJSON_CAST(PyCompactUnicodeObject *, (_unicode_buffer_info_)->head) + 1)

#define VEC_END(_unicode_buffer_info_) ((_unicode_buffer_info_)->end)


bool _unicode_buffer_reserve(EncodeUnicodeBufferInfo *unicode_buffer_info, usize target_size);


force_noinline void init_pyunicode(void *, Py_ssize_t size, int kind);

force_inline bool check_unicode_writer_valid(void *writer, EncodeUnicodeBufferInfo *unicode_buffer_info) {
    return PYYJSON_CAST(u8 *, writer) <= (u8 *)unicode_buffer_info->end && PYYJSON_CAST(u8 *, writer) >= (u8 *)unicode_buffer_info->head;
}

/* Resize the buffer described by `unicode_buffer_info`.
 * If resize succeed, the buffer will be updated to the new address and return true.
 * Otherwise, buffer left unchanged and returns false.
 * Args:
 *     unicode_buffer_info: The buffer.
 *     len: Count of valid unicode points in the buffer.
 *     ucs_type: The unicode type of the buffer (0 stands for ascii).
 */
force_noinline bool resize_to_fit_pyunicode(EncodeUnicodeBufferInfo *unicode_buffer_info, Py_ssize_t len, int ucs_type);


#endif // PYYJSON_UNICODE_BUFFER_H
