#ifdef PYYJSON_CLANGD_DUMMY
#    include "simd/sse2/checker.h"
#    include "simd/sse2/common.h"
#    include "simd/sse2/cvt.h"
#    ifndef COMPILE_READ_UCS_LEVEL
#        define COMPILE_READ_UCS_LEVEL 1
#    endif
#    ifndef COMPILE_WRITE_UCS_LEVEL
#        define COMPILE_WRITE_UCS_LEVEL 1
#    endif
#endif
//
#define COMPILE_SIMD_BITS 128

#include "compile_context/srw_in.inl.h"
extern Py_ssize_t _ControlJump[_Slash + 1];
extern _dst_t _CONTROL_SEQ_TABLE[(_Slash + 1) * 8];

force_inline void trailing_copy_with_cvt(_dst_t **dst_addr, const _src_t *src, usize copy_len) {
    _dst_t *dst = *dst_addr;
    assert(copy_len * sizeof(_src_t) < 16);
    const _src_t *const load_start = src + copy_len - 16 / sizeof(_src_t);
    const vector_a vec = *(vector_u *)load_start;
    vector_a vec_shifted = runtime_byte_rshift_128(vec, 16 - copy_len * sizeof(_src_t));
    cvt_to_dst(dst, vec_shifted);
    dst += copy_len;
    *dst_addr = dst;
}

force_inline void encode_trailing_copy_with_cvt(_dst_t **dst_addr, const _src_t *src, usize copy_len) {
    _dst_t *dst = *dst_addr;
    assert(copy_len * sizeof(_src_t) < 16);
    const _src_t *const load_start = src + copy_len - 16 / sizeof(_src_t);
    const vector_a vec = *(vector_u *)load_start;
    vector_a old_escape_mask = get_escape_mask(vec);
restart:;
    vector_a escape_mask = high_mask(old_escape_mask, copy_len);
    vector_a vec_shifted = runtime_byte_rshift_128(vec, 16 - copy_len * sizeof(_src_t));
    cvt_to_dst(dst, vec_shifted);
    if (likely(testz(escape_mask))) {
        dst += copy_len;
    } else {
        usize done_count = escape_mask_to_done_count(escape_mask);
        usize real_done_count = done_count + copy_len - READ_BATCH_COUNT;
        dst += real_done_count;
        const _src_t *escape_ptr = src + real_done_count;
        src += real_done_count + 1;
        copy_len -= real_done_count + 1;
        usize unicode = *escape_ptr;
        assert(unicode < ControlMax || unicode == _Slash || unicode == _Quote);
        memcpy(dst, _CONTROL_SEQ_TABLE + unicode * 8, 8 * sizeof(_dst_t));
        dst += _ControlJump[unicode];
        if (copy_len) goto restart;
    }
    *dst_addr = dst;
}

#undef COMPILE_SIMD_BITS
#include "compile_context/srw_out.inl.h"
