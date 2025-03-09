#ifndef PYYJSON_MEMCMP_H
#define PYYJSON_MEMCMP_H

#include "simd_impl.h"

force_inline bool __memcmp_neq_short(u8 **x_addr, u8 **y_addr, usize *size_addr, usize small_cmp_size) {
    if (*size_addr >= small_cmp_size) {
        if (memcmp(*x_addr, *y_addr, small_cmp_size)) return true;
        *size_addr -= small_cmp_size;
        *x_addr += small_cmp_size;
        *y_addr += small_cmp_size;
    }
    return false;
}

/* Compare memory blocks smaller (or equal to) 64 bytes.
 * Return true if not equal (be compatible with memcmp().) */
force_inline bool pyyjson_memcmp_neq_le64(u8 *x, u8 *y, usize size) {
    assert(size <= 64);
#if SIMD_BIT_SIZE == 512
    if (size == 64) {
        return memcmp(x, y, 64) ? 1 : 0;
    }
#endif
    if (__memcmp_neq_short(&x, &y, &size, 32)) return 1;
    if (__memcmp_neq_short(&x, &y, &size, 32)) return 1;
    if (__memcmp_neq_short(&x, &y, &size, 16)) return 1;
    if (__memcmp_neq_short(&x, &y, &size, 8)) return 1;
    if (__memcmp_neq_short(&x, &y, &size, 4)) return 1;
    if (__memcmp_neq_short(&x, &y, &size, 2)) return 1;
    if (__memcmp_neq_short(&x, &y, &size, 1)) return 1;
    return 0;
}

#endif // PYYJSON_MEMCMP_H
