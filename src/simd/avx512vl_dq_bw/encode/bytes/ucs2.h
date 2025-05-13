#ifndef PYYJSON_SIMD_AVX512VLDQBW_ENCODE_BYTES_UCS2_H
#define PYYJSON_SIMD_AVX512VLDQBW_ENCODE_BYTES_UCS2_H

#include "simd/simd_detect.h"
#include "simd/vector_types.h"
//
#include "encode/encode_utf8_shared.h"
//
#define COMPILE_READ_UCS_LEVEL 2
#define COMPILE_WRITE_UCS_LEVEL 1
#define COMPILE_SIMD_BITS 512
#include "compile_context/srw_in.inl.h"

/* 
 * Encode UCS2 trailing to utf-8.
 * Consider 3 types of vector:
 *   vector in ASCII range
 *   vector in 2-bytes range
 *   vector in 3-bytes range
 */
force_inline bool bytes_write_ucs2_trailing_512(u8 **writer_addr, const u16 *src, usize len) {
    return false; // TODO
    //     assert(len && len < READ_BATCH_COUNT);
    //     // constants
    //     const u16 *const src_end = src + len;
    //     const u16 *const last_batch_start = src_end - READ_BATCH_COUNT;
    //     const vector_a vec = *(const vector_u *)last_batch_start;
    //     //
    //     u8 *writer = *writer_addr;
    // restart:;
    //     if (len == 1) {
    //         if (unlikely(!encode_one_ucs2(&writer, *src))) return false;
    //         goto finished;
    //     }
    //     u16 cur_unicode = *src;
    //     bool is_escaped;
    //     int unicode_type = ucs2_get_type(cur_unicode, &is_escaped);
    //     switch (unicode_type) {
    //         case 1: {
    //             if (unlikely(is_escaped)) {
    //                 memcpy(writer, &ControlEscapeTable_u8[cur_unicode * 8], 8);
    //                 writer += _ControlJump[cur_unicode];
    //                 src++;
    //                 len--;
    //                 if (len) goto restart;
    //                 goto finished;
    //             }
    //             goto ascii;
    //         }
    //         case 2: {
    //             goto _2bytes;
    //         }
    //         case 3: {
    //             goto _3bytes;
    //         }
    //         default: {
    //             PYYJSON_UNREACHABLE();
    //         }
    //     }
    //     PYYJSON_UNREACHABLE();
    // ascii:;
    //     {
    //         const vector_a m_not_ascii = (vec == broadcast(_Quote)) | (vec == broadcast(_Slash)) | signed_cmpgt(broadcast(ControlMax), vec) | signed_cmpgt(vec, broadcast(0x7f));
    //         vector_a m = high_mask(m_not_ascii, len);
    //         cvt_to_dst_blendhigh(writer, vec, len);
    //         if (likely(testz(m))) {
    //             writer += len;
    //             goto finished;
    //         } else {
    //             usize done_count = escape_mask_to_done_count_no_eq0(m);
    //             usize real_done_count = done_count - (READ_BATCH_COUNT - len);
    //             assert(real_done_count < len);
    //             u16 escape_unicode = last_batch_start[done_count];
    //             src = last_batch_start + done_count + 1;
    //             writer += real_done_count;
    //             len = READ_BATCH_COUNT - done_count - 1;
    //             if (escape_unicode >= ControlMax && escape_unicode < 0x80 && escape_unicode != _Slash && escape_unicode != _Quote) {
    //                 PYYJSON_UNREACHABLE();
    //             } else {
    //                 if (unlikely(!encode_one_ucs2(&writer, escape_unicode))) return false;
    //             }
    //             if (len) goto restart;
    //             goto finished;
    //         }
    //         PYYJSON_UNREACHABLE();
    //     }
    // _2bytes:;
    //     {
    //         const vector_a m_not_2bytes = signed_cmpgt(broadcast(0x80), vec) | signed_cmpgt(vec, broadcast(0x7ff));
    //         vector_a m = high_mask(m_not_2bytes, len);
    //         ucs2_encode_2bytes_utf8_avx2_blendhigh(writer + len * 2 - READ_BATCH_COUNT * 2, vec, len);
    //         if (likely(testz(m))) {
    //             writer += len * 2;
    //             goto finished;
    //         } else {
    //             usize done_count = escape_mask_to_done_count_no_eq0(m);
    //             usize real_done_count = done_count - (READ_BATCH_COUNT - len);
    //             assert(real_done_count < len);
    //             u16 escape_unicode = last_batch_start[done_count];
    //             src = last_batch_start + done_count + 1;
    //             writer += real_done_count * 2;
    //             len = READ_BATCH_COUNT - done_count - 1;
    //             if (escape_unicode >= 0x80 && escape_unicode <= 0x7ff) {
    //                 PYYJSON_UNREACHABLE();
    //             } else {
    //                 if (unlikely(!encode_one_ucs2(&writer, escape_unicode))) return false;
    //             }
    //             if (len) goto restart;
    //             goto finished;
    //         }
    //         PYYJSON_UNREACHABLE();
    //     }
    // _3bytes:;
    //     {
    //         const vector_a m_not_3bytes = unsigned_saturate_minus(broadcast(0x800), vec) | (signed_cmpgt(vec, broadcast(0xd7ff)) & signed_cmpgt(broadcast(0xe000), vec));
    //         vector_a m = high_mask(m_not_3bytes, len);
    //         ucs2_encode_3bytes_utf8_avx2_blendhigh(writer + len * 3 - READ_BATCH_COUNT * 3, vec, len);
    //         if (likely(testz(m))) {
    //             writer += len * 3;
    //             goto finished;
    //         } else {
    //             // cannot use no_eq0 version
    //             usize done_count = escape_mask_to_done_count(m);
    //             usize real_done_count = done_count - (READ_BATCH_COUNT - len);
    //             assert(real_done_count < len);
    //             u16 escape_unicode = last_batch_start[done_count];
    //             src = last_batch_start + done_count + 1;
    //             writer += real_done_count * 3;
    //             len = READ_BATCH_COUNT - done_count - 1;
    //             if (escape_unicode >= 0x800 && (escape_unicode <= 0xd7ff || escape_unicode >= 0xe000)) {
    //                 PYYJSON_UNREACHABLE();
    //             } else {
    //                 if (unlikely(!encode_one_ucs2(&writer, escape_unicode))) return false;
    //             }
    //             if (len) goto restart;
    //             goto finished;
    //         }
    //         PYYJSON_UNREACHABLE();
    //     }
    // finished:;
    //     *writer_addr = writer;
    //     return true;
}

#include "compile_context/srw_out.inl.h"
#undef COMPILE_SIMD_BITS
#undef COMPILE_WRITE_UCS_LEVEL
#undef COMPILE_READ_UCS_LEVEL

#endif // PYYJSON_SIMD_AVX512VLDQBW_ENCODE_BYTES_UCS2_H
