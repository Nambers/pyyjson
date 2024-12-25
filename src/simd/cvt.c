#include "simd/mask_table.h"
#include "simd/simd_impl.h"
#include "unicode/uvector.h"


force_inline void _long_back_elevate_1_4_small_tail_2(u8 **restrict read_end_addr, u32 **restrict write_end_addr) {
#if SIMD_BIT_SIZE == 256
    // 64 -> 256.
    SIMD_128 x_read;
    SIMD_256 elevated;
    *read_end_addr -= 8;
    *write_end_addr -= 8;
    x_read = broadcast_64_128(*(i64 *) *read_end_addr);
    elevated = elevate_1_4_to_256(x_read);
    write_256((void *) *write_end_addr, elevated);
#else
    SIMD_128 x_read, elevated;
    *read_end_addr -= 8;
    *write_end_addr -= 8;
    i32 *read_i32_ptr = (i32 *) *read_end_addr;
    // TODO check this assembly
    x_read = set_32_128(*read_i32_ptr, 0, *(read_i32_ptr + 1), 0);
    elevated = elevate_1_4_to_128(x_read);
    write_128((void *) *write_end_addr, elevated);
    x_read = unpack_hi_64_128(x_read, x_read);
    elevated = elevate_1_4_to_128(x_read);
    write_128((void *) ((*write_end_addr) + 4), elevated);
#endif
}


#if SIMD_BIT_SIZE > 128
force_inline void _long_back_elevate_2_4_loop_impl(u16 *restrict read_start, u16 *restrict read_end, u32 *restrict write_end, Py_ssize_t read_once_count, SIMD_HALF_TYPE (*load_interface)(const void *), void (*write_interface)(void *, SIMD_TYPE)) {
    read_end -= read_once_count;
    write_end -= read_once_count;
    while (read_end >= read_start) {
        SIMD_HALF_TYPE half;
        SIMD_TYPE full;
        half = load_interface(read_end);
        full = PYYJSON_CONCAT2(elevate_2_4_to, SIMD_BIT_SIZE)(half);
        write_interface(write_end, full);
        read_end -= read_once_count;
        write_end -= read_once_count;
    }
}
#endif // SIMD_BIT_SIZE > 128


// long_back_elevate_1_4 tool
#if SIMD_BIT_SIZE == 512
force_inline void _long_back_elevate_1_4_loop_impl(u8 *restrict read_start, u8 *restrict read_end, u32 *restrict write_end, Py_ssize_t read_once_count, SIMD_128 (*load_interface)(const void *), void (*write_interface)(void *, SIMD_TYPE)) {
    SIMD_128 small;
    SIMD_512 full;
    read_end -= read_once_count;
    write_end -= read_once_count;
    while (read_end >= read_start) {
        small = load_interface((const void *) read_end);
        full = elevate_1_4_to_512(small);
        write_interface((void *) write_end, full);
        read_end -= read_once_count;
        write_end -= read_once_count;
    }
}
#endif

force_inline void _long_back_elevate_1_4_small_tail_1(u8 **restrict read_end_addr, u32 **restrict write_end_addr) {
    SIMD_128 x_read, elevated;
    *read_end_addr -= 4;
    *write_end_addr -= 4;
    x_read = broadcast_32_128(*(i32 *) *read_end_addr);
    elevated = elevate_1_4_to_128(x_read);
    write_128((void *) *write_end_addr, elevated);
}


#if SIMD_BIT_SIZE > 128
force_inline void _long_back_elevate_1_2_loop_impl(u8 *restrict read_start, u8 *restrict read_end, u16 *restrict write_end, Py_ssize_t read_once_count, SIMD_HALF_TYPE (*load_interface)(const void *), void (*write_interface)(void *, SIMD_TYPE)) {
    read_end -= read_once_count;
    write_end -= read_once_count;
    while (read_end >= read_start) {
        SIMD_HALF_TYPE half;
        SIMD_TYPE full;
        half = load_interface(read_end);
        full = PYYJSON_CONCAT2(elevate_1_2_to, SIMD_BIT_SIZE)(half);
        write_interface(write_end, full);
        read_end -= read_once_count;
        write_end -= read_once_count;
    }
}
#else
force_inline void _long_back_elevate_1_2_small_tail_1(u8 **read_end_addr, u16 **write_end_addr) {
    SIMD_128 x_read, elevated;
    *read_end_addr -= 8;
    *write_end_addr -= 8;
    x_read = broadcast_64_128(*(i64 *) *read_end_addr);
    elevated = elevate_1_2_to_128(x_read);
    write_128((void *) *write_end_addr, elevated);
}

