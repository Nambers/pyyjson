#include "simd/simd_impl.h"

#ifndef COMPILE_WRITE_UCS_LEVEL
#error "COMPILE_WRITE_UCS_LEVEL is not defined"
#endif

#include "commondef/w_in.inl.h"
#include "unicode/include/reserve.h"

#define _ELEVATE_FROM_U8_NUM_BUFFER PYYJSON_CONCAT2(_elevate_u8_copy, COMPILE_WRITE_UCS_LEVEL)


/*
 * (PRIVATE)
 * Elevate the u8 buffer to the vector.
 * The space (32 * sizeof(_TARGET_TYPE)) must be reserved before calling this function.
 */
force_inline void _ELEVATE_FROM_U8_NUM_BUFFER(UnicodeVector *vec, u8 *buffer, Py_ssize_t len) {
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
#if SIMD_BIT_SIZE == 512 && COMPILE_WRITE_UCS_LEVEL == 2
    SIMD_256 y;
    SIMD_512 z;
    y = load_256((const void *) buffer);
    z = elevate_1_2_to_512(y);
    write_512((void *) _WRITER(vec), z); // processed 32, done
    _WRITER(vec) += len;
#else // SIMD_BIT_SIZE == 512 && COMPILE_WRITE_UCS_LEVEL == 2
    const Py_ssize_t per_write_count = SIMD_BIT_SIZE / 8 / COMPILE_WRITE_UCS_LEVEL;
    _TARGET_TYPE *writer = _WRITER(vec);
    u8 *buffer_end = buffer + len;
    SIMD_128 _x;
    SIMD_TYPE SIMD_VAR;
    while (buffer < buffer_end) {
        _x = load_128((const void *) buffer);
#if SIMD_BIT_SIZE == 512
        z = elevate_1_4_to_512(_x);
        write_512((void *) writer, z);
#elif SIMD_BIT_SIZE == 256
        y = PYYJSON_CONCAT3(elevate_1, COMPILE_WRITE_UCS_LEVEL, to_256)(_x);
        write_256((void *) writer, y);
#else  // SIMD_BIT_SIZE == 128
        x = PYYJSON_CONCAT3(elevate_1, COMPILE_WRITE_UCS_LEVEL, to_128)(_x);
        write_128((void *) writer, x);
#endif // SIMD_BIT_SIZE
        writer += per_write_count;
        buffer += per_write_count;
    }
    _WRITER(vec) += len;
#endif // SIMD_BIT_SIZE == 512 && COMPILE_WRITE_UCS_LEVEL == 2
    assert(vec_in_boundary(vec));
#endif // COMPILE_WRITE_UCS_LEVEL != 1
}

/*
 * Write a u64 number to the vector.
 * The space (32 * sizeof(_TARGET_TYPE)) must be reserved before calling this function.
 */
force_inline void PYYJSON_CONCAT2(vec_write_u64, COMPILE_WRITE_UCS_LEVEL)(UnicodeVector *vec, u64 val, usize sign) {
    assert(sign <= 1);
#if COMPILE_WRITE_UCS_LEVEL == 1
    u8 *buffer = _WRITER(vec);
#else
    u8 _buffer[64];
    u8 *buffer = _buffer;
#endif
    if (sign) *buffer = '-';
    u8 *buffer_end = write_u64(val, buffer + sign);
#if COMPILE_WRITE_UCS_LEVEL == 1
    vec->head.write_u8 = buffer_end;
#else
    Py_ssize_t write_len = buffer_end - buffer;
    _ELEVATE_FROM_U8_NUM_BUFFER(vec, buffer, write_len);
#endif
    assert(vec_in_boundary(vec));
}

/*
 * Write a f64 number to the vector.
 * The space (32 * sizeof(_TARGET_TYPE)) must be reserved before calling this function.
 */
force_inline void PYYJSON_CONCAT2(vec_write_f64, COMPILE_WRITE_UCS_LEVEL)(UnicodeVector *vec, u64 val_u64_repr) {
#if COMPILE_WRITE_UCS_LEVEL == 1
    u8 *buffer = _WRITER(vec);
#else
    u8 _buffer[64];
    u8 *buffer = _buffer;
#endif
    u8 *buffer_end = write_f64_raw(buffer, val_u64_repr);
#if COMPILE_WRITE_UCS_LEVEL == 1
    vec->head.write_u8 = buffer_end;
#else
    Py_ssize_t write_len = buffer_end - buffer;
    _ELEVATE_FROM_U8_NUM_BUFFER(vec, buffer, write_len);
#endif
}
#include "commondef/w_out.inl.h"

#undef _ELEVATE_FROM_U8_NUM_BUFFER
#undef VEC_RESERVE
