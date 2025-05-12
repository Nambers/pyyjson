#ifndef PYYJSON_SIMD_SSE2_ENCODE_BYTES_UCS2_H
#define PYYJSON_SIMD_SSE2_ENCODE_BYTES_UCS2_H

#include "simd/simd_detect.h"
#include "simd/vector_types.h"
//
#include "encode/encode_utf8_shared.h"
#include "simd/sse2/checker.h"
#include "simd/sse2/common.h"
#include "simd/sse2/cvt.h"
//
#define COMPILE_READ_UCS_LEVEL 2
#define COMPILE_WRITE_UCS_LEVEL 1
#define COMPILE_SIMD_BITS 128
#include "compile_context/srw_in.inl.h"

#if __SSSE3__
force_inline void ucs2_encode_3bytes_utf8_ssse3(vector_a x, u8 *writer);
#endif

force_inline void ucs2_encode_2bytes_utf8_sse2(vector_a x, u8 *writer) {
    /* abcdefgh|12300000 -> gh123[mmm]|abcdef[mm] */
    /* x1 = gh123000|00000000 */
    vector_a x1 = rshift_u16(x, 6);
    /* x2 = ????????|abcdefgh */
    vector_a x2 = byte_lshift_128(x, 1);
    /* x2 = 00000000|abcdef00 */
    x2 = x2 & broadcast(0x3f00);
    /* y = gh123000|abcdef00 */
    x = x1 | x2;
    /* y = gh123[mmm]|abcdef[mm] */
    x = x | broadcast(0x80c0);
    *(vector_u *)writer = x;
}

/* 
 * Encode UCS2 trailing to utf-8.
 * Consider 3 types of vector:
 *   vector in ASCII range
 *   vector in 2-bytes range
 *   vector in 3-bytes range
 */
