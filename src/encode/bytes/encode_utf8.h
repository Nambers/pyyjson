#ifndef PYYJSON_ENCODE_UTF8_H
#define PYYJSON_ENCODE_UTF8_H
#include "pyyjson.h"
#include "simd/simd_impl.h"
#include "simd/union_vector.h"
//
#include "writers/write_bytes_ucs1.h"
#include "writers/write_bytes_ucs2.h"
#include "writers/write_bytes_ucs4.h"
//
#include "simd/compile_feature_check.h"
/* write to u8, COMPILE_WRITE_UCS_LEVEL is always 1.*/
#define COMPILE_WRITE_UCS_LEVEL 1
/* ASCII and UCS1: COMPILE_READ_UCS_LEVEL is 1. */
#define COMPILE_READ_UCS_LEVEL 1
#include "compile_context/srw_in.inl.h"

/* ASCII src. */
force_inline void bytes_write_ascii(u8 **writer_addr, const u8 *src, usize len) {
    // reuse the unicode encode loop.
    encode_unicode_loop4(writer_addr, &src, &len);
    encode_unicode_loop(writer_addr, &src, &len);
    if (!len) return;
    encode_trailing_copy_with_cvt(writer_addr, src, len);
}

/* UCS1 src. */
force_inline void encode_one_special_ucs1(u8 **writer_addr, u8 unicode) {
    u8 *writer = *writer_addr;

    if (unicode >= 128) {
        *writer++ = (unicode >> 6) | 0xc0;
        *writer++ = (unicode & 0x3f) | 0x80;
    } else {
        assert(unicode < ControlMax || unicode == _Quote || unicode == _Slash);
        memcpy(writer, &ControlEscapeTable_u8[unicode * 8], 8);
        writer += _ControlJump[unicode];
    }

    *writer_addr = writer;
}

force_inline void encode_one_ucs1(u8 **writer_addr, u8 unicode) {
    if (unicode < 128 && unicode >= ControlMax && unicode != _Quote && unicode != _Slash) {
        *(*writer_addr)++ = unicode;
        return;
    }
    encode_one_special_ucs1(writer_addr, unicode);
}

force_inline void check_ascii_in_ucs1_and_get_done_countx4(unionvector_a_x4 vec, bool *out_checked, usize *out_done_count) {
    vector_a t1 = broadcast(_Quote);
    vector_a t2 = broadcast(_Slash);
    vector_a t3 = broadcast(ControlMax);
    vector_a t4 = broadcast(0x80);
#if PYYJSON_X86 && COMPILE_SIMD_BITS == 512
    struct {
        u64 x[4];
    } m;

    u64 r;

    m.x[0] = cmpeq_bitmask(vec.x[0], t1) |
             cmpeq_bitmask(vec.x[0], t2) |
             unsigned_cmplt_bitmask(vec.x[0], t3) |
             get_bitmask_from(vec.x[0]);
    m.x[1] = cmpeq_bitmask(vec.x[1], t1) |
             cmpeq_bitmask(vec.x[1], t2) |
             unsigned_cmplt_bitmask(vec.x[1], t3) |
             get_bitmask_from(vec.x[1]);
    m.x[2] = cmpeq_bitmask(vec.x[2], t1) |
             cmpeq_bitmask(vec.x[2], t2) |
             unsigned_cmplt_bitmask(vec.x[2], t3) |
             get_bitmask_from(vec.x[2]);
    m.x[3] = cmpeq_bitmask(vec.x[3], t1) |
             cmpeq_bitmask(vec.x[3], t2) |
             unsigned_cmplt_bitmask(vec.x[3], t3) |
             get_bitmask_from(vec.x[3]);
#else
    unionvector_a_x4 m;
    vector_a r;
    m.x[0] = (vec.x[0] == t1) | (vec.x[0] == t2) | (vec.x[0] < t3) | (vec.x[0] & t4);
    m.x[1] = (vec.x[1] == t1) | (vec.x[1] == t2) | (vec.x[1] < t3) | (vec.x[1] & t4);
    m.x[2] = (vec.x[2] == t1) | (vec.x[2] == t2) | (vec.x[2] < t3) | (vec.x[2] & t4);
    m.x[3] = (vec.x[3] == t1) | (vec.x[3] == t2) | (vec.x[3] < t3) | (vec.x[3] & t4);
#endif

    r = m.x[0] | m.x[1];
    r = r | (m.x[2] | m.x[3]);

    if (testz_escape_mask(r)) {
        *out_checked = true;
    } else {
        *out_checked = false;
        usize done_count = 0;
        for (int i = 0; i < 4; ++i) {
            if (testz_escape_mask(m.x[i])) {
                done_count += READ_BATCH_COUNT;
            } else {
                done_count += escape_anymask_to_done_count(m.x[i]);
                break;
            }
        }
        *out_done_count = done_count;
    }
}