force_inline void _long_back_elevate_2_4_small_tail_1(u16 **read_end_addr, u32 **write_end_addr) {
    SIMD_128 x_read, elevated;
    *read_end_addr -= 4;
    *write_end_addr -= 4;
    x_read = broadcast_64_128(*(i64 *) *read_end_addr);
    elevated = elevate_2_4_to_128(x_read);
    write_128((void *) *write_end_addr, elevated);
}
#endif // SIMD_BIT_SIZE


void long_back_elevate_1_2(u16 *restrict write_start, u8 *restrict read_start, Py_ssize_t len) {
    // only 128 -> 256 and 256 -> 512 should consider aligness.
    // 64(128) -> 128 cannot be aligned anyway.
    u8 *read_end = read_start + len;
    u16 *write_end = write_start + len;
#if SIMD_BIT_SIZE > 128
    const Py_ssize_t read_once_count = SIMD_BIT_SIZE / 2 / 8;
    Py_ssize_t tail_len = len & (read_once_count - 1);
    if (tail_len) {
#if SIMD_BIT_SIZE == 256
        // read and write with blendv.
        SIMD_128 x;
        SIMD_256 y, mask, blend;
        u16 *const write_tail_start = write_end - read_once_count;
        x = load_128((const void *) (read_end - read_once_count));
        y = elevate_1_2_to_256(x);
        mask = load_256_aligned(read_tail_mask_table_16(read_once_count - tail_len));
        blend = load_256((const void *) write_tail_start);
        y = blendv_256(blend, y, mask);
        write_256((void *) write_tail_start, y);
#else
        // 512, use mask_storeu.
        SIMD_256 y;
        SIMD_512 z;
        y = load_256((const void *) (read_end - tail_len));
        z = elevate_1_2_to_512(y);
        _mm512_mask_storeu_epi16((void *) (write_end - tail_len), (1 << (usize) tail_len) - 1, z);
#endif // SIMD_BIT_SIZE
        len &= ~(read_once_count - 1);
        read_end = read_start + len;
        write_end = write_start + len;
    }
    if (0 == (((Py_ssize_t) (read_start)) & (read_once_count - 1))) {
        if (0 == (((Py_ssize_t) (write_start)) & (read_once_count * 2 - 1))) {
            goto elevate_both_aligned;
        } else {
            goto elevate_src_aligned;
        }
    } else {
        if (0 == (((Py_ssize_t) (write_start)) & (read_once_count * 2 - 1))) {
            goto elevate_dst_aligned;
        } else {
            goto elevate_both_not_aligned;
        }
    }
elevate_both_aligned:;
    _long_back_elevate_1_2_loop_impl(read_start, read_end, write_end, read_once_count, load_aligned_half, write_aligned);
    return;
elevate_src_aligned:;
    _long_back_elevate_1_2_loop_impl(read_start, read_end, write_end, read_once_count, load_aligned_half, write_simd);
    return;
elevate_dst_aligned:;
    _long_back_elevate_1_2_loop_impl(read_start, read_end, write_end, read_once_count, load_half, write_aligned);
    return;
elevate_both_not_aligned:;
    _long_back_elevate_1_2_loop_impl(read_start, read_end, write_end, read_once_count, load_half, write_simd);
    return;
#else // SIMD_BIT_SIZE == 128
    SIMD_128 x_read;
    SIMD_128 x;
    // 16 bytes as a block.
    Py_ssize_t tail_len = len & (Py_ssize_t) 15;
    if (tail_len) {
        usize tail_parts = (usize) tail_len >> 3;
        usize small_tail_len = (usize) tail_len & 7;
        for (usize i = 0; i < small_tail_len; ++i) {
            *--write_end = *--read_end;
        }
        if (tail_parts) {
            // 64 -> 128.
            _long_back_elevate_1_2_small_tail_1(&read_end, &write_end);
        }
        len &= ~(Py_ssize_t) 15;
    }
    read_end -= 16;
    write_end -= 16;
    while (read_end >= read_start) {
        x_read = load_128((const void *) read_end);
        x = elevate_1_2_to_128(x_read);
        write_128((void *) write_end, x);
        x_read = unpack_hi_64_128(x_read, x_read);
        x = elevate_1_2_to_128(x_read);
        write_128((void *) (write_end + 8), x);
        read_end -= 16;
        write_end -= 16;
    }
#endif
}


