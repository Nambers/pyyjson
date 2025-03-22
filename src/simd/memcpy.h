/*
 * Code in this file is modified from *gdrcopy*.
 * https://github.com/NVIDIA/gdrcopy
 */

/*
 * Copyright (c) 2014-2021, NVIDIA CORPORATION. All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in 
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */
#ifndef PYYJSON_MEMCPY_H
#define PYYJSON_MEMCPY_H

#include "simd_impl.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define PYYJSON_MEMCPY_MAX_ALIGN 64
#if __AVX512F__
#    define pyyjson_memcpy pyyjson_memcpy_avx512
#    define PYYJSON_MEMCPY_SIMD_SIZE 64
#elif __AVX__
#    define pyyjson_memcpy pyyjson_memcpy_avx
#    define PYYJSON_MEMCPY_SIMD_SIZE 32
#elif __SSE4_1__
#    define pyyjson_memcpy pyyjson_memcpy_sse4_1
#    define PYYJSON_MEMCPY_SIMD_SIZE 16
#else
#    define pyyjson_memcpy pyyjson_memcpy_sse
#    define PYYJSON_MEMCPY_SIMD_SIZE 16
#endif

force_inline void __pyyjson_memcpy(char **dest_addr, const char **src_addr, size_t n_bytes) {
    memcpy((void *)*dest_addr, (const void *)*src_addr, n_bytes);
    *dest_addr += n_bytes;
    *src_addr += n_bytes;
}

force_inline void __pyyjson_short_memcpy_small_first(char **dest_addr, const char **src_addr, size_t n_bytes) {
    assert(n_bytes < PYYJSON_MEMCPY_SIMD_SIZE);
    if (n_bytes & 1) __pyyjson_memcpy(dest_addr, src_addr, 1);
    if (n_bytes & 2) __pyyjson_memcpy(dest_addr, src_addr, 2);
    if (n_bytes & 4) __pyyjson_memcpy(dest_addr, src_addr, 4);
    if (n_bytes & 8) __pyyjson_memcpy(dest_addr, src_addr, 8);
#if PYYJSON_MEMCPY_SIMD_SIZE >= 32
    if (n_bytes & 16) __pyyjson_memcpy(dest_addr, src_addr, 16);
#endif
#if PYYJSON_MEMCPY_SIMD_SIZE >= 64
    if (n_bytes & 32) __pyyjson_memcpy(dest_addr, src_addr, 32);
#endif
}

force_inline void __pyyjson_short_memcpy_large_first(char **dest_addr, const char **src_addr, size_t n_bytes) {
    assert(n_bytes < PYYJSON_MEMCPY_SIMD_SIZE);
#if PYYJSON_MEMCPY_SIMD_SIZE >= 64
    if (n_bytes & 32) __pyyjson_memcpy(dest_addr, src_addr, 32);
#endif
#if PYYJSON_MEMCPY_SIMD_SIZE >= 32
    if (n_bytes & 16) __pyyjson_memcpy(dest_addr, src_addr, 16);
#endif
    if (n_bytes & 8) __pyyjson_memcpy(dest_addr, src_addr, 8);
    if (n_bytes & 4) __pyyjson_memcpy(dest_addr, src_addr, 4);
    if (n_bytes & 2) __pyyjson_memcpy(dest_addr, src_addr, 2);
    if (n_bytes & 1) __pyyjson_memcpy(dest_addr, src_addr, 1);
}

/* Copy memory size smaller than sizeof(SUPPORTED_SIMD_SIZE). */
force_inline void pyyjson_short_memcpy_small_first(void *dst, const void *src, size_t n_bytes) {
    assert(n_bytes < PYYJSON_MEMCPY_SIMD_SIZE);
    char *d = (char *)dst;
    const char *s = (const char *)src;
    __pyyjson_short_memcpy_small_first(&d, &s, n_bytes);
}

/* Copy memory size smaller than sizeof(SUPPORTED_SIMD_SIZE). */
force_inline void pyyjson_short_memcpy_large_first(void *dst, const void *src, size_t n_bytes) {
    assert(n_bytes < PYYJSON_MEMCPY_SIMD_SIZE);
    char *d = (char *)dst;
    const char *s = (const char *)src;
    __pyyjson_short_memcpy_large_first(&d, &s, n_bytes);
}

#if __AVX512F__