force_inline void check_ascii_in_ucs1_and_get_done_count(vector_a vec, bool *out_checked, usize *out_done_count) {
    vector_a t1 = broadcast(_Quote);
    vector_a t2 = broadcast(_Slash);
    vector_a t3 = broadcast(ControlMax);
    vector_a t4 = broadcast(0x80);
#if PYYJSON_X86 && COMPILE_SIMD_BITS == 512
    u64 m;

    m = cmpeq_bitmask(vec, t1) |
        cmpeq_bitmask(vec, t2) |
        unsigned_cmplt_bitmask(vec, t3) |
        get_bitmask_from(vec);
#else
    vector_a m;
    m = (vec == t1) | (vec == t2) | (vec < t3) | (vec & t4);
#endif
    bool checked = testz_escape_mask(m);
    *out_checked = checked;
    if (!checked) {
        *out_done_count = escape_anymask_to_done_count(m);
    }
}

force_inline bool ascii_in_ucs1_encode_loop4(u8 **dst_addr, const u8 **src_addr, usize *len_addr) {
    // prepare
    u8 *dst = *dst_addr;
    const u8 *src = *src_addr;
    usize len = *len_addr;

    unionvector_a_x4 vec;

    // read
    vec.x[0] = *(const vector_u *)(src + READ_BATCH_COUNT * 0);
    vec.x[1] = *(const vector_u *)(src + READ_BATCH_COUNT * 1);
    vec.x[2] = *(const vector_u *)(src + READ_BATCH_COUNT * 2);
    vec.x[3] = *(const vector_u *)(src + READ_BATCH_COUNT * 3);

    // write
    *(vector_u *)(dst + READ_BATCH_COUNT * 0) = vec.x[0];
    *(vector_u *)(dst + READ_BATCH_COUNT * 1) = vec.x[1];
    *(vector_u *)(dst + READ_BATCH_COUNT * 2) = vec.x[2];
    *(vector_u *)(dst + READ_BATCH_COUNT * 3) = vec.x[3];

    // check
    bool checked;
    usize done_count;
    check_ascii_in_ucs1_and_get_done_countx4(vec, &checked, &done_count);

    // update ptr
    if (likely(checked)) {
        dst += 4 * READ_BATCH_COUNT;
        src += 4 * READ_BATCH_COUNT;
        len -= 4 * READ_BATCH_COUNT;
    } else {
        dst += done_count;
        src += done_count;
        len -= done_count;
    }
    *dst_addr = dst;
    *src_addr = src;
    *len_addr = len;
    return checked;
}

force_inline bool ascii_in_ucs1_encode_loop(u8 **dst_addr, const u8 **src_addr, usize *len_addr) {
    // prepare
    u8 *dst = *dst_addr;
    const u8 *src = *src_addr;
    usize len = *len_addr;

    // read
    vector_a vec = *(const vector_u *)src;

    // write
    *(vector_u *)dst = vec;

    // check
    bool checked;
    usize done_count;
    check_ascii_in_ucs1_and_get_done_count(vec, &checked, &done_count);

    // update ptr
    if (likely(checked)) {
        dst += READ_BATCH_COUNT;
        src += READ_BATCH_COUNT;
        len -= READ_BATCH_COUNT;
    } else {
        dst += done_count;
        src += done_count;
        len -= done_count;
    }
    *dst_addr = dst;
    *src_addr = src;
    *len_addr = len;
    return checked;
}