void long_back_elevate_1_4(u32 *restrict write_start, u8 *restrict read_start, Py_ssize_t len) {
    // only 128 -> 512 should consider aligness.
    // 32/64(128) -> 128/256 cannot be aligned anyway.
    u8 *read_end = read_start + len;
    u32 *write_end = write_start + len;
#if SIMD_BIT_SIZE == 512
    const Py_ssize_t read_once_count = SIMD_BIT_SIZE / 4 / 8;
    Py_ssize_t tail_len = len & (read_once_count - 1);
    if (tail_len) {
        SIMD_128 x;
        SIMD_512 z;
        x = load_128((const void *) (read_end - tail_len));
        z = elevate_1_4_to_512(x);
        _mm512_mask_storeu_epi32((void *) (write_end - tail_len), ((__mmask16) 1 << (usize) tail_len) - 1, z);
        len &= ~(read_once_count - 1);
        read_end = read_start + len;
        write_end = write_start + len;
    }
    if (0 == (((Py_ssize_t) (read_start)) & (128 / 8 - 1))) {
        if (0 == (((Py_ssize_t) (write_start)) & (512 / 8 - 1))) {
            goto elevate_both_aligned;
        } else {
            goto elevate_src_aligned;
        }
    } else {
        if (0 == (((Py_ssize_t) (write_start)) & (512 / 8 - 1))) {
            goto elevate_dst_aligned;
        } else {
            goto elevate_both_not_aligned;
        }
    }
elevate_both_aligned:;
    _long_back_elevate_1_4_loop_impl(read_start, read_end, write_end, read_once_count, load_128_aligned, write_aligned);
    return;
elevate_src_aligned:;
    _long_back_elevate_1_4_loop_impl(read_start, read_end, write_end, read_once_count, load_128_aligned, write_simd);
    return;
elevate_dst_aligned:;
    _long_back_elevate_1_4_loop_impl(read_start, read_end, write_end, read_once_count, load_128, write_aligned);
    return;
elevate_both_not_aligned:;
    _long_back_elevate_1_4_loop_impl(read_start, read_end, write_end, read_once_count, load_128, write_simd);
    return;
#else
    SIMD_128 x_read;
    SIMD_TYPE SIMD_VAR;
    // 16 bytes as a block.
    Py_ssize_t tail_len = len & (Py_ssize_t) 15;
    if (tail_len) {
        usize tail_parts = (usize) tail_len >> 2;
        usize small_tail_len = (usize) tail_len & 3;
        for (usize i = 0; i < small_tail_len; ++i) {
            *--write_end = *--read_end;
        }
        switch (tail_parts) {
            case 0: {
                // nothing.
                break;
            }
            case 1: {
                // 32 -> 128.
                _long_back_elevate_1_4_small_tail_1(&read_end, &write_end);
                break;
            }
            case 2: {
                _long_back_elevate_1_4_small_tail_2(&read_end, &write_end);
                break;
            }
            case 3: {
                // 3 = 1 + 2, so...
                _long_back_elevate_1_4_small_tail_1(&read_end, &write_end);
                _long_back_elevate_1_4_small_tail_2(&read_end, &write_end);
                break;
            }
            default: {
                Py_UNREACHABLE();
                assert(false);
                break;
            }
        }

        len &= ~(Py_ssize_t) 15;
    }
    read_end -= 16;
    write_end -= 16;
    while (read_end >= read_start) {
        x_read = load_128((const void *) read_end);
#if SIMD_BIT_SIZE == 256
        SIMD_VAR = elevate_1_4_to_256(x_read);
        write_256((void *) write_end, SIMD_VAR);
        x_read = unpack_hi_64_128(x_read, x_read);
        SIMD_VAR = elevate_1_4_to_256(x_read);
        write_256((void *) (write_end + 8), SIMD_VAR);
#else
        SIMD_VAR = elevate_1_4_to_128(x_read);
        write_128((void *) write_end, SIMD_VAR);
        RIGHT_SHIFT_128BITS(x_read, 32, &x_read);
        SIMD_VAR = elevate_1_4_to_128(x_read);
        write_128((void *) (write_end + 4), SIMD_VAR);
        RIGHT_SHIFT_128BITS(x_read, 32, &x_read);
        SIMD_VAR = elevate_1_4_to_128(x_read);
        write_128((void *) (write_end + 8), SIMD_VAR);
        RIGHT_SHIFT_128BITS(x_read, 32, &x_read);
        SIMD_VAR = elevate_1_4_to_128(x_read);
        write_128((void *) (write_end + 12), SIMD_VAR);
#endif
        read_end -= 16;
        write_end -= 16;
    }
#endif
}


