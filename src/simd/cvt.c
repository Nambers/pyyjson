#include "long_cvt.h"
#include "simd/mask_table.h"
#include "simd/simd_impl.h"
#include "unicode/unicode_buffer.h"
#if PYYJSON_X86
force_inline void _long_back_elevate_1_4_small_tail_2(u8 **restrict read_end_addr, u32 **restrict write_end_addr) {
#    if COMPILE_SIMD_BITS == 256
    // 64 -> 256.
    SIMD_128 x_read;
    SIMD_256 elevated;
    *read_end_addr -= 8;
    *write_end_addr -= 8;
    x_read = broadcast_u64_128(*(i64 *)*read_end_addr);
    elevated = elevate_1_4_to_256(x_read);
    write_256((void *)*write_end_addr, elevated);
#    else
    *read_end_addr -= 8;
    *write_end_addr -= 8;
    // assert(false);
#        if PYYJSON_X86
    vector_a_u8_128 x_read1, x_read2;
#        elif PYYJSON_AARCH
    vector_a_u8_32 x_read1, x_read2;
#        endif
    vector_a_u32_128 elevated1, elevated2;
    memcpy(&x_read1, (*read_end_addr) + 0, 4);
    memcpy(&x_read2, (*read_end_addr) + 4, 4);
    elevated1 = elevate_1_4_to_128(x_read1);
    elevated2 = elevate_1_4_to_128(x_read2);
    write_128((void *)((*write_end_addr) + 0), elevated1);
    write_128((void *)((*write_end_addr) + 4), elevated2);
#    endif
}
#endif

#if PYYJSON_X86 && COMPILE_SIMD_BITS > 128
force_inline void _long_back_elevate_2_4_loop_impl(u16 *restrict read_start, u16 *restrict read_end, u32 *restrict write_end, Py_ssize_t read_once_count, SIMD_HALF_TYPE (*load_interface)(const void *), void (*write_interface)(void *, SIMD_TYPE)) {
    read_end -= read_once_count;
    write_end -= read_once_count;
    while (read_end >= read_start) {
        SIMD_HALF_TYPE half;
        SIMD_TYPE full;
        half = load_interface(read_end);
        full = PYYJSON_CONCAT2(elevate_2_4_to, COMPILE_SIMD_BITS)(half);
        write_interface(write_end, full);
        read_end -= read_once_count;
        write_end -= read_once_count;
    }
}
#endif // COMPILE_SIMD_BITS > 128


// long_back_elevate_1_4 tool
#if PYYJSON_X86 && COMPILE_SIMD_BITS == 512
force_inline void _long_back_elevate_1_4_loop_impl(u8 *restrict read_start, u8 *restrict read_end, u32 *restrict write_end, Py_ssize_t read_once_count, SIMD_128 (*load_interface)(const void *), void (*write_interface)(void *, SIMD_TYPE)) {
    SIMD_128 small;
    SIMD_512 full;
    read_end -= read_once_count;
    write_end -= read_once_count;
    while (read_end >= read_start) {
        small = load_interface((const void *)read_end);
        full = elevate_1_4_to_512(small);
        write_interface((void *)write_end, full);
        read_end -= read_once_count;
        write_end -= read_once_count;
    }
}
#endif

#if PYYJSON_X86
force_inline void _long_back_elevate_1_4_small_tail_1(u8 **restrict read_end_addr, u32 **restrict write_end_addr) {
    *read_end_addr -= 4;
    *write_end_addr -= 4;
#    if PYYJSON_X86
    vector_a_u8_128 x_read;
#    elif PYYJSON_AARCH
    vector_a_u8_32 x_read;
#    endif
    vector_a_u32_128 elevated;
    memcpy(&x_read, *read_end_addr, 4);
    elevated = elevate_1_4_to_128(x_read);
    write_128((void *)*write_end_addr, elevated);
}
#endif

