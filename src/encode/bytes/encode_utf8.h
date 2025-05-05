#ifndef PYYJSON_ENCODE_UTF8_H
#define PYYJSON_ENCODE_UTF8_H
#include "pyyjson.h"
#include "simd/simd_impl.h"
#ifdef PYYJSON_CLANGD_DUMMY
#    ifndef COMPILE_SIMD_BITS
#        define COMPILE_SIMD_BITS 256
#    endif
#endif
/* ASCII src. */
force_inline void bytes_write_ascii(u8 **writer_addr, const u8 *src, usize len) {
    write_unicode_loopx4_0_1_1(writer_addr, &src, &len);
    write_unicode_loop_0_1_1(writer_addr, &src, &len);
    if (!len) goto done;
    write_unicode_trailing_impl_0_1_1(src, len, writer_addr);
done:;
}

/* UCS1 src. */

#define COMPILE_READ_UCS_LEVEL 1
#include "commondef/r_in.inl.h"

force_inline void encode_one_special_ucs1(u8 **writer_addr, u8 unicode) {
    u8 *writer = *writer_addr;

    if (unicode >= 128) {
        *writer++ = (unicode >> 6) | 0xc0;
        *writer++ = (unicode & 0x3f) | 0x80;
    } else {
        assert(unicode < ControlMax || unicode == _Quote || unicode == _Slash);
        memcpy(writer, &_ControlSeqTable_1[unicode * 8], 8);
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

force_inline void check_ascii_in_ucs1_and_get_done_countx4(UNIONVECx4 vec, bool *out_checked, usize *out_done_count) {
    vector_a t1 = SET_ALL(_Quote);
    vector_a t2 = SET_ALL(_Slash);
    vector_a t3 = SET_ALL(ControlMax);
    vector_a t4 = SET_ALL(0x80);
#if PYYJSON_X86 && COMPILE_SIMD_BITS == 512
    struct {
        u64 x[4];
    } m;

    u64 r;

    m.x[0] = _mm512_cmpeq_epi8_mask(vec.x[0], t1) |
             _mm512_cmpeq_epi8_mask(vec.x[0], t2) |
             _mm512_cmplt_epu8_mask(vec.x[0], t3) |
             _mm512_movepi8_mask(vec.x[0]);
    m.x[1] = _mm512_cmpeq_epi8_mask(vec.x[1], t1) |
             _mm512_cmpeq_epi8_mask(vec.x[1], t2) |
             _mm512_cmplt_epu8_mask(vec.x[1], t3) |
             _mm512_movepi8_mask(vec.x[1]);
    m.x[2] = _mm512_cmpeq_epi8_mask(vec.x[2], t1) |
             _mm512_cmpeq_epi8_mask(vec.x[2], t2) |
             _mm512_cmplt_epu8_mask(vec.x[2], t3) |
             _mm512_movepi8_mask(vec.x[2]);
    m.x[3] = _mm512_cmpeq_epi8_mask(vec.x[3], t1) |
             _mm512_cmpeq_epi8_mask(vec.x[3], t2) |
             _mm512_cmplt_epu8_mask(vec.x[3], t3) |
             _mm512_movepi8_mask(vec.x[3]);
#else
    UNIONVECx4 m;
    vector_a r;
    m.x[0] = (vec.x[0] == t1) | (vec.x[0] == t2) | (vec.x[0] < t3) | (vec.x[0] & t4);
    m.x[1] = (vec.x[1] == t1) | (vec.x[1] == t2) | (vec.x[1] < t3) | (vec.x[1] & t4);
    m.x[2] = (vec.x[2] == t1) | (vec.x[2] == t2) | (vec.x[2] < t3) | (vec.x[2] & t4);
    m.x[3] = (vec.x[3] == t1) | (vec.x[3] == t2) | (vec.x[3] < t3) | (vec.x[3] & t4);
#endif

    r = m.x[0] | m.x[1];
    r = r | (m.x[2] | m.x[3]);

    if (check_mask_zero(r)) {
        *out_checked = true;
    } else {
        *out_checked = false;
        usize done_count = 0;
        for (int i = 0; i < 4; ++i) {
            if (check_mask_zero(m.x[i])) {
                done_count += READ_BATCH_COUNT;
            } else {
                done_count += get_done_count_from_mask_1(m.x[i]);
                break;
            }
        }
        *out_done_count = done_count;
    }
}

force_inline void check_ascii_in_ucs1_and_get_done_count(vector_a vec, bool *out_checked, usize *out_done_count) {
    vector_a t1 = SET_ALL(_Quote);
    vector_a t2 = SET_ALL(_Slash);
    vector_a t3 = SET_ALL(ControlMax);
    vector_a t4 = SET_ALL(0x80);
#if PYYJSON_X86 && COMPILE_SIMD_BITS == 512
    u64 m;

    m = _mm512_cmpeq_epi8_mask(vec, t1) |
        _mm512_cmpeq_epi8_mask(vec, t2) |
        _mm512_cmplt_epu8_mask(vec, t3) |
        _mm512_movepi8_mask(vec);
#else
    vector_a m;
    m = (vec == t1) | (vec == t2) | (vec < t3) | (vec & t4);
#endif

    if (check_mask_zero(m)) {
        *out_checked = true;
    } else {
        *out_checked = false;
        *out_done_count = get_done_count_from_mask_1(m);
    }
}

force_inline bool ascii_in_ucs1_encode_loop4(u8 **dst_addr, const u8 **src_addr, usize *len_addr) {
    // prepare
    u8 *dst = *dst_addr;
    const u8 *src = *src_addr;
    usize len = *len_addr;

    UNIONVECx4 vec;

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

force_inline void bytes_write_ucs1_trailing(u8 **writer_addr, const u8 *src, usize len) {
    assert(len && len < (READ_BATCH_COUNT));
    const u8 *src_end = src + len;
    const u8 *last_batch_start = src_end - READ_BATCH_COUNT;
    u8 *writer = *writer_addr;
#if PYYJSON_X86 && COMPILE_SIMD_BITS == 128
    vector_a vec = *(const vector_u *)last_batch_start;
    vector_a t1 = SET_ALL(_Quote);
    vector_a t2 = SET_ALL(_Slash);
    vector_a t3 = SET_ALL(ControlMax);
    vector_a t4 = SET_ALL(0x80);
    vector_a m0 = (vec == t1) | (vec == t2) | (vec < t3) | (vec & t4);
restart:;
    vector_a x, m;
    int shift;
    shift = PYYJSON_CAST(int, (READ_BATCH_COUNT)-len);
    x = runtime_right_shift_128bits(vec, shift);
    m = runtime_right_shift_128bits(m0, shift);
    *(vector_u *)writer = x;
    if (likely(check_mask_zero(m))) {
        writer += len;
    } else {
        usize done_count = get_done_count_from_mask_1(m);
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
#elif PYYJSON_X86 && COMPILE_SIMD_BITS == 256
    unionvector_a_u8_128_x2 vec;
    unionvector_a_u8_128_x2 m0;
    vector_a_u8_256 t1 = SET_ALL(_Quote);
    vector_a_u8_256 t2 = SET_ALL(_Slash);
    vector_a_u8_256 t3 = SET_ALL(ControlMax);
    vector_a_u8_256 t4 = SET_ALL(0x80);
    vec.y = *(const vector_u_u8_256 *)last_batch_start;
    m0.y = (vec.y == t1) | (vec.y == t2) | (vec.y < t3) | (vec.y & t4);
    const usize half_batch = READ_BATCH_COUNT / 2;
    vector_a_u8_128 x, m;
    int shift;
restart:;
    if (len > half_batch) {
        shift = PYYJSON_CAST(int, READ_BATCH_COUNT - len);
        x = runtime_right_shift_128bits(vec.x[0], shift);
        m = runtime_right_shift_128bits(m0.x[0], shift);
        *(vector_u_u8_128 *)writer = x;
        *(vector_u_u8_128 *)(writer + len) = vec.x[1];
        bool check1 = testz_128(m, m);
        bool check2 = testz_128(m0.x[1], m0.x[1]);
        if (likely(check1 && check2)) {
            writer += len;
        } else {
            usize done_count;
            if (check1) {
                done_count = len - half_batch + _get_done_count_from_mask_128_1(m0.x[1]);
            } else {
                done_count = _get_done_count_from_mask_128_1(m);
            }
            assert(done_count < len);
            len -= done_count + 1;
            writer += done_count;
            src += done_count;
            u8 unicode = *src++;
            encode_one_special_ucs1(&writer, unicode);
            if (len) goto restart;
        }
    } else {
        shift = PYYJSON_CAST(int, half_batch - len);
        if (likely(shift)) {
        restart2:;
            x = runtime_right_shift_128bits(vec.x[1], shift);
            m = runtime_right_shift_128bits(m0.x[1], shift);
        } else {
            x = vec.x[1];
            m = m0.x[1];
        }
        *(vector_u_u8_128 *)writer = x;
        if (likely(testz_128(m, m))) {
            writer += len;
        } else {
            usize done_count = _get_done_count_from_mask_128_1(m);
            assert(done_count < len);
            len -= done_count + 1;
            writer += done_count;
            src += done_count;
            u8 unicode = *src++;
            encode_one_special_ucs1(&writer, unicode);
            if (len) {
                shift = PYYJSON_CAST(int, half_batch - len);
                goto restart2;
            }
        }
    }
    *writer_addr = writer;
    return;
#elif PYYJSON_X86 && COMPILE_SIMD_BITS == 512
    // TODO
    vector_a vec = _mm512_maskz_loadu_epi8((PYYJSON_CAST(u64, 1) << len) - 1, src);
    const vector_a t1 = SET_ALL(_Quote);
    const vector_a t2 = SET_ALL(_Slash);
    const vector_a t3 = SET_ALL(ControlMax);
    const vector_a t4 = SET_ALL(0x80);
    u64 m;
    m = _mm512_cmpeq_epi8_mask(vec, t1) |
        _mm512_cmpeq_epi8_mask(vec, t2) |
        _mm512_cmplt_epu8_mask(vec, t3) |
        _mm512_movepi8_mask(vec);
    m = m & ((PYYJSON_CAST(u64, 1) << len) - 1);
restart:;
    *(vector_u *)writer = vec;
    if (likely(m == 0)) {
        writer += len;
    } else {
        usize done_count = _get_done_count_from_mask_512_1(m);
        assert(done_count < len);
        len -= done_count + 1;
        writer += done_count;
        src += done_count;
        u8 unicode = *src++;
        encode_one_special_ucs1(&writer, unicode);
        if (len) {
            m = m >> (done_count + 1);
            vec = _mm512_maskz_loadu_epi8((PYYJSON_CAST(u64, 1) << len) - 1, src);
            goto restart;
        }
    }
    return;
#elif PYYJSON_AARCH
    // TODO
#endif
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

#include "commondef/r_out.inl.h"
#undef COMPILE_READ_UCS_LEVEL

/* UCS2 src. */
#define COMPILE_READ_UCS_LEVEL 2
#include "commondef/r_in.inl.h"
#define COMPILE_WRITE_UCS_LEVEL 1
#include "commondef/w_in.inl.h"

force_inline void check_ascii_in_ucs2_and_get_done_countx4(UNIONVECx4 vec, bool *out_checked, usize *out_done_count) {
    vector_a t1 = SET_ALL(_Quote);
    vector_a t2 = SET_ALL(_Slash);
    vector_a t3 = SET_ALL(ControlMax);
    vector_a t4 = SET_ALL(0x80);
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
    UNIONVECx4 m;
    vector_a r;
    m.x[0] = (vec.x[0] == t1) | (vec.x[0] == t2) | (vec.x[0] < t3) | (vec.x[0] >= t4);
    m.x[1] = (vec.x[1] == t1) | (vec.x[1] == t2) | (vec.x[1] < t3) | (vec.x[1] >= t4);
    m.x[2] = (vec.x[2] == t1) | (vec.x[2] == t2) | (vec.x[2] < t3) | (vec.x[2] >= t4);
    m.x[3] = (vec.x[3] == t1) | (vec.x[3] == t2) | (vec.x[3] < t3) | (vec.x[3] >= t4);
#endif

    r = m.x[0] | m.x[1];
    r = r | (m.x[2] | m.x[3]);

    if (check_mask_zero(r)) {
        *out_checked = true;
    } else {
        *out_checked = false;
        usize done_count = 0;
        for (int i = 0; i < 4; ++i) {
            if (check_mask_zero(m.x[i])) {
                done_count += READ_BATCH_COUNT;
            } else {
                done_count += get_done_count_from_mask_2(m.x[i]);
                break;
            }
        }
        *out_done_count = done_count;
    }
}

force_inline void check_ascii_in_ucs2_and_get_done_count(vector_a vec, bool *out_checked, usize *out_done_count) {
    vector_a t1 = SET_ALL(_Quote);
    vector_a t2 = SET_ALL(_Slash);
    vector_a t3 = SET_ALL(ControlMax);
    vector_a t4 = SET_ALL(0x80);
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

    if (check_mask_zero(m)) {
        *out_checked = true;
    } else {
        *out_checked = false;
        *out_done_count = get_done_count_from_mask_2(m);
    }
}

force_inline bool ascii_in_ucs2_encode_loop4(u8 **dst_addr, const u16 **src_addr, usize *len_addr) {
    // prepare
    u8 *dst = *dst_addr;
    const u16 *src = *src_addr;
    usize len = *len_addr;

    UNIONVECx4 vec;

    // read
    vec.x[0] = *(const vector_u *)(src + READ_BATCH_COUNT * 0);
    vec.x[1] = *(const vector_u *)(src + READ_BATCH_COUNT * 1);
    vec.x[2] = *(const vector_u *)(src + READ_BATCH_COUNT * 2);
    vec.x[3] = *(const vector_u *)(src + READ_BATCH_COUNT * 3);

    // write
    *(_WVEC_half_U_ *)(dst + READ_BATCH_COUNT * 0) = (_WVEC_half_U_)zip_simd_16_to_8(vec.x[0]);
    *(_WVEC_half_U_ *)(dst + READ_BATCH_COUNT * 1) = (_WVEC_half_U_)zip_simd_16_to_8(vec.x[1]);
    *(_WVEC_half_U_ *)(dst + READ_BATCH_COUNT * 2) = (_WVEC_half_U_)zip_simd_16_to_8(vec.x[2]);
    *(_WVEC_half_U_ *)(dst + READ_BATCH_COUNT * 3) = (_WVEC_half_U_)zip_simd_16_to_8(vec.x[3]);

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
    *(_WVEC_half_U_ *)dst = (_WVEC_half_U_)zip_simd_16_to_8(vec);

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
    vector_a t1 = SET_ALL(0x80);
    vector_a t2 = SET_ALL(0x800);
#if PYYJSON_X86 && COMPILE_SIMD_BITS == 512
    u32 m;
    m = _mm512_cmplt_epu16_mask(vec, t1) | _mm512_cmpge_epu16_mask(vec, t2);
#else
    vector_a m;
    m = (vec < t1) | (vec >= t2);
#endif

    if (check_mask_zero(m)) {
        *out_checked = true;
    } else {
        *out_checked = false;
        *out_done_count = get_done_count_from_mask_2(m);
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
    vector_a t1 = SET_ALL(0x800);
    vector_a t2 = SET_ALL(0xd800);
    vector_a t3 = SET_ALL(0xdfff);

#if PYYJSON_X86 && COMPILE_SIMD_BITS == 512
    u32 m;

    m = _mm512_cmplt_epu16_mask(vec, t1) |
        _mm512_cmpge_epi16_mask(vec, t2) |
        _mm512_cmple_epu16_mask(vec, t3);
#else
    vector_a m;
    m = (vec < t1) | ((vec >= t2) & (vec <= t3));
#endif

    if (check_mask_zero(m)) {
        *out_checked = true;
    } else {
        *out_checked = false;
        *out_done_count = get_done_count_from_mask_2(m);
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
            memcpy(writer, &_ControlSeqTable_1[unicode * 8], 8);
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

force_inline bool bytes_write_ucs2_trailing(u8 **writer_addr, const u16 *src, usize len) {
    // TODO, a bit compicated


    assert(len && len < (READ_BATCH_COUNT));
    const u16 *src_end = src + len;
    const u16 *last_batch_start = src_end - READ_BATCH_COUNT;
    u8 *writer = *writer_addr;
#if PYYJSON_X86 && COMPILE_SIMD_BITS == 128
    vector_a vec = *(const vector_u *)last_batch_start;
    const vector_a t1 = SET_ALL(_Quote);
    const vector_a t2 = SET_ALL(_Slash);
    const vector_a t3 = SET_ALL(ControlMax);
    const vector_a t4 = SET_ALL(0x80);
    const vector_a t5 = SET_ALL(0x800);
    const vector_a t6 = SET_ALL(0xd800);
    const vector_a t7 = SET_ALL(0xdfff);

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
            memcpy(writer, &_ControlSeqTable_1[cur_unicode * 8], 8);
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
#    if __SSSE3__
            goto restart_3bytes;
#    else
            *writer++ = (cur_unicode >> 12) | 0xe0;
            *writer++ = ((cur_unicode & 0xfc0) >> 6) | 0x80;
            *writer++ = (cur_unicode & 0x3f) | 0x80;
            if (len) goto restart;
            goto finished;
#    endif
        }
        default: {
            assert(false);
            Py_UNREACHABLE();
        }
    }
restart_ascii:;
    {
        int shift = sizeof(u16) * PYYJSON_CAST(int, READ_BATCH_COUNT - len);
        x_tmp = runtime_right_shift_128bits(vec, shift);
        m_tmp = runtime_right_shift_128bits(m_not_ascii, shift);
    }
    *(_WVEC_half_U_ *)writer = (_WVEC_half_U_)zip_simd_16_to_8(x_tmp);
    if (likely(check_mask_zero(m_tmp))) {
        writer += len;
    } else {
        usize done_count = get_done_count_from_mask_1(m_tmp);
        assert(done_count < len);
        len -= done_count + 1;
        writer += done_count;
        src += done_count;
        u16 unicode = *src++;
        if (unlikely(unicode < 128)) {
            memcpy(writer, &_ControlSeqTable_1[unicode * 8], 8);
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
        x_tmp = runtime_right_shift_128bits(vec, shift);
        m_tmp = runtime_right_shift_128bits(m_not_2bytes, shift);
    }
    ucs2_encode_2bytes_utf8_sse2(x_tmp, writer);
    if (likely(check_mask_zero(m_tmp))) {
        writer += 2 * len;
    } else {
        usize done_count = get_done_count_from_mask_1(m_tmp);
        assert(done_count < len);
        len -= done_count + 1;
        writer += 2 * done_count;
        src += done_count;
        u16 unicode = *src++;
        encode_one_ucs2(&writer, unicode);
        if (len) goto restart;
    }
    goto finished;
#    if __SSSE3__
restart_3bytes:;
    {
        int shift = sizeof(u16) * PYYJSON_CAST(int, READ_BATCH_COUNT - len);
        x_tmp = runtime_right_shift_128bits(vec, shift);
        m_tmp = runtime_right_shift_128bits(m_not_3bytes, shift);
    }
    ucs2_encode_3bytes_utf8_ssse3(x_tmp, writer);
    if (likely(check_mask_zero(m_tmp))) {
        writer += 3 * len;
    } else {
        usize done_count = get_done_count_from_mask_1(m_tmp);
        assert(done_count < len);
        len -= done_count + 1;
        writer += 3 * done_count;
        src += done_count;
        u16 unicode = *src++;
        encode_one_ucs2(&writer, unicode);
        if (len) goto restart;
    }
    goto finished;
#    endif
#elif PYYJSON_X86 && COMPILE_SIMD_BITS == 256
    if (len > READ_BATCH_COUNT / 2) {
    restart_128:;
        {
            u16 unicode = *src;
            if (unicode < 128) {
                if (unicode >= ControlMax && unicode != _Quote && unicode != _Slash) {
                    goto restart_ascii;
                } else {
                    memcpy(writer, &_ControlSeqTable_1[unicode * 8], 8);
                    writer += _ControlJump[unicode];
                    src++;
                    len--;
                    if (len) goto restart_128;
                    goto finished;
                }
            } else if (unicode < 0x800) {
                goto restart_2bytes;
            } else {
                if (unlikely(unicode >= 0xd800 && unicode <= 0xdfff)) {
                    PyErr_SetString(JSONEncodeError, "Cannot encode unicode character in range [0xd800, 0xdfff] to utf-8");
                    return false;
                }
            }
        }
        // prepare
        vector_a_u16_128 vec;

        // read
        vec = *(const vector_u_u16_128 *)src;

        // write

        *(_WVEC_half_U_ *)(dst + READ_BATCH_COUNT * 0) = (_WVEC_half_U_)zip_simd_16_to_8(vec.x[0]);
        *(_WVEC_half_U_ *)(dst + READ_BATCH_COUNT * 1) = (_WVEC_half_U_)zip_simd_16_to_8(vec.x[1]);
        *(_WVEC_half_U_ *)(dst + READ_BATCH_COUNT * 2) = (_WVEC_half_U_)zip_simd_16_to_8(vec.x[2]);
        *(_WVEC_half_U_ *)(dst + READ_BATCH_COUNT * 3) = (_WVEC_half_U_)zip_simd_16_to_8(vec.x[3]);

        // check
        bool checked;
        usize done_count;
        check_ascii_in_ucs2_and_get_done_count(vec, &checked, &done_count);

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
#elif PYYJSON_X86 && COMPILE_SIMD_BITS == 512
//     // TODO
//     vector_a vec = _mm512_maskz_loadu_epi8((PYYJSON_CAST(u64, 1) << len) - 1, src);
//     const vector_a t1 = SET_ALL(_Quote);
//     const vector_a t2 = SET_ALL(_Slash);
//     const vector_a t3 = SET_ALL(ControlMax);
//     const vector_a t4 = SET_ALL(0x80);
//     u64 m;
//     m = _mm512_cmpeq_epi8_mask(vec, t1) |
//         _mm512_cmpeq_epi8_mask(vec, t2) |
//         _mm512_cmplt_epu8_mask(vec, t3) |
//         _mm512_movepi8_mask(vec);
//     m = m & ((PYYJSON_CAST(u64, 1) << len) - 1);
// restart:;
//     *(vector_u *)writer = vec;
//     if (likely(m == 0)) {
//         writer += len;
//     } else {
//         usize done_count = _get_done_count_from_mask_512_1(m);
//         assert(done_count < len);
//         len -= done_count + 1;
//         writer += done_count;
//         src += done_count;
//         u8 unicode = *src++;
//         encode_one_special_ucs1(&writer, unicode);
//         if (len) {
//             m = m >> (done_count + 1);
//             vec = _mm512_maskz_loadu_epi8((PYYJSON_CAST(u64, 1) << len) - 1, src);
//             goto restart;
//         }
//     }
//     return;
#elif PYYJSON_AARCH
    // TODO
#endif
finished:;
    *writer_addr = writer;
    return true;
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

#include "commondef/w_out.inl.h"
#undef COMPILE_WRITE_UCS_LEVEL
#include "commondef/r_out.inl.h"
#undef COMPILE_READ_UCS_LEVEL

/* UCS4 src. */
#define COMPILE_READ_UCS_LEVEL 4
#include "commondef/r_in.inl.h"

force_inline void bytes_write_ucs4(u8 **writer_addr, const u32 *src, usize len) {
}

#include "commondef/r_out.inl.h"
#undef COMPILE_READ_UCS_LEVEL

#endif // PYYJSON_ENCODE_UTF8_H