void long_back_elevate_2_4(u32 *restrict write_start, u16 *restrict read_start, Py_ssize_t len) {
    // TODO
    // only 128 -> 256 and 256 -> 512 should consider aligness.
    // 64(128) -> 128 cannot be aligned anyway.
    u16 *read_end = read_start + len;
    u32 *write_end = write_start + len;
#if SIMD_BIT_SIZE > 128
    const Py_ssize_t read_once_count = SIMD_BIT_SIZE / 4 / 8;
    Py_ssize_t tail_len = len & (read_once_count - 1);
    if (tail_len) {
#if SIMD_BIT_SIZE == 256
        // read and write with blendv.
        SIMD_128 x;
        SIMD_256 y, mask, blend;
        u32 *const write_tail_start = write_end - read_once_count;
        x = load_128((const void *) (read_end - read_once_count));
        y = elevate_2_4_to_256(x);
        mask = load_256_aligned(read_tail_mask_table_32(read_once_count - tail_len));
        blend = load_256((const void *) write_tail_start);
        y = blendv_256(blend, y, mask);
        write_256((void *) write_tail_start, y);
#else
        // 512, use mask_storeu.
        SIMD_256 y;
        SIMD_512 z;
        y = load_256((const void *) (read_end - tail_len));
        z = elevate_2_4_to_512(y);
        _mm512_mask_storeu_epi32((void *) (write_end - tail_len), (1 << (usize) tail_len) - 1, z);
#endif // SIMD_BIT_SIZE
        len &= ~(read_once_count - 1);
        read_end = read_start + len;
        write_end = write_start + len;
    }
    if (0 == (((Py_ssize_t) (read_start)) & (read_once_count * 2 - 1))) {
        if (0 == (((Py_ssize_t) (write_start)) & (read_once_count * 4 - 1))) {
            goto elevate_both_aligned;
        } else {
            goto elevate_src_aligned;
        }
    } else {
        if (0 == (((Py_ssize_t) (write_start)) & (read_once_count * 4 - 1))) {
            goto elevate_dst_aligned;
        } else {
            goto elevate_both_not_aligned;
        }
    }
elevate_both_aligned:;
    _long_back_elevate_2_4_loop_impl(read_start, read_end, write_end, read_once_count, load_aligned_half, write_aligned);
    return;
elevate_src_aligned:;
    _long_back_elevate_2_4_loop_impl(read_start, read_end, write_end, read_once_count, load_aligned_half, write_simd);
    return;
elevate_dst_aligned:;
    _long_back_elevate_2_4_loop_impl(read_start, read_end, write_end, read_once_count, load_half, write_aligned);
    return;
elevate_both_not_aligned:;
    _long_back_elevate_2_4_loop_impl(read_start, read_end, write_end, read_once_count, load_half, write_simd);
    return;
#else // SIMD_BIT_SIZE == 128
    SIMD_128 x_read;
    SIMD_128 x;
    // 16 bytes as a block. i.e. 8 WORDs
    Py_ssize_t tail_len = len & (Py_ssize_t) 7;
    if (tail_len) {
        usize tail_parts = (usize) tail_len >> 2;
        usize small_tail_len = (usize) tail_len & 3;
        for (usize i = 0; i < small_tail_len; ++i) {
            *--write_end = *--read_end;
        }
        if (tail_parts) {
            // 64 -> 128.
            _long_back_elevate_2_4_small_tail_1(&read_end, &write_end);
        }
        len &= ~(Py_ssize_t) 7;
    }
    read_end -= 8;
    write_end -= 8;
    while (read_end >= read_start) {
        x_read = load_128((const void *) read_end);
        x = elevate_2_4_to_128(x_read);
        write_128((void *) write_end, x);
        x_read = unpack_hi_64_128(x_read, x_read);
        x = elevate_2_4_to_128(x_read);
        write_128((void *) (write_end + 4), x);
        read_end -= 8;
        write_end -= 8;
    }
#endif
}