#if PYYJSON_X86 && COMPILE_SIMD_BITS > 128
force_inline void _long_back_elevate_1_2_loop_impl(u8 *restrict read_start, u8 *restrict read_end, u16 *restrict write_end, Py_ssize_t read_once_count, SIMD_HALF_TYPE (*load_interface)(const void *), void (*write_interface)(void *, SIMD_TYPE)) {
    read_end -= read_once_count;
    write_end -= read_once_count;
    while (read_end >= read_start) {
        SIMD_HALF_TYPE half;
        SIMD_TYPE full;
        half = load_interface(read_end);
        full = PYYJSON_CONCAT2(elevate_1_2_to, COMPILE_SIMD_BITS)(half);
        write_interface(write_end, full);
        read_end -= read_once_count;
        write_end -= read_once_count;
    }
}
#elif PYYJSON_X86
/* Elevate 64 bytes u8 src to u16 dst. */
force_inline void _long_back_elevate_1_2_small_tail_1(u8 **read_end_addr, u16 **write_end_addr) {
#    if PYYJSON_X86
    vector_a_u8_128 x_read;
#    elif PYYJSON_AARCH
    vector_a_u8_64 x_read;
#    endif
    // assert(false);
    vector_a_u16_128 elevated;
    *read_end_addr -= 8;
    *write_end_addr -= 8;
    memcpy(&x_read, *read_end_addr, 8);
    // x_read = broadcast_u64_128(*(i64 *)*read_end_addr);
    elevated = elevate_1_2_to_128(x_read);
    write_128((void *)*write_end_addr, elevated);
}

force_inline void _long_back_elevate_2_4_small_tail_1(u16 **read_end_addr, u32 **write_end_addr) {
#    if PYYJSON_X86
    vector_a_u16_128 x_read;
#    elif PYYJSON_AARCH
    vector_a_u16_64 x_read;
#    endif
    // assert(false);
    vector_a_u32_128 elevated;
    *read_end_addr -= 4;
    *write_end_addr -= 4;
    memcpy(&x_read, *read_end_addr, 8);
    // x_read = broadcast_u64_128(*(i64 *)*read_end_addr);
    elevated = elevate_2_4_to_128(x_read);
    write_128((void *)*write_end_addr, elevated);
}
#endif // COMPILE_SIMD_BITS


