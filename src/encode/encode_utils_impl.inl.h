#include "ryu/ryu.h"
#include "simd/simd_impl.h"

#ifndef COMPILE_WRITE_UCS_LEVEL
#    error "COMPILE_WRITE_UCS_LEVEL is not defined"
#endif

#include "commondef/w_in.inl.h"
#include "unicode/include/reserve.h"

#define _ELEVATE_FROM_U8_NUM_BUFFER PYYJSON_CONCAT2(_elevate_u8_copy, COMPILE_WRITE_UCS_LEVEL)
#define VEC_WRITE_U64 PYYJSON_CONCAT2(vec_write_u64, COMPILE_WRITE_UCS_LEVEL)
#define VEC_WRITE_F64 PYYJSON_CONCAT2(vec_write_f64, COMPILE_WRITE_UCS_LEVEL)
/*
 * (PRIVATE)
 * Elevate the u8 buffer to the vector.
 * The space (32 * sizeof(_TARGET_TYPE)) must be reserved before calling this function.
 */
force_inline void _ELEVATE_FROM_U8_NUM_BUFFER(EncodeUnicodeBufferInfo *unicode_buffer_info, u8 *buffer, Py_ssize_t len) {
#if COMPILE_WRITE_UCS_LEVEL == 1
    Py_UNREACHABLE();
    assert(false);
#else // COMPILE_WRITE_UCS_LEVEL != 1
    assert(len >= 0 && len <= 32);
    // there are two cases: 1 -> 2 or 1 -> 4
    // 1 -> 2:
    // for simd size < 512, load 128 (16 bytes) and write 128 or 256.
    // for simd size == 512, load 256 (32 bytes) and write 512.
    // 2 -> 4:
    // always load 128 (16 bytes), and write 128/256/512.
#    if SIMD_BIT_SIZE == 512 && COMPILE_WRITE_UCS_LEVEL == 2
    SIMD_256 y;
    SIMD_512 z;
    y = load_256((const void *)buffer);
    z = elevate_1_2_to_512(y);
    write_512((void *)_WRITER(unicode_buffer_info), z); // processed 32, done
    _WRITER(unicode_buffer_info) += len;
#    else // SIMD_BIT_SIZE != 512 || COMPILE_WRITE_UCS_LEVEL == 4
    const Py_ssize_t per_write_count = SIMD_BIT_SIZE / 8 / COMPILE_WRITE_UCS_LEVEL;
    _TARGET_TYPE *writer = _WRITER(unicode_buffer_info);
    u8 *buffer_end = buffer + len;
#        define ELEVATOR PYYJSON_CONCAT4(elevate_1, COMPILE_WRITE_UCS_LEVEL, to, SIMD_BIT_SIZE)
#        define WRITER PYYJSON_CONCAT2(write, SIMD_BIT_SIZE)
    while (buffer < buffer_end) {
        WRITER((void *)writer, ELEVATOR(load_128((const void *)buffer)));
        writer += per_write_count;
        buffer += per_write_count;
    }
#        undef ELEVATOR
#        undef WRITER
    _WRITER(unicode_buffer_info) += len;
#    endif // SIMD_BIT_SIZE != 512 || COMPILE_WRITE_UCS_LEVEL == 4
    assert(vec_in_boundary(unicode_buffer_info));
#endif     // COMPILE_WRITE_UCS_LEVEL != 1
}

/*
 * Write a u64 number to the vector.
 * The space (32 * sizeof(_TARGET_TYPE)) must be reserved before calling this function.
 */
force_inline void VEC_WRITE_U64(EncodeUnicodeBufferInfo *unicode_buffer_info, u64 val, usize sign) {
    assert(sign <= 1);
#if COMPILE_WRITE_UCS_LEVEL == 1
    u8 *buffer = _WRITER(unicode_buffer_info);
#else
    u8 _buffer[64];
    u8 *buffer = _buffer;
#endif
    if (sign) *buffer = '-';
    u8 *buffer_end = write_u64(val, buffer + sign);
#if COMPILE_WRITE_UCS_LEVEL == 1
    unicode_buffer_info->writer.writer_u8 = buffer_end;
#else
    Py_ssize_t write_len = buffer_end - buffer;
    _ELEVATE_FROM_U8_NUM_BUFFER(unicode_buffer_info, buffer, write_len);
#endif
    assert(vec_in_boundary(unicode_buffer_info));
}

/*
 * Write a f64 number to the vector.
 * The space (32 * sizeof(_TARGET_TYPE)) must be reserved before calling this function.
 */
force_inline void VEC_WRITE_F64(EncodeUnicodeBufferInfo *unicode_buffer_info, u64 val_u64_repr) {
#if COMPILE_WRITE_UCS_LEVEL == 1
    u8 *buffer = _WRITER(unicode_buffer_info);
#else
    u8 _buffer[64];
    u8 *buffer = _buffer;
#endif
    u8 *buffer_end = buffer + d2s_buffered_n(f64_from_raw(val_u64_repr), (char *)buffer);
#if COMPILE_WRITE_UCS_LEVEL == 1
    unicode_buffer_info->writer.writer_u8 = buffer_end;
#else
    Py_ssize_t write_len = buffer_end - buffer;
    _ELEVATE_FROM_U8_NUM_BUFFER(unicode_buffer_info, buffer, write_len);
#endif
}

#include "commondef/w_out.inl.h"

#undef VEC_RESERVE
#undef VEC_WRITE_F64
#undef VEC_WRITE_U64
#undef _ELEVATE_FROM_U8_NUM_BUFFER
