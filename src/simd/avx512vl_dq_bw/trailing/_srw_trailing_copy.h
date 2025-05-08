#ifdef PYYJSON_CLANGD_DUMMY
#    include "simd/avx512f_cd/common.h"
#    include "simd/avx512vl_dq_bw/checker.h"
#    include "simd/avx512vl_dq_bw/common.h"
#    include "simd/avx512vl_dq_bw/cvt.h"
#    ifndef COMPILE_READ_UCS_LEVEL
#        define COMPILE_READ_UCS_LEVEL 1
#    endif
#    ifndef COMPILE_WRITE_UCS_LEVEL
#        define COMPILE_WRITE_UCS_LEVEL 1
#    endif
#endif
//
#define COMPILE_SIMD_BITS 512

#include "compile_context/srw_in.inl.h"

extern _dst_t _CONTROL_SEQ_TABLE[(_Slash + 1) * 8];
extern Py_ssize_t _ControlJump[_Slash + 1];

force_inline void trailing_copy_with_cvt(_dst_t **dst_addr, const _src_t *src, usize len) {
    _dst_t *dst = *dst_addr;
    vector_a vec;
    usize maskz = len_to_maskz(len);
    vec = maskz_loadu(maskz, src);
    cvt_to_dst(dst, vec);
    dst += len;
    *dst_addr = dst;
}

force_inline void encode_trailing_copy_with_cvt(_dst_t **dst_addr, const _src_t *src, usize len) {
    _dst_t *dst = *dst_addr;
    vector_a vec;
    usize maskz = len_to_maskz(len);
    vec = maskz_loadu(maskz, src);
    AVX512_BITMASK_TYPE bitmask = get_escape_bitmask(vec);
    bitmask = bitmask & maskz;
restart:;
    cvt_to_dst(dst, vec);
    if (likely(!bitmask)) {
        dst += len;
    } else {
        u32 done_count = escape_bitmask_to_done_count(bitmask);
        const _src_t *escape_pos = src + done_count;
        src += done_count + 1;
        len -= done_count + 1;
        _src_t escape_unicode = *escape_pos;
        assert(escape_unicode == _Quote || escape_unicode == _Slash || escape_unicode < ControlMax);
        dst += done_count;
        memcpy(dst, &_CONTROL_SEQ_TABLE[escape_unicode * 8], 8 * sizeof(_dst_t));
        dst += _ControlJump[escape_unicode];
        if (len) {
            // no need to compute bitmask again
            bitmask = bitmask >> (done_count + 1);
            vec = maskz_loadu(len_to_maskz(len), src);
            goto restart;
        }
    }

    *dst_addr = dst;
}

#undef COMPILE_SIMD_BITS
#include "compile_context/srw_out.inl.h"
