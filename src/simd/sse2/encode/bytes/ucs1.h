#ifndef PYYJSON_SIMD_SSE2_ENCODE_BYTES_UCS1_H
#define PYYJSON_SIMD_SSE2_ENCODE_BYTES_UCS1_H

#include "simd/simd_detect.h"
#include "simd/vector_types.h"
//
#include "encode/encode_utf8_shared.h"
#include "simd/sse2/checker.h"
#include "simd/sse2/common.h"
//
#define COMPILE_READ_UCS_LEVEL 1
#define COMPILE_SIMD_BITS 128
#include "compile_context/sr_in.inl.h"

force_inline void bytes_write_ucs1_trailing_128(u8 **writer_addr, const u8 *src, usize len) {
    assert(len && len < (READ_BATCH_COUNT));
    const u8 *src_end = src + len;
    const u8 *last_batch_start = src_end - 128 / 8;
    u8 *writer = *writer_addr;

    vector_a vec = *(const vector_u *)last_batch_start;
    vector_a t1 = broadcast(_Quote);
    vector_a t2 = broadcast(_Slash);
    vector_a t3 = broadcast(ControlMax);
    vector_a t4 = broadcast(0x80);
    vector_a m0 = (vec == t1) | (vec == t2) | (vec < t3) | (vec & t4);
restart:;
    vector_a x, m;
    int shift;
    shift = PYYJSON_CAST(int, (128 / 8) - len);
    x = runtime_byte_rshift_128(vec, shift);
    m = runtime_byte_rshift_128(m0, shift);
    *(vector_u *)writer = x;
    if (likely(testz(m))) {
        writer += len;
    } else {
        usize done_count = escape_mask_to_done_count(m);
        assert(done_count < len);
        len -= done_count + 1;
        writer += done_count;
        src += done_count;
        u8 unicode = *src++;
        encode_one_special_ucs1(&writer, unicode);
        if (len) goto restart;
    }
    *writer_addr = writer;
    return;
}

#include "compile_context/sr_out.inl.h"
#undef COMPILE_SIMD_BITS
#undef COMPILE_READ_UCS_LEVEL

#endif // PYYJSON_SIMD_SSE2_ENCODE_BYTES_UCS1_H