force_inline void pyyjson_memcpy_avx512(void *dest, const void *src, size_t n_bytes) {
    char *d = (char *)dest;
    uintptr_t d_int = (uintptr_t)d;
    const char *s = (const char *)src;
    uintptr_t s_int = (uintptr_t)s;
    size_t n = n_bytes;

    // align dest to 512-bits
    if (d_int & 0x3f) {
        size_t tmp = 0x40 - (d_int & 0x3f);
        size_t nh = PYYJSON_MIN(tmp, n);
        __pyyjson_short_memcpy_small_first(&d, &s, nh);
        s_int += nh;
        n -= nh;
    }

    if (s_int & 0x3f) { // src is not aligned to 512-bits
        __m512d r0, r1, r2, r3;
        // unroll 4
        while (n >= 4 * sizeof(__m512d)) {
            r0 = _mm512_loadu_pd((double *)(s + 0 * sizeof(__m512d)));
            r1 = _mm512_loadu_pd((double *)(s + 1 * sizeof(__m512d)));
            r2 = _mm512_loadu_pd((double *)(s + 2 * sizeof(__m512d)));
            r3 = _mm512_loadu_pd((double *)(s + 3 * sizeof(__m512d)));
            _mm512_store_pd((double *)(d + 0 * sizeof(__m512d)), r0);
            _mm512_store_pd((double *)(d + 1 * sizeof(__m512d)), r1);
            _mm512_store_pd((double *)(d + 2 * sizeof(__m512d)), r2);
            _mm512_store_pd((double *)(d + 3 * sizeof(__m512d)), r3);
            s += 4 * sizeof(__m512d);
            d += 4 * sizeof(__m512d);
            n -= 4 * sizeof(__m512d);
        }
        while (n >= sizeof(__m512d)) {
            r0 = _mm512_loadu_pd((double *)(s));
            _mm512_store_pd((double *)(d), r0);
            s += sizeof(__m512d);
            d += sizeof(__m512d);
            n -= sizeof(__m512d);
        }
    } else { // or it IS aligned
        __m512d r0, r1, r2, r3;
        // unroll 4
        while (n >= 4 * sizeof(__m512d)) {
            r0 = _mm512_load_pd((double *)(s + 0 * sizeof(__m512d)));
            r1 = _mm512_load_pd((double *)(s + 1 * sizeof(__m512d)));
            r2 = _mm512_load_pd((double *)(s + 2 * sizeof(__m512d)));
            r3 = _mm512_load_pd((double *)(s + 3 * sizeof(__m512d)));
            _mm512_store_pd((double *)(d + 0 * sizeof(__m512d)), r0);
            _mm512_store_pd((double *)(d + 1 * sizeof(__m512d)), r1);
            _mm512_store_pd((double *)(d + 2 * sizeof(__m512d)), r2);
            _mm512_store_pd((double *)(d + 3 * sizeof(__m512d)), r3);
            s += 4 * sizeof(__m512d);
            d += 4 * sizeof(__m512d);
            n -= 4 * sizeof(__m512d);
        }
        while (n >= sizeof(__m512d)) {
            r0 = _mm512_load_pd((double *)(s));
            _mm512_store_pd((double *)(d), r0);
            s += sizeof(__m512d);
            d += sizeof(__m512d);
            n -= sizeof(__m512d);
        }
    }
    if (n) __pyyjson_short_memcpy_large_first(&d, &s, n);
}
#endif


#ifdef __AVX__
force_inline void pyyjson_memcpy_avx(void *dest, const void *src, size_t n_bytes) {
    char *d = (char *)dest;
    uintptr_t d_int = (uintptr_t)d;
    const char *s = (const char *)src;
    uintptr_t s_int = (uintptr_t)s;
    size_t n = n_bytes;

    // align dest to 256-bits
    if (d_int & 0x1f) {
        size_t tmp = 0x20 - (d_int & 0x1f);
        size_t nh = PYYJSON_MIN(tmp, n);
        __pyyjson_short_memcpy_small_first(&d, &s, nh);
        s_int += nh;
        n -= nh;
    }

    if (s_int & 0x1f) { // src is not aligned to 256-bits
        __m256d r0, r1, r2, r3;
        // unroll 4
        while (n >= 4 * sizeof(__m256d)) {
            r0 = _mm256_loadu_pd((double *)(s + 0 * sizeof(__m256d)));
            r1 = _mm256_loadu_pd((double *)(s + 1 * sizeof(__m256d)));
            r2 = _mm256_loadu_pd((double *)(s + 2 * sizeof(__m256d)));
            r3 = _mm256_loadu_pd((double *)(s + 3 * sizeof(__m256d)));
            _mm256_store_pd((double *)(d + 0 * sizeof(__m256d)), r0);
            _mm256_store_pd((double *)(d + 1 * sizeof(__m256d)), r1);
            _mm256_store_pd((double *)(d + 2 * sizeof(__m256d)), r2);
            _mm256_store_pd((double *)(d + 3 * sizeof(__m256d)), r3);
            s += 4 * sizeof(__m256d);
            d += 4 * sizeof(__m256d);
            n -= 4 * sizeof(__m256d);
        }
        while (n >= sizeof(__m256d)) {
            r0 = _mm256_loadu_pd((double *)(s));
            _mm256_store_pd((double *)(d), r0);
            s += sizeof(__m256d);
            d += sizeof(__m256d);
            n -= sizeof(__m256d);
        }
    } else { // or it IS aligned
        __m256d r0, r1, r2, r3;
        // unroll 4
        while (n >= 4 * sizeof(__m256d)) {
            r0 = _mm256_load_pd((double *)(s + 0 * sizeof(__m256d)));
            r1 = _mm256_load_pd((double *)(s + 1 * sizeof(__m256d)));
            r2 = _mm256_load_pd((double *)(s + 2 * sizeof(__m256d)));
            r3 = _mm256_load_pd((double *)(s + 3 * sizeof(__m256d)));
            _mm256_store_pd((double *)(d + 0 * sizeof(__m256d)), r0);
            _mm256_store_pd((double *)(d + 1 * sizeof(__m256d)), r1);
            _mm256_store_pd((double *)(d + 2 * sizeof(__m256d)), r2);
            _mm256_store_pd((double *)(d + 3 * sizeof(__m256d)), r3);
            s += 4 * sizeof(__m256d);
            d += 4 * sizeof(__m256d);
            n -= 4 * sizeof(__m256d);
        }
        while (n >= sizeof(__m256d)) {
            r0 = _mm256_load_pd((double *)(s));
            _mm256_store_pd((double *)(d), r0);
            s += sizeof(__m256d);
            d += sizeof(__m256d);
            n -= sizeof(__m256d);
        }
    }
    if (n) __pyyjson_short_memcpy_large_first(&d, &s, n);
}
#endif

