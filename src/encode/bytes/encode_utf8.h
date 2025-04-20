#ifndef PYYJSON_ENCODE_UTF8_H
#define PYYJSON_ENCODE_UTF8_H
#include "pyyjson.h"
#include "simd/simd_impl.h"

#define COMPILE_READ_UCS_LEVEL 1
#include "commondef/r_in.inl.h"

force_inline void encode_one_ucs1(u8 **writer_addr, u8 unicode) {
    u8 *writer = *writer_addr;

    if (unicode >= 128) {
        *writer++ = (unicode & 0x3f) | 0x80;
        *writer++ = (unicode >> 6) | 0xc0;
    } else if (unicode >= ControlMax && unicode != _Quote && unicode != _Slash) {
        *writer++ = unicode;
    } else {
        memcpy(writer, &_control_seq_table_8[unicode * 8], 8);
        writer += _ControlJump[unicode];
    }

    *writer_addr = writer;
}

force_inline void bytes_write_ascii(u8 **writer_addr, const u8 *src, usize len) {
    write_unicode_loopx4_0_1_1(writer_addr, &src, &len);
    write_unicode_loop_0_1_1(writer_addr, &src, &len);
    if (!len) goto done;
    write_unicode_trailing_impl_0_1_1(src, len, writer_addr);
done:;
}

force_inline void check_ascii_in_ucs1_and_get_done_countx4(_VECx4_A_ vec_joined, bool* out_checked, usize* out_done_count) {
    // _VECx4_A_ t1 = SET_ALL(_Quote);
}

force_inline bool ascii_in_ucs1_encode_loop4(u8 **dst_addr, const u8 **src_addr, usize *len_addr) {
    // prepare
    u8 *dst = *dst_addr;
    const u8 *src = *src_addr;
    usize len = *len_addr;
    union {
        _VECx4_A_ joined;
        _VEC_A_ src_vec[4];
    } vec;

    // read
    vec.src_vec[0] = *(_VEC_U_ *)(src + READ_BATCH_COUNT * 0);
    vec.src_vec[1] = *(_VEC_U_ *)(src + READ_BATCH_COUNT * 1);
    vec.src_vec[2] = *(_VEC_U_ *)(src + READ_BATCH_COUNT * 2);
    vec.src_vec[3] = *(_VEC_U_ *)(src + READ_BATCH_COUNT * 3);

    // write
    *(_VEC_U_ *)(dst + READ_BATCH_COUNT * 0) = vec.src_vec[0];
    *(_VEC_U_ *)(dst + READ_BATCH_COUNT * 1) = vec.src_vec[1];
    *(_VEC_U_ *)(dst + READ_BATCH_COUNT * 2) = vec.src_vec[2];
    *(_VEC_U_ *)(dst + READ_BATCH_COUNT * 3) = vec.src_vec[3];

    // check
    bool checked;
    usize done_count;
    check_ascii_in_ucs1_and_get_done_countx4(vec.joined, &checked, &done_count);
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

force_inline bool latin1_in_ucs1_encode_loop4(u8 **dst_addr, const u8 **src_addr, usize *len_addr) {
    // prepare
    u8 *dst = *dst_addr;
    const u8 *src = *src_addr;
    usize len = *len_addr;
    union {
        _VECx4_A_ joined;
        _VEC_A_ src_vec[4];
    } vec;

    // read
    vec.src_vec[0] = *(_VEC_U_ *)(src + READ_BATCH_COUNT * 0);
    vec.src_vec[1] = *(_VEC_U_ *)(src + READ_BATCH_COUNT * 1);
    vec.src_vec[2] = *(_VEC_U_ *)(src + READ_BATCH_COUNT * 2);
    vec.src_vec[3] = *(_VEC_U_ *)(src + READ_BATCH_COUNT * 3);

    // write

}

force_inline void bytes_write_ucs1(u8 **writer_addr, const u8 *src, usize len) {
#define CAN_LOOP4 (len >= 4 * (SIMD_BIT_SIZE / 8 / sizeof(u8)))
    while (CAN_LOOP4) {
        u8 unicode;
        unicode = *src;
        if (unicode < 128) {
        ascii_loop4:;
            bool continuous;
            continuous = ascii_in_ucs1_encode_loop4(writer_addr, &src, &len);
            if (unlikely(!continuous)) {
                goto encode_special4;
            }
            if (CAN_LOOP4) {
                goto ascii_loop4;
            }
            break;
        } else {
        latin1_loop4:;
            bool continuous;
            continuous = latin1_in_ucs1_encode_loop4(writer_addr, &src, &len);
            if (unlikely(!continuous)) {
                goto encode_special4;
            }
            if (CAN_LOOP4) {
                goto latin1_loop4;
            }
            break;
        }
    encode_special4:;
        unicode = *src;
        encode_one_ucs1(writer_addr, unicode);
        src++;
    }
#undef CAN_LOOP4
#define CAN_LOOP (len >= (SIMD_BIT_SIZE / 8 / sizeof(u8)))
    while (CAN_LOOP) {
        u8 unicode;
        unicode = *src;
        if (unicode < 128) {
        ascii_loop:;
            bool continuous;
            continuous = ascii_in_ucs1_encode_loop(writer_addr, &src, &len);
            if (unlikely(!continuous)) {
                goto encode_special;
            }
            if (CAN_LOOP) {
                goto ascii_loop;
            }
            break;
        } else {
        latin1_loop:;
            bool continuous;
            continuous = latin1_in_ucs1_encode_loop(writer_addr, &src, &len);
            if (unlikely(!continuous)) {
                goto encode_special;
            }
            if (CAN_LOOP) {
                goto latin1_loop;
            }
            break;
        }
    encode_special:;
        unicode = *src;
        encode_one_ucs1(writer_addr, unicode);
        src++;
    }
#undef CAN_LOOP
    if (!len) return;
    bytes_write_ucs1_trailing(writer_addr, src, len);
}

force_inline void bytes_write_ucs2(u8 **writer_addr, const u16 *src, usize len) {
}

force_inline void bytes_write_ucs4(u8 **writer_addr, const u32 *src, usize len) {
}

#include "commondef/r_out.inl.h"
#undef COMPILE_READ_UCS_LEVEL
#endif // PYYJSON_ENCODE_UTF8_H