void SIMD_NAME_MODIFIER(long_back_elevate_1_2)(u16 *restrict write_start, u8 *restrict read_start, Py_ssize_t _len) {
    // only 128 -> 256 and 256 -> 512 should consider aligness.
    // 64(128) -> 128 cannot be aligned anyway.
    usize len = (usize)_len;
    u8 *read_end = read_start + len;
    u16 *write_end = write_start + len;
#if PYYJSON_X86 && COMPILE_SIMD_BITS > 128
    const usize read_once_count = COMPILE_SIMD_BITS / 2 / 8;
    usize tail_len = len & (read_once_count - 1);
    if (tail_len) {
#    if COMPILE_SIMD_BITS == 256
        // read and write with blendv.
        SIMD_128 x;
        SIMD_256 y, mask, blend;
        u16 *const write_tail_start = write_end - read_once_count;
        x = load_128((const void *)(read_end - read_once_count));
        y = elevate_1_2_to_256(x);
        mask = load_256_aligned(read_tail_mask_table_16(read_once_count - tail_len));
        blend = load_256((const void *)write_tail_start);
        y = blendv_256(blend, y, mask);
        write_256((void *)write_tail_start, y);
#    else
        // 512, use mask_storeu.
        SIMD_256 y;
        SIMD_512 z;
        y = load_256((const void *)(read_end - tail_len));
        z = elevate_1_2_to_512(y);
        _mm512_mask_storeu_epi16((void *)(write_end - tail_len), (1 << (usize)tail_len) - 1, z);
#    endif // COMPILE_SIMD_BITS
        len &= ~(read_once_count - 1);
        read_end = read_start + len;
        write_end = write_start + len;
    }
    if (0 == (((usize)(read_start)) & (read_once_count - 1))) {
        if (0 == (((usize)(write_start)) & (read_once_count * 2 - 1))) {
            goto elevate_both_aligned;
        } else {
            goto elevate_src_aligned;
        }
    } else {
        if (0 == (((usize)(write_start)) & (read_once_count * 2 - 1))) {
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
#elif PYYJSON_X86 // COMPILE_SIMD_BITS == 128
    // 16 bytes as a block.
    const usize _BlockSize = 16;
    const usize _SmallBlockSize = 8;
    usize tail_len = len & (usize)(_BlockSize - 1);
    if (tail_len) {
        usize tail_parts = (usize)tail_len / _SmallBlockSize;
        usize small_tail_len = (usize)tail_len & (_SmallBlockSize - 1);
        for (usize i = 0; i < small_tail_len; ++i) {
            *--write_end = *--read_end;
        }
        if (tail_parts) {
            // 64 -> 128.
            _long_back_elevate_1_2_small_tail_1(&read_end, &write_end);
        }
        len &= ~(usize)(_BlockSize - 1);
    }
    read_end -= _BlockSize;
    write_end -= _BlockSize;
    while (read_end >= read_start) {
        vector_a_u8_128 x_read;
        SIMD_128 x;
        x_read = load_128((const void *)read_end);
        x = elevate_1_2_to_128(x_read);
        write_128((void *)write_end, x);
        x_read = unpack_hi_64_128(x_read, x_read);
        x = elevate_1_2_to_128(x_read);
        write_128((void *)(write_end + 8), x);
        read_end -= _BlockSize;
        write_end -= _BlockSize;
    }
#elif PYYJSON_AARCH
    const usize _BlockSize = 8;
    usize tail_len = len & (usize)(_BlockSize - 1);
    if (tail_len) {
        for (usize i = 0; i < tail_len; ++i) {
            *--write_end = *--read_end;
        }
        len &= ~(usize)(_BlockSize - 1);
    }
    read_end -= _BlockSize;
    write_end -= _BlockSize;
    const usize loop_count = (read_end - read_start) / _BlockSize + 1;
    for (usize i = 0; i < loop_count / 2; ++i) {
        vector_a_u8_128 x_read;
        vector_a_u16_256 x;
        memcpy(&x_read, read_end, sizeof(x_read));
        // ushll + ushll2
        for (usize i = 0; i < 16; ++i) {
            x[i] = x_read[i];
        }
        memcpy(write_end, &x, sizeof(x));
        read_end -= _BlockSize * 2;
        write_end -= _BlockSize * 2;
    }
    if (0 != (loop_count & 1)) {
        vector_a_u8_64 x_read;
        vector_a_u16_128 x;
        memcpy(&x_read, read_end, sizeof(x_read));
        for (usize i = 0; i < 8; ++i) {
            x[i] = x_read[i];
        }
        memcpy(write_end, &x, sizeof(x));
        read_end -= _BlockSize;
        write_end -= _BlockSize;
    }
#endif
}

void SIMD_NAME_MODIFIER(long_back_elevate_1_4)(u32 *restrict write_start, u8 *restrict read_start, Py_ssize_t _len) {
    // only 128 -> 512 should consider aligness.
    // 32/64(128) -> 128/256 cannot be aligned anyway.
    usize len = (usize)_len;
    u8 *read_end = read_start + len;
    u32 *write_end = write_start + len;
#if PYYJSON_X86 && COMPILE_SIMD_BITS == 512
    const usize read_once_count = COMPILE_SIMD_BITS / 4 / 8;
    usize tail_len = len & (read_once_count - 1);
    if (tail_len) {
        SIMD_128 x;
        SIMD_512 z;
        x = load_128((const void *)(read_end - tail_len));
        z = elevate_1_4_to_512(x);
        _mm512_mask_storeu_epi32((void *)(write_end - tail_len), ((__mmask16)1 << (usize)tail_len) - 1, z);
        len &= ~(read_once_count - 1);
        read_end = read_start + len;
        write_end = write_start + len;
    }
    if (0 == (((usize)(read_start)) & (128 / 8 - 1))) {
        if (0 == (((usize)(write_start)) & (512 / 8 - 1))) {
            goto elevate_both_aligned;
        } else {
            goto elevate_src_aligned;
        }
    } else {
        if (0 == (((usize)(write_start)) & (512 / 8 - 1))) {
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
#elif PYYJSON_X86
    // 16 bytes as a block.
    const usize _BlockSize = 16;
    const usize _SmallBlockSize = 4;
    usize tail_len = len & (_BlockSize - 1);
    if (tail_len) {
        usize tail_parts = tail_len / _SmallBlockSize;
        usize small_tail_len = tail_len & (_SmallBlockSize - 1);
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

        len &= ~(Py_ssize_t)(_BlockSize - 1);
    }
    read_end -= _BlockSize;
    write_end -= _BlockSize;
    while (read_end >= read_start) {
        SIMD_128 x_read;
        SIMD_TYPE SIMD_VAR;
        x_read = load_128((const void *)read_end);
#    if COMPILE_SIMD_BITS == 256
        SIMD_VAR = elevate_1_4_to_256(x_read);
        write_256((void *)write_end, SIMD_VAR);
        x_read = unpack_hi_64_128(x_read, x_read);
        SIMD_VAR = elevate_1_4_to_256(x_read);
        write_256((void *)(write_end + 8), SIMD_VAR);
#    else
        SIMD_VAR = elevate_1_4_to_128(x_read);
        write_128((void *)write_end, SIMD_VAR);
        RIGHT_SHIFT_128BITS(x_read, 32, &x_read);
        SIMD_VAR = elevate_1_4_to_128(x_read);
        write_128((void *)(write_end + 4), SIMD_VAR);
        RIGHT_SHIFT_128BITS(x_read, 32, &x_read);
        SIMD_VAR = elevate_1_4_to_128(x_read);
        write_128((void *)(write_end + 8), SIMD_VAR);
        RIGHT_SHIFT_128BITS(x_read, 32, &x_read);
        SIMD_VAR = elevate_1_4_to_128(x_read);
        write_128((void *)(write_end + 12), SIMD_VAR);
#    endif
        read_end -= _BlockSize;
        write_end -= _BlockSize;
    }
#elif PYYJSON_AARCH
    // 4 bytes as a block.
    const usize _BlockSize = 4;
    usize tail_len = len & (_BlockSize - 1);
    if (tail_len) {
        for (usize i = 0; i < tail_len; ++i) {
            *--write_end = *--read_end;
        }
        len &= ~(Py_ssize_t)(_BlockSize - 1);
    }
    read_end -= _BlockSize;
    write_end -= _BlockSize;
    const usize loop_count = (read_end - read_start) / _BlockSize + 1;
    for (usize i = 0; i < loop_count / 4; ++i) {
        // elevate_1_4_to_128 in ARM is not very fast.
        // this should be compiled to some tbl instructions, which might be faster
        vector_a_u8_128 x_read;
        memcpy(&x_read, read_end, sizeof(x_read));
        vector_a_u32_512 x;
        for (usize i = 0; i < 16; ++i) {
            x[i] = x_read[i];
        }
        memcpy(write_end, &x, sizeof(x));
        read_end -= _BlockSize * 4;
        write_end -= _BlockSize * 4;
    }
    for (usize i = 0; i < (loop_count & 3); ++i) {
        vector_a_u8_32 x_read;
        vector_a_u32_128 x;
        memcpy(&x_read, read_end, 4);
        x = elevate_1_4_to_128(x_read);
        write_u32_128((void *)write_end, x);
        read_end -= _BlockSize;
        write_end -= _BlockSize;
    }
#endif
}

void SIMD_NAME_MODIFIER(long_back_elevate_2_4)(u32 *restrict write_start, u16 *restrict read_start, Py_ssize_t _len) {
    usize len = (usize)_len;
    // only 128 -> 256 and 256 -> 512 should consider aligness.
    // 64(128) -> 128 cannot be aligned anyway.
    u16 *read_end = read_start + len;
    u32 *write_end = write_start + len;
#if PYYJSON_X86 && COMPILE_SIMD_BITS > 128
    const usize read_once_count = COMPILE_SIMD_BITS / 4 / 8;
    usize tail_len = len & (read_once_count - 1);
    if (tail_len) {
#    if COMPILE_SIMD_BITS == 256
        // read and write with blendv.
        SIMD_128 x;
        SIMD_256 y, mask, blend;
        u32 *const write_tail_start = write_end - read_once_count;
        x = load_128((const void *)(read_end - read_once_count));
        y = elevate_2_4_to_256(x);
        mask = load_256_aligned(read_tail_mask_table_32(read_once_count - tail_len));
        blend = load_256((const void *)write_tail_start);
        y = blendv_256(blend, y, mask);
        write_256((void *)write_tail_start, y);
#    else
        // 512, use mask_storeu.
        SIMD_256 y;
        SIMD_512 z;
        y = load_256((const void *)(read_end - tail_len));
        z = elevate_2_4_to_512(y);
        _mm512_mask_storeu_epi32((void *)(write_end - tail_len), (1 << (usize)tail_len) - 1, z);
#    endif // COMPILE_SIMD_BITS
        len &= ~(read_once_count - 1);
        read_end = read_start + len;
        write_end = write_start + len;
    }
    if (0 == (((usize)(read_start)) & (read_once_count * 2 - 1))) {
        if (0 == (((usize)(write_start)) & (read_once_count * 4 - 1))) {
            goto elevate_both_aligned;
        } else {
            goto elevate_src_aligned;
        }
    } else {
        if (0 == (((usize)(write_start)) & (read_once_count * 4 - 1))) {
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
#elif PYYJSON_X86 // COMPILE_SIMD_BITS == 128
    // 16 bytes as a block. PTR size is 2, so 8 u16 as a block.
    const usize _BlockSize = 8;
    const usize _SmallBlockSize = 4;
    usize tail_len = len & (_BlockSize - 1);
    if (tail_len) {
        usize tail_parts = (usize)tail_len / (_SmallBlockSize);
        usize small_tail_len = (usize)tail_len & (_SmallBlockSize - 1);
        for (usize i = 0; i < small_tail_len; ++i) {
            *--write_end = *--read_end;
        }
        if (tail_parts) {
            // 64 -> 128.
            _long_back_elevate_2_4_small_tail_1(&read_end, &write_end);
        }
        len &= ~(usize)(_BlockSize - 1);
    }
    read_end -= _BlockSize;
    write_end -= _BlockSize;
    while (read_end >= read_start) {
        SIMD_128 x_read;
        SIMD_128 x;
        x_read = load_128((const void *)read_end);
        x = elevate_2_4_to_128(x_read);
        write_128((void *)write_end, x);
        x_read = unpack_hi_64_128(x_read, x_read);
        x = elevate_2_4_to_128(x_read);
        write_128((void *)(write_end + 4), x);
        read_end -= _BlockSize;
        write_end -= _BlockSize;
    }
#elif PYYJSON_AARCH
    // 8 bytes as a block. PTR size is 2, so 4 u16 as a block.
    const usize _BlockSize = 4;
    usize tail_len = len & (_BlockSize - 1);
    if (tail_len) {
        for (usize i = 0; i < tail_len; ++i) {
            *--write_end = *--read_end;
        }
        len &= ~(usize)(_BlockSize - 1);
    }
    read_end -= _BlockSize;
    write_end -= _BlockSize;
    const usize loop_count = (read_end - read_start) / _BlockSize + 1;
    for (usize i = 0; i < loop_count / 2; ++i) {
        vector_a_u16_128 x_read;
        vector_a_u32_256 x;
        memcpy(&x_read, read_end, sizeof(x_read));
        // ushll + ushll2
        for (usize i = 0; i < 8; ++i) {
            x[i] = x_read[i];
        }
        memcpy(write_end, &x, sizeof(x));
        read_end -= _BlockSize * 2;
        write_end -= _BlockSize * 2;
    }
    if (0 != (loop_count & 1)) {
        vector_a_u16_64 x_read;
        vector_a_u32_128 x;
        memcpy(&x_read, read_end, sizeof(x_read));
        for (usize i = 0; i < 4; ++i) {
            x[i] = x_read[i];
        }
        memcpy(write_end, &x, sizeof(x));
        read_end -= _BlockSize;
        write_end -= _BlockSize;
    }
#endif
}