force_inline void bytes_write_ucs1(u8 **writer_addr, const u8 *src, usize len) {
#define CAN_LOOP4 (len >= 4 * READ_BATCH_COUNT)
#define CAN_LOOP (len >= READ_BATCH_COUNT)
    while (CAN_LOOP) {
        u8 unicode;
        unicode = *src;
        if (unicode < 128 && unicode >= ControlMax && unicode != _Quote && unicode != _Slash) {
            bool continuous;
            while (CAN_LOOP4) {
                continuous = ascii_in_ucs1_encode_loop4(writer_addr, &src, &len);
                if (unlikely(!continuous)) {
                    goto encode_one;
                }
            }
            assert(!CAN_LOOP4);
            while (CAN_LOOP) {
                continuous = ascii_in_ucs1_encode_loop(writer_addr, &src, &len);
                if (unlikely(!continuous)) {
                    goto encode_one;
                }
            }
            assert(!CAN_LOOP);
            break;
        } else {
            goto do_encode_one;
        }
    encode_one:;
        unicode = *src;
    do_encode_one:;
        encode_one_ucs1(writer_addr, unicode);
        src++;
        len--;
    }
    if (!len) return;
    bytes_write_ucs1_trailing(writer_addr, src, len);

#undef CAN_LOOP
#undef CAN_LOOP4
}

#include "compile_context/srw_out.inl.h"
#undef COMPILE_WRITE_UCS_LEVEL
#undef COMPILE_READ_UCS_LEVEL

/* UCS2 src. */
#define COMPILE_READ_UCS_LEVEL 2
#define COMPILE_WRITE_UCS_LEVEL 1
#include "compile_context/srw_in.inl.h"

force_inline void check_ascii_in_ucs2_and_get_done_countx4(unionvector_a_x4 vec, bool *out_checked, usize *out_done_count) {
    vector_a t1 = broadcast(_Quote);
    vector_a t2 = broadcast(_Slash);
    vector_a t3 = broadcast(ControlMax);
    vector_a t4 = broadcast(0x80);
#if PYYJSON_X86 && COMPILE_SIMD_BITS == 512
    struct {
        u32 x[4];
    } m;

    u32 r;

    m.x[0] = _mm512_cmpeq_epi16_mask(vec.x[0], t1) |
             _mm512_cmpeq_epi16_mask(vec.x[0], t2) |
             _mm512_cmplt_epu16_mask(vec.x[0], t3) |
             _mm512_cmpge_epu16_mask(vec.x[0], t4);
    m.x[1] = _mm512_cmpeq_epi16_mask(vec.x[1], t1) |
             _mm512_cmpeq_epi16_mask(vec.x[1], t2) |
             _mm512_cmplt_epu16_mask(vec.x[1], t3) |
             _mm512_cmpge_epu16_mask(vec.x[1], t4);
    m.x[2] = _mm512_cmpeq_epi16_mask(vec.x[2], t1) |
             _mm512_cmpeq_epi16_mask(vec.x[2], t2) |
             _mm512_cmplt_epu16_mask(vec.x[2], t3) |
             _mm512_cmpge_epu16_mask(vec.x[2], t4);
    m.x[3] = _mm512_cmpeq_epi16_mask(vec.x[3], t1) |
             _mm512_cmpeq_epi16_mask(vec.x[3], t2) |
             _mm512_cmplt_epu16_mask(vec.x[3], t3) |
             _mm512_cmpge_epu16_mask(vec.x[3], t4);
#else
    unionvector_a_x4 m;
    vector_a r;
    m.x[0] = (vec.x[0] == t1) | (vec.x[0] == t2) | (vec.x[0] < t3) | (vec.x[0] >= t4);
    m.x[1] = (vec.x[1] == t1) | (vec.x[1] == t2) | (vec.x[1] < t3) | (vec.x[1] >= t4);
    m.x[2] = (vec.x[2] == t1) | (vec.x[2] == t2) | (vec.x[2] < t3) | (vec.x[2] >= t4);
    m.x[3] = (vec.x[3] == t1) | (vec.x[3] == t2) | (vec.x[3] < t3) | (vec.x[3] >= t4);
#endif

    r = m.x[0] | m.x[1];
    r = r | (m.x[2] | m.x[3]);

    if (testz_escape_mask(r)) {
        *out_checked = true;
    } else {
        *out_checked = false;
        usize done_count = 0;
        for (int i = 0; i < 4; ++i) {
            if (testz_escape_mask(m.x[i])) {
                done_count += READ_BATCH_COUNT;
            } else {
                done_count += escape_anymask_to_done_count(m.x[i]);
                break;
            }
        }
        *out_done_count = done_count;
    }
}

