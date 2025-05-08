#ifdef PYYJSON_CLANGD_DUMMY
#    include "simd/avx2/checker.h"
#    include "simd/avx2/common.h"
#    include "simd/avx2/cvt.h"
#    include "simd/avx2/trailing.h"
#    ifndef COMPILE_READ_UCS_LEVEL
#        define COMPILE_READ_UCS_LEVEL 1
#    endif
#    ifndef COMPILE_WRITE_UCS_LEVEL
#        define COMPILE_WRITE_UCS_LEVEL 1
#    endif

#endif
//
#define COMPILE_SIMD_BITS 256

#include "compile_context/srw_in.inl.h"

extern _dst_t _CONTROL_SEQ_TABLE[(_Slash + 1) * 8];
extern Py_ssize_t _ControlJump[_Slash + 1];

force_inline void encode_unicode_loop(_dst_t **dst_addr, const _src_t **src_addr, usize *len_addr) {
    register usize len = *len_addr;
    register const _src_t *src = *src_addr;
    register _dst_t *dst = *dst_addr;
    while (len >= READ_BATCH_COUNT) {
        vector_a x = *(vector_u *)src;
        vector_a escape_mask = get_escape_mask(x);
        cvt_to_dst(dst, x);
        if (likely(testz(escape_mask))) {
            src += READ_BATCH_COUNT;
            dst += READ_BATCH_COUNT;
            len -= READ_BATCH_COUNT;
        } else {
            u32 done_count = escape_mask_to_done_count(escape_mask);
            const _src_t *escape_pos = src + done_count;
            src += done_count + 1;
            _src_t escape_unicode = *escape_pos;
            assert(escape_unicode == _Quote || escape_unicode == _Slash || escape_unicode < ControlMax);
            dst += done_count;
            len -= done_count + 1;
            memcpy(dst, &_CONTROL_SEQ_TABLE[escape_unicode * 8], 8 * sizeof(_dst_t));
            dst += _ControlJump[escape_unicode];
        }
    }
    *len_addr = len;
    *src_addr = src;
    *dst_addr = dst;
}

force_inline void encode_unicode_loop4(_dst_t **dst_addr, const _src_t **src_addr, usize *len_addr) {
    register usize len = *len_addr;
    register const _src_t *src = *src_addr;
    register _dst_t *dst = *dst_addr;
    while (len >= READ_BATCH_COUNT * 4) {
        union {
            vector_a x[4];
        } union_vec;

        union {
            vector_a x[4];
        } escape_union_vec;

        memcpy(&union_vec, src, sizeof(union_vec));
        for (usize i = 0; i < 4; ++i) {
            cvt_to_dst(dst + READ_BATCH_COUNT * i, union_vec.x[i]);
            escape_union_vec.x[i] = get_escape_mask(union_vec.x[i]);
        }
        if (likely(testz(escape_union_vec.x[0] | escape_union_vec.x[1] | escape_union_vec.x[2] | escape_union_vec.x[3]))) {
            src += 4 * READ_BATCH_COUNT;
            dst += 4 * READ_BATCH_COUNT;
            len -= 4 * READ_BATCH_COUNT;
        } else {
            usize done_count = joined4_escape_mask_to_done_count(escape_union_vec.x[0], escape_union_vec.x[1], escape_union_vec.x[2], escape_union_vec.x[3]);
            const _src_t *escape_pos = src + done_count;
            src += done_count + 1;
            _src_t escape_unicode = *escape_pos;
            assert(escape_unicode == _Quote || escape_unicode == _Slash || escape_unicode < ControlMax);
            dst += done_count;
            len -= done_count + 1;
            memcpy(dst, &_CONTROL_SEQ_TABLE[escape_unicode * 8], 8 * sizeof(_dst_t));
            dst += _ControlJump[escape_unicode];
        }
    }
    *len_addr = len;
    *src_addr = src;
    *dst_addr = dst;
}

force_inline void encode_unicode_impl(_dst_t **dst_addr, const _src_t *src, usize len) {
    encode_unicode_loop4(dst_addr, &src, &len);
    encode_unicode_loop(dst_addr, &src, &len);
    if (!len) return;
    encode_trailing_copy_with_cvt(dst_addr, src, len);
}

#include "compile_context/srw_out.inl.h"
#undef COMPILE_SIMD_BITS
