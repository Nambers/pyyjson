#ifdef PYYJSON_CLANGD_DUMMY
#    ifndef COMPILE_READ_UCS_LEVEL
#        include "decode/decode.h"
#        include "simd/union_vector.h"
//
#        define COMPILE_READ_UCS_LEVEL 1
#        define COMPILE_SIMD_BITS 512
// #        include "simd/compile_feature_check.h"
//
#        include "_r_impls.inl.h"
#    endif
#endif

#include "compile_context/sr_in.inl.h"

force_inline void _decode_str_loop4_read_src_impl(
        const _src_t *src,
        unionvector_a_x4 *out_vec,
        anymask_t *out_check_mask_arr4,
        anymask_t *out_check_mask_total) {
    for (int i = 0; i < 4; ++i) {
        out_vec->x[i] = *(PYYJSON_CAST(vector_u *, src) + i);
    }
    for (int i = 0; i < 4; ++i) {
        out_check_mask_arr4[i] = get_escape_anymask(out_vec->x[i]);
    }
    *out_check_mask_total = (out_check_mask_arr4[0] | out_check_mask_arr4[1]) | (out_check_mask_arr4[2] | out_check_mask_arr4[3]);
}

force_inline void _decode_str_loop_read_src_impl(
        const _src_t *src,
        vector_a *out_vec,
        anymask_t *out_check_mask) {
    *out_vec = *PYYJSON_CAST(vector_u *, src);
    *out_check_mask = get_escape_anymask(*out_vec);
}

force_inline void _decode_str_trailing_read_src_impl(
        const _src_t *src,
        const _src_t *src_end,
        vector_a *out_vec,
        anymask_t *out_check_mask) {
    usize trailing_len = src_end - src;
    assert(trailing_len < READ_BATCH_COUNT);
#if PYYJSON_X86 && COMPILE_SIMD_BITS == 512
    usize maskz = len_to_maskz(src_end - src);
    *out_vec = maskz_loadu(maskz, src);
    *out_check_mask = maskz & get_escape_bitmask(*out_vec);
#elif PYYJSON_X86 && COMPILE_SIMD_BITS == 256
    vector_a vec = *(vector_u *)(src_end - READ_BATCH_COUNT);
    *out_vec = high_mask(vec, trailing_len);
    *out_check_mask = high_mask(get_escape_anymask(vec), trailing_len);
#elif PYYJSON_X86
    vector_a vec = *(vector_u *)(src_end - READ_BATCH_COUNT);
    *out_vec = runtime_byte_rshift_128(vec, (READ_BATCH_COUNT - trailing_len) * sizeof(_src_t));
    *out_check_mask = low_mask(get_escape_anymask(*out_vec), trailing_len);
#elif PYYJSON_AARCH
// TODO
#endif
}

force_inline usize _decode_str_loop4_decoder_impl(
        const _src_t **src_addr,
        const _src_t *src_end,
        anymask_t *check_mask_arr4,
        anymask_t check_mask_total,
        int *ret_addr,
        bool inline_escape, // immediate
        EscapeInfo *escapeval_addr) {
    const _src_t *src = *src_addr;
    usize done_count;
    if (testz_escape_mask(check_mask_total)) {
        src += 4 * READ_BATCH_COUNT;
        done_count = 4 * READ_BATCH_COUNT;
        *ret_addr = DECODE_LOOPSTATE_CONTINUE;
    } else {
        done_count = joined4_escape_anymask_to_done_count(check_mask_arr4[0], check_mask_arr4[1], check_mask_arr4[2], check_mask_arr4[3]);
        src += done_count;
        _src_t unicode = *src;
        if (unicode == _Quote) {
            *ret_addr = DECODE_LOOPSTATE_END;
        } else if (unicode == _Slash) {
            if (inline_escape) { // compile time determined
                *escapeval_addr = do_decode_escape(src, src_end);
            } else {
                *escapeval_addr = do_decode_escape_noinline(src, src_end);
            }
            bool is_invalid = escapeval_addr->escape_val == _DECODE_UNICODE_ERR;
            *ret_addr = DECODE_LOOPSTATE_ESCAPE + is_invalid;
        } else {
            assert(unicode < ControlMax);
            PyErr_SetString(JSONDecodeError, "Invalid control character in string");
            *ret_addr = DECODE_LOOPSTATE_INVALID;
        }
        assert(*ret_addr != DECODE_LOOPSTATE_INVALID || PyErr_Occurred());
    }
    *src_addr = src;
    return done_count;
}