force_inline void check_ascii_in_ucs2_and_get_done_count(vector_a vec, bool *out_checked, usize *out_done_count) {
    vector_a t1 = broadcast(_Quote);
    vector_a t2 = broadcast(_Slash);
    vector_a t3 = broadcast(ControlMax);
    vector_a t4 = broadcast(0x80);
#if PYYJSON_X86 && COMPILE_SIMD_BITS == 512
    u32 m;

    m = _mm512_cmpeq_epi16_mask(vec, t1) |
        _mm512_cmpeq_epi16_mask(vec, t2) |
        _mm512_cmplt_epu16_mask(vec, t3) |
        _mm512_cmpge_epu16_mask(vec, t4);
#else
    vector_a m;
    m = (vec == t1) | (vec == t2) | (vec < t3) | (vec >= t4);
#endif

    if (testz_escape_mask(m)) {
        *out_checked = true;
    } else {
        *out_checked = false;
        *out_done_count = escape_anymask_to_done_count(m);
    }
}

force_inline bool ascii_in_ucs2_encode_loop4(u8 **dst_addr, const u16 **src_addr, usize *len_addr) {
    // prepare
    u8 *dst = *dst_addr;
    const u16 *src = *src_addr;
    usize len = *len_addr;

    unionvector_a_x4 vec;

    // read
    vec.x[0] = *(const vector_u *)(src + READ_BATCH_COUNT * 0);
    vec.x[1] = *(const vector_u *)(src + READ_BATCH_COUNT * 1);
    vec.x[2] = *(const vector_u *)(src + READ_BATCH_COUNT * 2);
    vec.x[3] = *(const vector_u *)(src + READ_BATCH_COUNT * 3);

    // write
    cvt_to_dst(dst + READ_BATCH_COUNT * 0, vec.x[0]);
    cvt_to_dst(dst + READ_BATCH_COUNT * 1, vec.x[1]);
    cvt_to_dst(dst + READ_BATCH_COUNT * 2, vec.x[2]);
    cvt_to_dst(dst + READ_BATCH_COUNT * 3, vec.x[3]);
    // *(_WVEC_half_U_ *)(dst + READ_BATCH_COUNT * 0) = (_WVEC_half_U_)zip_simd_16_to_8(vec.x[0]);
    // *(_WVEC_half_U_ *)(dst + READ_BATCH_COUNT * 1) = (_WVEC_half_U_)zip_simd_16_to_8(vec.x[1]);
    // *(_WVEC_half_U_ *)(dst + READ_BATCH_COUNT * 2) = (_WVEC_half_U_)zip_simd_16_to_8(vec.x[2]);
    // *(_WVEC_half_U_ *)(dst + READ_BATCH_COUNT * 3) = (_WVEC_half_U_)zip_simd_16_to_8(vec.x[3]);

    // check
    bool checked;
    usize done_count;
    check_ascii_in_ucs2_and_get_done_countx4(vec, &checked, &done_count);

    // update ptr
    if (likely(checked)) {
        dst += 4 * READ_BATCH_COUNT;
        src += 4 * READ_BATCH_COUNT;
        len -= 4 * READ_BATCH_COUNT;
    } else {
        dst += done_count;
        src += done_count;
        len -= done_count;
    }
    *dst_addr = dst;
    *src_addr = src;
    *len_addr = len;
    return checked;
}

force_inline bool ascii_in_ucs2_encode_loop(u8 **dst_addr, const u16 **src_addr, usize *len_addr) {
    // prepare
    u8 *dst = *dst_addr;
    const u16 *src = *src_addr;
    usize len = *len_addr;

    vector_a vec;

    // read
    vec = *(const vector_u *)src;

    // write
    cvt_to_dst(dst, vec);
    // *(_WVEC_half_U_ *)dst = (_WVEC_half_U_)zip_simd_16_to_8(vec);

    // check
    bool checked;
    usize done_count;
    check_ascii_in_ucs2_and_get_done_count(vec, &checked, &done_count);

    // update ptr
    if (likely(checked)) {
        dst += READ_BATCH_COUNT;
        src += READ_BATCH_COUNT;
        len -= READ_BATCH_COUNT;
    } else {
        dst += done_count;
        src += done_count;
        len -= done_count;
    }
    *dst_addr = dst;
    *src_addr = src;
    *len_addr = len;
    return checked;
}

