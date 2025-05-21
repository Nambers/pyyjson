#ifdef PYYJSON_CLANGD_DUMMY
#    ifndef COMPILE_CONTEXT_ENCODE
#        define COMPILE_CONTEXT_ENCODE
#    endif
#    ifndef COMPILE_WRITE_UCS_LEVEL
#        include "encode_shared.h"
#        include "simd/simd_impl.h"
#        define COMPILE_WRITE_UCS_LEVEL 1
#        include "simd/compile_feature_check.h"
#    endif
#endif

#include "compile_context/w_in.inl.h"

#define _ELEVATE_FROM_U8_NUM_BUFFER MAKE_W_NAME(_elevate_u8_copy)

/*
 * (PRIVATE)
 * Convert the u8 buffer to the buffer.
 * The space (32 * sizeof(_dst_t)) must be reserved before calling this function.
 */
force_inline void _ELEVATE_FROM_U8_NUM_BUFFER(_dst_t **writer_addr, u8 *buffer, Py_ssize_t len) {
    _dst_t *writer = *writer_addr;
#if COMPILE_WRITE_UCS_LEVEL == 1
    PYYJSON_UNREACHABLE();
#else // COMPILE_WRITE_UCS_LEVEL != 1
    assert(len >= 0 && len <= 32);
    // there are two cases: 1 -> 2 or 1 -> 4
    // 1 -> 2:
    // for simd size < 512, load 128 (16 bytes) and write 128 or 256.
    // for simd size == 512, load 256 (32 bytes) and write 512.
    // 2 -> 4:
    // always load 128 (16 bytes), and write 128/256/512.
#    if COMPILE_SIMD_BITS == 512 && COMPILE_WRITE_UCS_LEVEL == 2
    // SIMD_256 y;
    // SIMD_512 z;
    // y = load_256((const void *)buffer);
    // z = cvt_u8_to_u16_512(y);
    // *(vector_u_u16_512*)writer = z;
    *(vector_u_u16_512 *)writer = cvt_u8_to_u16_512(*(vector_u_u8_256 *)buffer);
    // write_512((void *)writer, z); // processed 32, done
    writer += len;
#    else // COMPILE_SIMD_BITS != 512 || COMPILE_WRITE_UCS_LEVEL == 4
    const Py_ssize_t per_write_count = COMPILE_SIMD_BITS / 8 / COMPILE_WRITE_UCS_LEVEL;
    _dst_t *writer2 = writer;
    u8 *buffer_end = buffer + len;
#        define ELEVATOR PYYJSON_CONCAT3(cvt_u8_to, _dst_t, COMPILE_SIMD_BITS)
#        define WRITER PYYJSON_CONCAT2(write, COMPILE_SIMD_BITS)
    while (buffer < buffer_end) {
        *(PYYJSON_CONCAT2(vector_u_u32, COMPILE_SIMD_BITS) *)writer2 = ELEVATOR(*(const vector_u_u8_128 *)buffer);
        // WRITER((void *)writer2, ELEVATOR(load_128((const void *)buffer)));
        writer2 += per_write_count;
        buffer += per_write_count;
    }
#        undef ELEVATOR
#        undef WRITER
    writer += len;
#    endif // COMPILE_SIMD_BITS != 512 || COMPILE_WRITE_UCS_LEVEL == 4
    // assert(check_unicode_writer_valid(unicode_buffer_info));
#endif     // COMPILE_WRITE_UCS_LEVEL != 1
    *writer_addr = writer;
}

/*
 * Write a u64 number to the buffer.
 * The space (32 * sizeof(_dst_t)) must be reserved before calling this function.
 */
force_inline void u64_to_unicode(_dst_t **writer_addr, u64 val, usize sign) {
    assert(sign <= 1);
#if COMPILE_WRITE_UCS_LEVEL == 1
    u8 *buffer = *writer_addr; //_WRITER(unicode_buffer_info);
#else
    u8 _buffer[64];
    u8 *buffer = _buffer;
#endif
    if (sign) *buffer = '-';
    u8 *buffer_end = write_u64(val, buffer + sign);
#if COMPILE_WRITE_UCS_LEVEL == 1
    *writer_addr = buffer_end;
    // unicode_buffer_info->writer.writer_u8 = buffer_end;
#else
    Py_ssize_t write_len = buffer_end - buffer;
    _ELEVATE_FROM_U8_NUM_BUFFER(writer_addr, buffer, write_len);
#endif
    // assert(check_unicode_writer_valid(unicode_buffer_info));
}

/*
 * Write a f64 number to the buffer.
 * The space (32 * sizeof(_dst_t)) must be reserved before calling this function.
 */
force_inline void f64_to_unicode(_dst_t **writer_addr, u64 val_u64_repr) {
#if COMPILE_WRITE_UCS_LEVEL == 1
    u8 *buffer = *writer_addr; //_WRITER(unicode_buffer_info);
#else
    u8 _buffer[64];
    u8 *buffer = _buffer;
#endif
    u8 *buffer_end = buffer + d2s_buffered_n(f64_from_raw(val_u64_repr), (char *)buffer);
#if COMPILE_WRITE_UCS_LEVEL == 1
    *writer_addr = buffer_end;
    // unicode_buffer_info->writer.writer_u8 = buffer_end;
#else
    Py_ssize_t write_len = buffer_end - buffer;
    _ELEVATE_FROM_U8_NUM_BUFFER(writer_addr, buffer, write_len);
#endif
}

#include "compile_context/w_out.inl.h"

#undef unicode_buffer_reserve
#undef _ELEVATE_FROM_U8_NUM_BUFFER