force_inline bool bytes_write_ucs2_trailing_128(u8 **writer_addr, const u16 *src, usize len) {
    assert(len && len < READ_BATCH_COUNT);
    const u16 *const src_end = src + len;
    const u16 *const last_batch_start = src_end - READ_BATCH_COUNT;
    const vector_a vec = *(const vector_u *)last_batch_start;
    u8 *writer = *writer_addr;
    //
    vector_a m, tail_vec;
    usize shift;

restart:;
    if (len == 1) {
        if (unlikely(!encode_one_ucs2(&writer, *src))) return false;
        goto finished;
    }
    u16 cur_unicode = *src;
    bool is_escaped;
    int unicode_type = ucs2_get_type(cur_unicode, &is_escaped);
    switch (unicode_type) {
        case 1: {
            if (unlikely(is_escaped)) {
                memcpy(writer, &ControlEscapeTable_u8[cur_unicode * 8], 8);
                writer += _ControlJump[cur_unicode];
                src++;
                len--;
                if (len) goto restart;
                goto finished;
            }
            goto ascii;
        }
        case 2: {
            goto _2bytes;
        }
        case 3: {
#if __SSSE3__
            goto _3bytes;
#else
            if (unlikely(!encode_one_ucs2(&writer, cur_unicode))) return false;
            src++;
            len--;
            if (len) goto restart;
            goto finished;
#endif
        }
        default: {
            PYYJSON_UNREACHABLE();
        }
    }
    PYYJSON_UNREACHABLE();
ascii:;
    {
        const vector_a m_not_ascii = (vec == broadcast(_Quote)) | (vec == broadcast(_Slash)) | unsigned_saturate_minus(broadcast(ControlMax), vec) | unsigned_saturate_minus(vec, broadcast(0x7f));
        m = high_mask(m_not_ascii, len);
        shift = sizeof(u16) * (READ_BATCH_COUNT - len);
        tail_vec = runtime_byte_rshift_128(vec, shift);
        cvt_to_dst(writer, tail_vec);
        if (likely(testz(m))) {
            writer += len;
            goto finished;
        } else {
            usize done_count = escape_mask_to_done_count(m);
            usize real_done_count = done_count - (READ_BATCH_COUNT - len);
            assert(real_done_count < len);
            u16 escape_unicode = last_batch_start[done_count];
            src = last_batch_start + done_count + 1;
            writer += real_done_count;
            len = READ_BATCH_COUNT - done_count - 1;
            if (escape_unicode >= ControlMax && escape_unicode < 0x80 && escape_unicode != _Slash && escape_unicode != _Quote) {
                PYYJSON_UNREACHABLE();
            } else {
                if (unlikely(!encode_one_ucs2(&writer, escape_unicode))) return false;
            }
            if (len) goto restart;
            goto finished;
        }
        PYYJSON_UNREACHABLE();
    }
_2bytes:;
    {
        const vector_a m_not_2bytes = unsigned_saturate_minus(broadcast(0x80), vec) | unsigned_saturate_minus(vec, broadcast(0x7ff));
        m = high_mask(m_not_2bytes, len);
        shift = sizeof(u16) * (READ_BATCH_COUNT - len);
        tail_vec = runtime_byte_rshift_128(vec, shift);
        ucs2_encode_2bytes_utf8_sse2(tail_vec, writer);
        if (likely(testz(m))) {
            writer += len * 2;
            goto finished;
        } else {
            usize done_count = escape_mask_to_done_count(m);
            usize real_done_count = done_count - (READ_BATCH_COUNT - len);
            assert(real_done_count < len);
            u16 escape_unicode = last_batch_start[done_count];
            src = last_batch_start + done_count + 1;
            writer += real_done_count * 2;
            len = READ_BATCH_COUNT - done_count - 1;
            if (escape_unicode >= 0x80 && escape_unicode <= 0x7ff) {
                PYYJSON_UNREACHABLE();
            } else {
                if (unlikely(!encode_one_ucs2(&writer, escape_unicode))) return false;
            }
            if (len) goto restart;
            goto finished;
        }
        PYYJSON_UNREACHABLE();
    }
#if __SSSE3__
_3bytes:;
    {
        const vector_a m_not_3bytes = unsigned_saturate_minus(broadcast(0x800), vec) | (signed_cmpgt(vec, broadcast(0xd7ff)) & signed_cmpgt(broadcast(0xe000), vec));
        m = high_mask(m_not_3bytes, len);
        shift = sizeof(u16) * (READ_BATCH_COUNT - len);
        tail_vec = runtime_byte_rshift_128(vec, shift);
        ucs2_encode_3bytes_utf8_ssse3(tail_vec, writer);
        if (likely(testz(m))) {
            writer += len * 3;
            goto finished;
        } else {
            usize done_count = escape_mask_to_done_count(m);
            usize real_done_count = done_count - (READ_BATCH_COUNT - len);
            assert(real_done_count < len);
            u16 escape_unicode = last_batch_start[done_count];
            src = last_batch_start + done_count + 1;
            writer += real_done_count * 3;
            len = READ_BATCH_COUNT - done_count - 1;
            if (escape_unicode >= 0x800 && (escape_unicode <= 0xd7ff || escape_unicode >= 0xe000)) {
                PYYJSON_UNREACHABLE();
            } else {
                if (unlikely(!encode_one_ucs2(&writer, escape_unicode))) return false;
            }
            if (len) goto restart;
            goto finished;
        }
        PYYJSON_UNREACHABLE();
    }
#endif
//     const u16 *src_end = src + len;
//     const u16 *last_batch_start = src_end - READ_BATCH_COUNT;
//     u8 *writer = *writer_addr;
//     vector_a vec = *(const vector_u *)last_batch_start;
//     const vector_a t1 = broadcast(_Quote);
//     const vector_a t2 = broadcast(_Slash);
//     const vector_a t3 = broadcast(ControlMax);
//     const vector_a t4 = broadcast(0x80);
//     const vector_a t5 = broadcast(0x800);
//     const vector_a t6 = broadcast(0xd800);
//     const vector_a t7 = broadcast(0xdfff);

//     vector_a m_not_ascii = (vec == t1) | (vec == t2) | (vec < t3) | (vec >= t4);
//     vector_a m_not_2bytes = (vec < t4) | (vec >= t5);
//     vector_a m_not_3bytes = (vec < t5) | ((vec >= t6) & (vec <= t7));
//     vector_a m_tmp;
//     vector_a x_tmp;
// restart:;
//     u16 cur_unicode = *src;
//     bool is_escaped;
//     int unicode_type = ucs2_get_type(cur_unicode, &is_escaped);
//     switch (unicode_type) {
//         case 1: {
//             if (likely(!is_escaped)) {
//                 goto restart_ascii;
//             }
//             memcpy(writer, &ControlEscapeTable_u8[cur_unicode * 8], 8);
//             writer += _ControlJump[cur_unicode];
//             src++;
//             len--;
//             if (len) goto restart;
//             goto finished;
//         }
//         case 2: {
//             goto restart_2bytes;
//         }
//         case 3: {
//             if (unlikely(cur_unicode >= 0xd800 && cur_unicode <= 0xdfff)) {
//                 PyErr_SetString(JSONEncodeError, "Cannot encode unicode character in range [0xd800, 0xdfff] to utf-8");
//                 return false;
//             }
// #if __SSSE3__
//             goto restart_3bytes;
// #else
//             *writer++ = (cur_unicode >> 12) | 0xe0;
//             *writer++ = ((cur_unicode & 0xfc0) >> 6) | 0x80;
//             *writer++ = (cur_unicode & 0x3f) | 0x80;
//             if (len) goto restart;
//             goto finished;
// #endif
//         }
//         default: {
//             PYYJSON_UNREACHABLE();
//         }
//     }
// restart_ascii:;
//     {
//         int shift = sizeof(u16) * PYYJSON_CAST(int, READ_BATCH_COUNT - len);
//         x_tmp = runtime_byte_rshift_128(vec, shift);
//         m_tmp = runtime_byte_rshift_128(m_not_ascii, shift);
//     }
//     cvt_to_dst_u16_u8_128(writer, x_tmp);
//     // *(_WVEC_half_U_ *)writer = (_WVEC_half_U_)zip_simd_16_to_8(x_tmp);
//     if (likely(testz(m_tmp))) {
//         writer += len;
//     } else {
//         usize done_count = escape_mask_to_done_count(m_tmp);
//         assert(done_count < len);
//         len -= done_count + 1;
//         writer += done_count;
//         src += done_count;
//         u16 unicode = *src++;
//         if (unlikely(unicode < 128)) {
//             memcpy(writer, &ControlEscapeTable_u8[unicode * 8], 8);
//             writer += _ControlJump[unicode];
//         } else {
//             encode_one_ucs2(&writer, unicode);
//         }
//         if (len) goto restart;
//     }
//     goto finished;

// restart_2bytes:;
//     {
//         int shift = sizeof(u16) * PYYJSON_CAST(int, READ_BATCH_COUNT - len);
//         x_tmp = runtime_byte_rshift_128(vec, shift);
//         m_tmp = runtime_byte_rshift_128(m_not_2bytes, shift);
//     }
//     ucs2_encode_2bytes_utf8_sse2(x_tmp, writer);
//     if (likely(testz(m_tmp))) {
//         writer += 2 * len;
//     } else {
//         usize done_count = escape_mask_to_done_count(m_tmp);
//         assert(done_count < len);
//         len -= done_count + 1;
//         writer += 2 * done_count;
//         src += done_count;
//         u16 unicode = *src++;
//         encode_one_ucs2(&writer, unicode);
//         if (len) goto restart;
//     }
//     goto finished;
// #if __SSSE3__
// restart_3bytes:;
//     {
//         int shift = sizeof(u16) * PYYJSON_CAST(int, READ_BATCH_COUNT - len);
//         x_tmp = runtime_byte_rshift_128(vec, shift);
//         m_tmp = runtime_byte_rshift_128(m_not_3bytes, shift);
//     }
//     ucs2_encode_3bytes_utf8_ssse3(x_tmp, writer);
//     if (likely(testz(m_tmp))) {
//         writer += 3 * len;
//     } else {
//         usize done_count = escape_mask_to_done_count(m_tmp);
//         assert(done_count < len);
//         len -= done_count + 1;
//         writer += 3 * done_count;
//         src += done_count;
//         u16 unicode = *src++;
//         encode_one_ucs2(&writer, unicode);
//         if (len) goto restart;
//     }
//     goto finished;
// #endif
finished:;
    *writer_addr = writer;
    return true;
}

#include "compile_context/srw_out.inl.h"
#undef COMPILE_SIMD_BITS
#undef COMPILE_WRITE_UCS_LEVEL
#undef COMPILE_READ_UCS_LEVEL

#endif // PYYJSON_SIMD_SSE2_ENCODE_BYTES_UCS2_H
