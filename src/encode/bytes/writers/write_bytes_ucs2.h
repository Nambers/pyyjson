#ifndef PYYJSON_ENCODE_BYTES_WRITERS_UCS2_H
#define PYYJSON_ENCODE_BYTES_WRITERS_UCS2_H

#ifndef PYYJSON_CLANGD_DUMMY
#    include "pyyjson.h"
#    include "simd/simd_detect.h"
#    include "simd/sse2/checker.h"
#    include "simd/sse2/common.h"
#    include "simd/vector_types.h"
#endif

// forward declaration
force_inline void encode_one_special_ucs1(u8 **writer_addr, u8 unicode);
force_inline int ucs2_get_type(u16 unicode, bool *is_escaped);
force_inline void ucs2_encode_3bytes_utf8_ssse3(vector_a_u16_128 x, u8 *writer);
force_inline bool encode_one_ucs2(u8 **writer_addr, u16 unicode);
force_inline void ucs2_encode_2bytes_utf8_sse2(vector_a_u16_128 x, u8 *writer);
extern PyObject *JSONEncodeError;

#define COMPILE_READ_UCS_LEVEL 2
//
#define COMPILE_SIMD_BITS 128
#include "compile_context/sr_in.inl.h"

force_inline bool bytes_write_ucs2_trailing_128(u8 **writer_addr, const u16 *src, usize len) {
    const u16 *src_end = src + len;
    const u16 *last_batch_start = src_end - READ_BATCH_COUNT;
    u8 *writer = *writer_addr;
    vector_a vec = *(const vector_u *)last_batch_start;
    const vector_a t1 = broadcast(_Quote);
    const vector_a t2 = broadcast(_Slash);
    const vector_a t3 = broadcast(ControlMax);
    const vector_a t4 = broadcast(0x80);
    const vector_a t5 = broadcast(0x800);
    const vector_a t6 = broadcast(0xd800);
    const vector_a t7 = broadcast(0xdfff);

    vector_a m_not_ascii = (vec == t1) | (vec == t2) | (vec < t3) | (vec >= t4);
    vector_a m_not_2bytes = (vec < t4) | (vec >= t5);
    vector_a m_not_3bytes = (vec < t5) | ((vec >= t6) & (vec <= t7));
    vector_a m_tmp;
    vector_a x_tmp;
restart:;
    u16 cur_unicode = *src;
    bool is_escaped;
    int unicode_type = ucs2_get_type(cur_unicode, &is_escaped);
    switch (unicode_type) {
        case 1: {
            if (likely(!is_escaped)) {
                goto restart_ascii;
            }
            memcpy(writer, &ControlEscapeTable_u8[cur_unicode * 8], 8);
            writer += _ControlJump[cur_unicode];
            src++;
            len--;
            if (len) goto restart;
            goto finished;
        }
        case 2: {
            goto restart_2bytes;
        }
        case 3: {
            if (unlikely(cur_unicode >= 0xd800 && cur_unicode <= 0xdfff)) {
                PyErr_SetString(JSONEncodeError, "Cannot encode unicode character in range [0xd800, 0xdfff] to utf-8");
                return false;
            }
#if __SSSE3__
            goto restart_3bytes;
#else
            *writer++ = (cur_unicode >> 12) | 0xe0;
            *writer++ = ((cur_unicode & 0xfc0) >> 6) | 0x80;
            *writer++ = (cur_unicode & 0x3f) | 0x80;
            if (len) goto restart;
            goto finished;
#endif
        }
        default: {
            assert(false);
            Py_UNREACHABLE();
        }
    }
restart_ascii:;
    {
        int shift = sizeof(u16) * PYYJSON_CAST(int, READ_BATCH_COUNT - len);
        x_tmp = runtime_byte_rshift_128(vec, shift);
        m_tmp = runtime_byte_rshift_128(m_not_ascii, shift);
    }
    cvt_to_dst_u16_u8_128(writer, x_tmp);
    // *(_WVEC_half_U_ *)writer = (_WVEC_half_U_)zip_simd_16_to_8(x_tmp);
    if (likely(testz(m_tmp))) {
        writer += len;
    } else {
        usize done_count = escape_mask_to_done_count(m_tmp);
        assert(done_count < len);
        len -= done_count + 1;
        writer += done_count;
        src += done_count;
        u16 unicode = *src++;
        if (unlikely(unicode < 128)) {
            memcpy(writer, &ControlEscapeTable_u8[unicode * 8], 8);
            writer += _ControlJump[unicode];
        } else {
            encode_one_ucs2(&writer, unicode);
        }
        if (len) goto restart;
    }
    goto finished;

restart_2bytes:;
    {
        int shift = sizeof(u16) * PYYJSON_CAST(int, READ_BATCH_COUNT - len);
        x_tmp = runtime_byte_rshift_128(vec, shift);
        m_tmp = runtime_byte_rshift_128(m_not_2bytes, shift);
    }
    ucs2_encode_2bytes_utf8_sse2(x_tmp, writer);
    if (likely(testz(m_tmp))) {
        writer += 2 * len;
    } else {
        usize done_count = escape_mask_to_done_count(m_tmp);
        assert(done_count < len);
        len -= done_count + 1;
        writer += 2 * done_count;
        src += done_count;
        u16 unicode = *src++;
        encode_one_ucs2(&writer, unicode);
        if (len) goto restart;
    }
    goto finished;
#if __SSSE3__
restart_3bytes:;
    {
        int shift = sizeof(u16) * PYYJSON_CAST(int, READ_BATCH_COUNT - len);
        x_tmp = runtime_byte_rshift_128(vec, shift);
        m_tmp = runtime_byte_rshift_128(m_not_3bytes, shift);
    }
    ucs2_encode_3bytes_utf8_ssse3(x_tmp, writer);
    if (likely(testz(m_tmp))) {
        writer += 3 * len;
    } else {
        usize done_count = escape_mask_to_done_count(m_tmp);
        assert(done_count < len);
        len -= done_count + 1;
        writer += 3 * done_count;
        src += done_count;
        u16 unicode = *src++;
        encode_one_ucs2(&writer, unicode);
        if (len) goto restart;
    }
    goto finished;
#endif
finished:;
    *writer_addr = writer;
    return true;
}

#include "compile_context/sr_out.inl.h"
#undef COMPILE_SIMD_BITS

#if SUPPORT_SIMD_256BITS
#    define COMPILE_SIMD_BITS 256
#    include "compile_context/sr_in.inl.h"

force_inline bool bytes_write_ucs2_trailing_256(u8 **writer_addr, const u16 *src, usize len) {
    return false;
}

#    include "compile_context/sr_out.inl.h"
#    undef COMPILE_SIMD_BITS
#endif

#if SUPPORT_SIMD_512BITS
#    define COMPILE_SIMD_BITS 512
#    include "compile_context/sr_in.inl.h"

force_inline bool bytes_write_ucs2_trailing_512(u8 **writer_addr, const u16 *src, usize len) {
    return false;
}

#    include "compile_context/sr_out.inl.h"
#    undef COMPILE_SIMD_BITS
#endif

#undef COMPILE_READ_UCS_LEVEL

#endif // PYYJSON_ENCODE_BYTES_WRITERS_UCS2_H
