#ifdef PYYJSON_CLANGD_DUMMY
#    ifndef COMPILE_UCS_LEVEL
#        include "decoder_impl_wrap.h"
#        include "simd/simd_impl.h"
#        include "simd/union_vector.h"
#        define COMPILE_UCS_LEVEL 1
#        define COMPILE_WRITE_UCS_LEVEL 1
#        include "simd/compile_feature_check.h"
#    endif
#endif

#define COMPILE_READ_UCS_LEVEL COMPILE_UCS_LEVEL
#include "compile_context/srw_in.inl.h"

force_inline int decode_str_copy_loop4(_dst_t **dst_addr, const _src_t **src_addr, const _src_t *src_end, EscapeInfo *escapeval_addr, vector_a *maxvec_addr) {
    int ret;
    //
    unionvector_a_x4 vec;
    anymask_t check_mask[4];
    anymask_t check_mask_total;
    //
    _decode_str_loop4_read_src_impl(*src_addr, &vec, check_mask, &check_mask_total);
    _dst_t *const dst = *dst_addr;
    cvt_to_dst(dst + 0 * READ_BATCH_COUNT, vec.x[0]);
    cvt_to_dst(dst + 1 * READ_BATCH_COUNT, vec.x[1]);
    cvt_to_dst(dst + 2 * READ_BATCH_COUNT, vec.x[2]);
    cvt_to_dst(dst + 3 * READ_BATCH_COUNT, vec.x[3]);
    usize moved_count = _decode_str_loop4_decoder_impl(src_addr, src_end, check_mask, check_mask_total, &ret, true, maxvec_addr, vec, escapeval_addr);
    *dst_addr += moved_count;
    return ret;
}

force_inline int decode_str_copy_loop(_dst_t **dst_addr, const _src_t **src_addr, const _src_t *src_end, EscapeInfo *escapeval_addr, vector_a *maxvec_addr) {
    int ret;
    //
    vector_a vec;
    anymask_t check_mask;
    //
    _decode_str_loop_read_src_impl(*src_addr, &vec, &check_mask);
    _dst_t *const dst = *dst_addr;
    cvt_to_dst(*dst_addr, vec);
    usize moved_count = _decode_str_loop_decoder_impl(src_addr, src_end, check_mask, &ret, true, maxvec_addr, vec, escapeval_addr);
    *dst_addr += moved_count;
    return ret;
}

force_inline int decode_str_copy_trailing(_dst_t **dst_addr, const _src_t **src_addr, const _src_t *src_end, EscapeInfo *escape_info_addr, vector_a *maxvec_addr) {
    int ret;
    //
    vector_a vec;
    anymask_t check_mask;
    //
    _decode_str_trailing_read_src_impl(*src_addr, src_end, &vec, &check_mask);
#if PYYJSON_X86 && COMPILE_SIMD_BITS == 256
    // write with blend
    usize trailing_count = src_end - *src_addr;
    cvt_to_dst_blendhigh((*dst_addr) + trailing_count - READ_BATCH_COUNT, vec, trailing_count);
#else
    cvt_to_dst(*dst_addr, vec);
#endif
    usize done_count = _decode_str_trailing_decoder_impl(src_addr, src_end, check_mask, &ret, false, maxvec_addr, vec, escape_info_addr);
    *dst_addr += done_count;
    //
    return ret;
}

// force_inline int process_escape(
//         EscapeInfo escape_info,
// #if COMPILE_WRITE_UCS_LEVEL <= 1
//         u8 **u8writer_addr,
// #endif
// #if COMPILE_WRITE_UCS_LEVEL <= 2
//         u16 **u16writer_addr,
// #endif
//         u32 **u32writer_addr,
// #if COMPILE_WRITE_UCS_LEVEL <= 2
//         usize *u8size_addr,
// #endif
// #if COMPILE_WRITE_UCS_LEVEL == 2
//         usize *u16size_addr,
// #endif
//         u32 *max_escapeval_addr
// #if COMPILE_WRITE_UCS_LEVEL < 4
//         ,
//         void *temp_buffer
// #endif
// ) {
//     u32 escape_val;
//     usize escape_len;
//     escape_val = escape_info.escape_val;
//     *max_escapeval_addr = PYYJSON_MAX(*max_escapeval_addr, escape_val);
//     assert(escape_val != _DECODE_UNICODE_ERR);
// #if COMPILE_WRITE_UCS_LEVEL <= 1
//     if (escape_val < 0x100) {
//         // R: ucs1 W: ucs1
//         *(*u8writer_addr)++ = (u8)escape_val;
//         return 1;
//     } else
// #endif
// #if COMPILE_WRITE_UCS_LEVEL <= 2
//             if (escape_val < 0x10000) {
//         // R: ucs1,ucs2 W: ucs1,ucs2
// #    if COMPILE_WRITE_UCS_LEVEL == 1
//         usize u8size = (*u8writer_addr) - PYYJSON_CAST(u8 *, temp_buffer);
//         *u8size_addr = u8size;
//         *u8writer_addr = NULL;
//         *u16writer_addr = PYYJSON_CAST(u16 *, temp_buffer) + u8size;
// #    endif
//         *(*u16writer_addr)++ = (u16)escape_val;
//         return 2;
//     } else
// #endif
//     {
//         // R: ucs1,ucs2,ucs4 W: ucs1,ucs2,ucs4
// #if COMPILE_WRITE_UCS_LEVEL == 1
//         // R,W: ucs1
//         usize u8size = (*u8writer_addr) - PYYJSON_CAST(u8 *, temp_buffer);
//         *u8size_addr = u8size;
//         *u8writer_addr = NULL;
//         *u32writer_addr = PYYJSON_CAST(u32 *, temp_buffer) + u8size;
// #elif COMPILE_WRITE_UCS_LEVEL == 2
// #    if COMPILE_READ_UCS_LEVEL == 1
//         // R: ucs1 W: ucs2
//         usize totalsize = (*u16writer_addr) - PYYJSON_CAST(u16 *, temp_buffer);
//         *u16size_addr = totalsize - *u8size_addr;
//         *u16writer_addr = NULL;
//         *u32writer_addr = PYYJSON_CAST(u32 *, temp_buffer) + totalsize;
// #    else
//         // R: ucs2 W: ucs2
//         usize u16size = (*u16writer_addr) - PYYJSON_CAST(u16 *, temp_buffer);
//         *u16size_addr = u16size;
//         *u16writer_addr = NULL;
//         *u32writer_addr = PYYJSON_CAST(u32 *, temp_buffer) + u16size;
// #    endif
// #endif
//         *(*u32writer_addr)++ = escape_val;
//         return 4;
//     }
// }

#include "compile_context/srw_out.inl.h"
#undef COMPILE_READ_UCS_LEVEL