force_inline void check_2bytes_in_ucs2_and_get_done_count(vector_a vec, bool *out_checked, usize *out_done_count) {
    vector_a t1 = broadcast(0x80);
    vector_a t2 = broadcast(0x800);
#if PYYJSON_X86 && COMPILE_SIMD_BITS == 512
    u32 m;
    m = _mm512_cmplt_epu16_mask(vec, t1) | _mm512_cmpge_epu16_mask(vec, t2);
#else
    vector_a m;
    m = (vec < t1) | (vec >= t2);
#endif

    if (testz_escape_mask(m)) {
        *out_checked = true;
    } else {
        *out_checked = false;
        *out_done_count = escape_anymask_to_done_count(m);
    }
}

force_inline bool _2bytes_in_ucs2_encode_loop(u8 **dst_addr, const u16 **src_addr, usize *len_addr) {
    // prepare
    u8 *dst = *dst_addr;
    const u16 *src = *src_addr;
    usize len = *len_addr;

    vector_a vec;

    // read
    vec = *(const vector_u *)src;

    // write
#if PYYJSON_X86
#    if COMPILE_SIMD_BITS == 512
    ucs2_encode_2bytes_utf8_avx512(vec, dst);
#    elif COMPILE_SIMD_BITS == 256
    ucs2_encode_2bytes_utf8_avx2(vec, dst);
#    else
    ucs2_encode_2bytes_utf8_sse2(vec, dst);
#    endif
#else
    // TODO
#endif

    // check
    bool checked;
    usize done_count;
    check_2bytes_in_ucs2_and_get_done_count(vec, &checked, &done_count);

    // update ptr
    if (likely(checked)) {
        dst += READ_BATCH_COUNT * 2;
        src += READ_BATCH_COUNT;
        len -= READ_BATCH_COUNT;
    } else {
        dst += done_count * 2;
        src += done_count;
        len -= done_count;
    }
    *dst_addr = dst;
    *src_addr = src;
    *len_addr = len;
    return checked;
}

force_inline void check_3bytes_in_ucs2_and_get_done_count(vector_a vec, bool *out_checked, usize *out_done_count) {
    vector_a t1 = broadcast(0x800);
    vector_a t2 = broadcast(0xd800);
    vector_a t3 = broadcast(0xdfff);

#if PYYJSON_X86 && COMPILE_SIMD_BITS == 512
    u32 m;

    m = _mm512_cmplt_epu16_mask(vec, t1) |
        _mm512_cmpge_epi16_mask(vec, t2) |
        _mm512_cmple_epu16_mask(vec, t3);
#else
    vector_a m;
    m = (vec < t1) | ((vec >= t2) & (vec <= t3));
#endif

    if (testz_escape_mask(m)) {
        *out_checked = true;
    } else {
        *out_checked = false;
        *out_done_count = escape_anymask_to_done_count(m);
    }
}

force_inline bool _3bytes_in_ucs2_encode_loop(u8 **dst_addr, const u16 **src_addr, usize *len_addr) {
    // prepare
    u8 *dst = *dst_addr;
    const u16 *src = *src_addr;
    usize len = *len_addr;

    vector_a vec;

    // read
    vec = *(const vector_u *)src;

    // write
#if PYYJSON_X86
#    if COMPILE_SIMD_BITS == 512
    ucs2_encode_3bytes_utf8_avx512(vec, dst);
#    elif COMPILE_SIMD_BITS == 256
    ucs2_encode_3bytes_utf8_avx2(vec, dst);
#    elif __SSSE3__
    ucs2_encode_3bytes_utf8_ssse3(vec, dst);
#    else
    assert(false);
    Py_UNREACHABLE();
#    endif
#else
    // TODO
#endif

    // check
    bool checked;
    usize done_count;
    check_3bytes_in_ucs2_and_get_done_count(vec, &checked, &done_count);

    // update ptr
    if (likely(checked)) {
        dst += READ_BATCH_COUNT * 3;
        src += READ_BATCH_COUNT;
        len -= READ_BATCH_COUNT;
    } else {
        dst += done_count * 3;
        src += done_count;
        len -= done_count;
    }
    *dst_addr = dst;
    *src_addr = src;
    *len_addr = len;
    return checked;
}