force_inline usize _decode_str_loop_decoder_impl(
        const _src_t **src_addr,
        const _src_t *src_end,
        anymask_t check_mask,
        int *ret_addr,
        bool inline_escape, // immediate
        EscapeInfo *escapeval_addr) {
    usize done_count;
    const _src_t *src = *src_addr;
    if (testz_escape_mask(check_mask)) {
        done_count = READ_BATCH_COUNT;
        src += READ_BATCH_COUNT;
        *ret_addr = DECODE_LOOPSTATE_CONTINUE;
    } else {
        done_count = escape_anymask_to_done_count(check_mask);
        src += done_count;
        _src_t unicode = *src;
        if (unicode == _Quote) {
            *ret_addr = DECODE_LOOPSTATE_END;
        } else if (unicode == _Slash) {
            if (inline_escape) { // compile time determined
                *escapeval_addr = do_decode_escape(src, src_end);
            } else {
                *escapeval_addr = do_decode_escape_noinline(src, src_end);
            }
            bool is_invalid = escapeval_addr->escape_val == _DECODE_UNICODE_ERR;
            *ret_addr = DECODE_LOOPSTATE_ESCAPE + is_invalid;
        } else {
            assert(unicode < ControlMax);
            PyErr_SetString(JSONDecodeError, "Invalid control character in string");
            *ret_addr = DECODE_LOOPSTATE_INVALID;
        }
        assert(*ret_addr != DECODE_LOOPSTATE_INVALID || PyErr_Occurred());
    }
    *src_addr = src;
    return done_count;
}

force_inline usize _decode_str_trailing_decoder_impl(
        const _src_t **src_addr,
        const _src_t *src_end,
        anymask_t check_mask,
        int *ret_addr,
        bool inline_escape, // immediate
        EscapeInfo *escapeval_addr) {
#if PYYJSON_X86 && COMPILE_SIMD_BITS == 256
#    define BLEND 1
#else
#    define BLEND 0
#endif
    usize done_count;
    const _src_t *src = *src_addr;
    usize trailing_len = src_end - src;
    if (unlikely(testz_escape_mask(check_mask))) {
        // err, no string ending in src
        done_count = 0;
        src += READ_BATCH_COUNT;
        PyErr_SetString(JSONDecodeError, "Unexpected ending in string");
        *ret_addr = DECODE_LOOPSTATE_INVALID;
    } else {
        done_count = escape_anymask_to_done_count(check_mask);
        if (BLEND) { // compile time determined
            done_count -= READ_BATCH_COUNT - trailing_len;
        }
        src += done_count;
        _src_t unicode = *src;
        if (unicode == _Quote) {
            *ret_addr = DECODE_LOOPSTATE_END;
        } else if (unicode == _Slash) {
            if (inline_escape) { // compile time determined
                *escapeval_addr = do_decode_escape(src, src_end);
            } else {
                *escapeval_addr = do_decode_escape_noinline(src, src_end);
            }
            bool is_invalid = escapeval_addr->escape_val == _DECODE_UNICODE_ERR;
            *ret_addr = DECODE_LOOPSTATE_ESCAPE + is_invalid;
        } else {
            assert(unicode < ControlMax);
            PyErr_SetString(JSONDecodeError, "Invalid control character in string");
            *ret_addr = DECODE_LOOPSTATE_INVALID;
        }
        assert(*ret_addr != DECODE_LOOPSTATE_INVALID || PyErr_Occurred());
    }
    *src_addr = src;
    return done_count;
#undef BLEND
}

#include "compile_context/sr_out.inl.h"
