#ifdef PYYJSON_CLANGD_DUMMY
#    include "simd/avx2/checker.h"
#    include "simd/avx2/common.h"
#    include "simd/avx2/cvt.h"
#    include "simd/sse2/encode.h"
#    include "simd/sse2/trailing.h"
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

force_inline void trailing_copy_with_cvt(_dst_t **dst_addr, const _src_t *src, usize len) {
    // use 128-bits trailing impl
    if (len >= READ_BATCH_COUNT / 2) {
#define half_vec_t PYYJSON_CONCAT4(vector, a, _src_t, 128)
#define half_vec_u_t PYYJSON_CONCAT4(vector, u, _src_t, 128)
#define half_cvt PYYJSON_CONCAT5(cvt_to, dst, _src_t, _dst_t, 128)
        half_vec_t half_vec = *(half_vec_u_t *)src;
        half_cvt(*dst_addr, half_vec);
        *dst_addr += READ_BATCH_COUNT / 2;
        src += READ_BATCH_COUNT / 2;
        len -= READ_BATCH_COUNT / 2;
        if (!len) return;
#undef half_cvt
#undef half_vec_u_t
#undef half_vec_t
    }
    PYYJSON_CONCAT5(trailing_copy_with, cvt, _src_t, _dst_t, 128)(dst_addr, src, len);
}

force_inline void encode_trailing_copy_with_cvt(_dst_t **dst_addr, const _src_t *src, usize len) {
    assert(len && len < READ_BATCH_COUNT);
    _dst_t *dst_old = *dst_addr;
    _dst_t *dst = *dst_addr;
    const _src_t *src_end = src + len;
    const _src_t *load_start = src_end - READ_BATCH_COUNT;
    const vector_a vec = *(vector_u *)load_start;
    const vector_a escape_mask = get_escape_mask(vec);
restart:;
    _dst_t *write_start = dst + len - READ_BATCH_COUNT;
    vector_a real_escape_mask = high_mask(escape_mask, len);
    cvt_to_dst_blendhigh(write_start, vec, len);
    if (likely(testz(real_escape_mask))) {
        dst += len;
    } else {
        usize done_count = escape_mask_to_done_count(real_escape_mask);
        usize real_done_count = done_count - (src - load_start);
        _src_t unicode = load_start[done_count];
        src = load_start + done_count + 1;
        dst = write_start + done_count;
        len -= real_done_count + 1;
        assert(unicode == _Slash || unicode == _Quote || unicode < ControlMax);
        memcpy(dst, &ControlEscapeTable[unicode * 8], 8 * sizeof(_dst_t));
        dst += _ControlJump[unicode];
        if (len) goto restart;
    }
    assert(dst > dst_old);
    *dst_addr = dst;
}

#undef COMPILE_SIMD_BITS
#include "compile_context/srw_out.inl.h"