force_inline bool encode_one_ucs2(u8 **writer_addr, u16 unicode) {
    if (unicode < 128) {
        if (unicode >= ControlMax && unicode != _Slash && unicode != _Quote) {
            *(*writer_addr)++ = unicode;
        } else {
            u8 *writer = *writer_addr;
            memcpy(writer, &ControlEscapeTable_u8[unicode * 8], 8);
            writer += _ControlJump[unicode];
            *writer_addr = writer;
        }
    } else if (unicode < 0x800) {
        // 2 bytes
        u8 *writer = *writer_addr;
        *writer++ = (unicode >> 6) | 0xc0;
        *writer++ = (unicode & 0x3f) | 0x80;
        *writer_addr = writer;
    } else {
        // 3 bytes
        if (unlikely(unicode >= 0xd800 && unicode <= 0xdfff)) {
            PyErr_SetString(JSONEncodeError, "Cannot encode unicode character in range [0xd800, 0xdfff] to utf-8");
            return false;
        }
        u8 *writer = *writer_addr;
        *writer++ = (unicode >> 12) | 0xe0;
        *writer++ = ((unicode & 0xfc0) >> 6) | 0x80;
        *writer++ = (unicode & 0x3f) | 0x80;
        *writer_addr = writer;
    }
    return true;
}

force_inline int ucs2_get_type(u16 unicode, bool *is_escaped) {
    if (unicode < 128) {
        *is_escaped = !(unicode >= ControlMax && unicode != _Slash && unicode != _Quote);
        return 1;
    } else if (unicode < 0x800) {
        return 2;
    }
    return 3;
}

force_inline bool bytes_write_ucs2(u8 **writer_addr, const u16 *src, usize len) {
#define CAN_LOOP4 (len >= READ_BATCH_COUNT)
#define CAN_LOOP (len >= READ_BATCH_COUNT)
    while (CAN_LOOP) {
        u16 unicode;
        unicode = *src;
        if (unicode < 128) {
            // ascii range
            bool continuous;
            while (CAN_LOOP4) {
                continuous = ascii_in_ucs2_encode_loop4(writer_addr, &src, &len);
                if (unlikely(!continuous)) {
                    goto encode_one;
                }
            }
            assert(!CAN_LOOP4);
            while (CAN_LOOP) {
                continuous = ascii_in_ucs2_encode_loop(writer_addr, &src, &len);
                if (unlikely(!continuous)) {
                    goto encode_one;
                }
            }
            assert(!CAN_LOOP);
            break;
        } else if (unicode < 0x800) {
            bool continuous;
            while (CAN_LOOP) {
                continuous = _2bytes_in_ucs2_encode_loop(writer_addr, &src, &len);
                if (unlikely(!continuous)) {
                    goto encode_one;
                }
            }
            assert(!CAN_LOOP);
            break;
        } else {
#if COMPILE_SIMD_BITS >= 256 || __SSSE3__
            bool continuous;
            while (CAN_LOOP) {
                continuous = _3bytes_in_ucs2_encode_loop(writer_addr, &src, &len);
                if (unlikely(!continuous)) {
                    goto encode_one;
                }
            }
            assert(!CAN_LOOP);
            break;
#else
            goto do_encode_one;
#endif
        }
    encode_one:;
        unicode = *src;
    do_encode_one:;
        if (unlikely(!encode_one_ucs2(writer_addr, unicode))) {
            return false;
        }
        src++;
        len--;
    }
    if (!len) return true;
    return bytes_write_ucs2_trailing(writer_addr, src, len);
#undef CAN_LOOP
#undef CAN_LOOP4
}

#include "compile_context/srw_out.inl.h"
#undef COMPILE_WRITE_UCS_LEVEL
#undef COMPILE_READ_UCS_LEVEL

/* UCS4 src. */
#define COMPILE_READ_UCS_LEVEL 4
#define COMPILE_WRITE_UCS_LEVEL 1
#include "compile_context/srw_in.inl.h"

force_inline void bytes_write_ucs4(u8 **writer_addr, const u32 *src, usize len) {
}

#include "compile_context/srw_out.inl.h"
#undef COMPILE_WRITE_UCS_LEVEL
#undef COMPILE_READ_UCS_LEVEL

#endif // PYYJSON_ENCODE_UTF8_H