force_inline void pyyjson_memcpy_sse(void *dest, const void *src, size_t n_bytes) {
    char *d = (char *)dest;
    uintptr_t d_int = (uintptr_t)d;
    const char *s = (const char *)src;
    uintptr_t s_int = (uintptr_t)s;
    size_t n = n_bytes;

    // align dest to 128-bits
    if (d_int & 0xf) {
        size_t tmp = 0x10 - (d_int & 0x0f);
        size_t nh = PYYJSON_MIN(tmp, n);
        __pyyjson_short_memcpy_small_first(&d, &s, nh);
        s_int += nh;
        n -= nh;
    }

    if (s_int & 0xf) { // src is not aligned to 128-bits
        __m128 r0, r1, r2, r3;
        // unroll 4
        while (n >= 4 * 4 * sizeof(float)) {
            r0 = _mm_loadu_ps((float *)(s + 0 * 4 * sizeof(float)));
            r1 = _mm_loadu_ps((float *)(s + 1 * 4 * sizeof(float)));
            r2 = _mm_loadu_ps((float *)(s + 2 * 4 * sizeof(float)));
            r3 = _mm_loadu_ps((float *)(s + 3 * 4 * sizeof(float)));
            _mm_store_ps((float *)(d + 0 * 4 * sizeof(float)), r0);
            _mm_store_ps((float *)(d + 1 * 4 * sizeof(float)), r1);
            _mm_store_ps((float *)(d + 2 * 4 * sizeof(float)), r2);
            _mm_store_ps((float *)(d + 3 * 4 * sizeof(float)), r3);
            s += 4 * 4 * sizeof(float);
            d += 4 * 4 * sizeof(float);
            n -= 4 * 4 * sizeof(float);
        }
        while (n >= 4 * sizeof(float)) {
            r0 = _mm_loadu_ps((float *)(s));
            _mm_store_ps((float *)(d), r0);
            s += 4 * sizeof(float);
            d += 4 * sizeof(float);
            n -= 4 * sizeof(float);
        }
    } else { // or it IS aligned
        __m128 r0, r1, r2, r3;
        // unroll 4
        while (n >= 4 * 4 * sizeof(float)) {
            r0 = _mm_load_ps((float *)(s + 0 * 4 * sizeof(float)));
            r1 = _mm_load_ps((float *)(s + 1 * 4 * sizeof(float)));
            r2 = _mm_load_ps((float *)(s + 2 * 4 * sizeof(float)));
            r3 = _mm_load_ps((float *)(s + 3 * 4 * sizeof(float)));
            _mm_store_ps((float *)(d + 0 * 4 * sizeof(float)), r0);
            _mm_store_ps((float *)(d + 1 * 4 * sizeof(float)), r1);
            _mm_store_ps((float *)(d + 2 * 4 * sizeof(float)), r2);
            _mm_store_ps((float *)(d + 3 * 4 * sizeof(float)), r3);
            s += 4 * 4 * sizeof(float);
            d += 4 * 4 * sizeof(float);
            n -= 4 * 4 * sizeof(float);
        }
        while (n >= 4 * sizeof(float)) {
            r0 = _mm_load_ps((float *)(s));
            _mm_store_ps((float *)(d), r0);
            s += 4 * sizeof(float);
            d += 4 * sizeof(float);
            n -= 4 * sizeof(float);
        }
    }

    if (n) __pyyjson_short_memcpy_large_first(&d, &s, n);
}

#endif // PYYJSON_MEMCPY_H
