#include "encode_shared.h"

#define cpy_inline yyjson_inline

cpy_inline void pyyjson_memcpy_big_first(void *dst, const void *src, usize count) {
    u8 *w = (u8 *) dst;
    const u8 *r = (const u8 *) src;
    usize loop;
#if SIMD_BIT_SIZE >= 512
    loop = count / (512 / 8);
    while (loop) {
        memcpy(w, r, 512 / 8);
        w += 512 / 8;
        r += 512 / 8;
        loop--;
    }
    count &= (512 / 8) - 1;
#endif
#if SIMD_BIT_SIZE >= 256
    loop = count / (256 / 8);
    while (loop) {
        memcpy(w, r, 256 / 8);
        w += 256 / 8;
        r += 256 / 8;
        loop--;
    }
    count &= (256 / 8) - 1;
#endif
    loop = count / (128 / 8);
    while (loop) {
        memcpy(w, r, 128 / 8);
        w += 128 / 8;
        r += 128 / 8;
        loop--;
    }
    count &= (128 / 8) - 1;
    if (count & 8) {
        *w++ = *r++;
        *w++ = *r++;
        *w++ = *r++;
        *w++ = *r++;
        *w++ = *r++;
        *w++ = *r++;
        *w++ = *r++;
        *w++ = *r++;
        count &= ~8;
    }
    if (count & 4) {
        *w++ = *r++;
        *w++ = *r++;
        *w++ = *r++;
        *w++ = *r++;
        count &= ~4;
    }
    if (count & 2) {
        *w++ = *r++;
        *w++ = *r++;
        count &= ~2;
    }
    if (count & 1) {
        *w++ = *r++;
        count &= ~1;
    }
    assert(!count);
}

cpy_inline void pyyjson_memset_small_fisrt(void *dst, u8 fill_val, usize count) {
    u8 *w = (u8 *) dst;
    if (count & 1) {
        *w++ = (u8) fill_val;
        count &= ~1;
    }
    if (count & 2) {
        *w++ = (u8) fill_val;
        *w++ = (u8) fill_val;
        count &= ~2;
    }
    if (count & 4) {
        *w++ = (u8) fill_val;
        *w++ = (u8) fill_val;
        *w++ = (u8) fill_val;
        *w++ = (u8) fill_val;
        count &= ~4;
    }
    if (count & 8) {
        *w++ = (u8) fill_val;
        *w++ = (u8) fill_val;
        *w++ = (u8) fill_val;
        *w++ = (u8) fill_val;
        *w++ = (u8) fill_val;
        *w++ = (u8) fill_val;
        *w++ = (u8) fill_val;
        *w++ = (u8) fill_val;
        count &= ~8;
    }
    usize loop;
#if SIMD_BIT_SIZE == 128
    loop = count / (128 / 8);
    while (loop) {
        memset(w, fill_val, 128 / 8);
        w += 128 / 8;
        loop--;
    }
#elif SIMD_BIT_SIZE == 256
    if (count & 16) {
        memset(w, fill_val, 16);
        w += 16;
        count &= ~16;
    }
    loop = count / (256 / 8);
    while (loop) {
        memset(w, fill_val, 256 / 8);
        w += 256 / 8;
        loop--;
    }
#elif SIMD_BIT_SIZE == 512
    if (count & 16) {
        memset(w, fill_val, 16);
        w += 16;
        count &= ~16;
    }
    if (count & 32) {
        memset(w, fill_val, 32);
        w += 32;
        count &= ~32;
    }
    loop = count / (512 / 8);
    while (loop) {
        memset(w, fill_val, 512 / 8);
        w += 512 / 8;
        loop--;
    }
#endif
}
