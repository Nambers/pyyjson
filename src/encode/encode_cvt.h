#ifndef PYYJSON_ENCODE_CVT_H
#define PYYJSON_ENCODE_CVT_H

#include "pyyjson.h"
#include "simd/cvt.h"
#include "simd/simd_detect.h"
#include "unicode/unicode_buffer.h"

force_inline void ascii_elevate2(EncodeUnicodeBufferInfo *unicode_buffer_info, UnicodeInfo *unicode_info) {
    u8 *start = ((u8 *)GET_VEC_ASCII_START(unicode_buffer_info));
    u16 *write_start = ((u16 *)GET_VEC_COMPACT_START(unicode_buffer_info));
    SIMD_NAME_MODIFIER(long_back_elevate_1_2)(write_start, start, unicode_info->ascii_size);
}

force_inline void ascii_elevate4(EncodeUnicodeBufferInfo *unicode_buffer_info, UnicodeInfo *unicode_info) {
    u8 *start = ((u8 *)GET_VEC_ASCII_START(unicode_buffer_info));
    u32 *write_start = ((u32 *)GET_VEC_COMPACT_START(unicode_buffer_info));
    SIMD_NAME_MODIFIER(long_back_elevate_1_4)(write_start, start, unicode_info->ascii_size);
}

force_inline void ucs1_elevate2(EncodeUnicodeBufferInfo *unicode_buffer_info, UnicodeInfo *unicode_info) {
    Py_ssize_t offset = unicode_info->ascii_size;
    u8 *start = ((u8 *)GET_VEC_COMPACT_START(unicode_buffer_info)) + offset;
    u16 *write_start = ((u16 *)GET_VEC_COMPACT_START(unicode_buffer_info)) + offset;
    SIMD_NAME_MODIFIER(long_back_elevate_1_2)(write_start, start, unicode_info->u8_size);
}

force_inline void ucs1_elevate4(EncodeUnicodeBufferInfo *unicode_buffer_info, UnicodeInfo *unicode_info) {
    Py_ssize_t offset = unicode_info->ascii_size;
    u8 *start = ((u8 *)GET_VEC_COMPACT_START(unicode_buffer_info)) + offset;
    u32 *write_start = ((u32 *)GET_VEC_COMPACT_START(unicode_buffer_info)) + offset;
    SIMD_NAME_MODIFIER(long_back_elevate_1_4)(write_start, start, unicode_info->u8_size);
}

force_inline void ucs2_elevate4(EncodeUnicodeBufferInfo *unicode_buffer_info, UnicodeInfo *unicode_info) {
    Py_ssize_t offset = unicode_info->ascii_size + unicode_info->u8_size;
    u16 *start = ((u16 *)GET_VEC_COMPACT_START(unicode_buffer_info)) + offset;
    u32 *write_start = ((u32 *)GET_VEC_COMPACT_START(unicode_buffer_info)) + offset;
    SIMD_NAME_MODIFIER(long_back_elevate_2_4)(write_start, start, unicode_info->u16_size);
}

force_inline void ascii_elevate1(EncodeUnicodeBufferInfo *unicode_buffer_info, UnicodeInfo *unicode_info) {
    memmove(GET_VEC_COMPACT_START(unicode_buffer_info), GET_VEC_ASCII_START(unicode_buffer_info), unicode_info->ascii_size);
}

#endif // PYYJSON_ENCODE_CVT_H
