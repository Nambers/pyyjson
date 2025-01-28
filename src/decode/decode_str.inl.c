#include "decode_str_common.h"
#include "pyyjson.h"
#include "simd/cvt.h"
#include "simd/mask_table.h"

#if COMPILE_UCS_LEVEL == 0
#    define COMPILE_READ_UCS_LEVEL 1
#else
#    define COMPILE_READ_UCS_LEVEL COMPILE_UCS_LEVEL
#endif
// #include "encode/encode_simd_utils.inl.h"
#include "simd/simd_impl.h"
//
#include "commondef/r_in.inl.h"
//
// #include "simd/check_mask.inl.h"

#define PYYJSON_DECODE_STR PYYJSON_CONCAT2(pyyjson_decode_str, COMPILE_UCS_LEVEL)
#define READ_STR PYYJSON_CONCAT2(read_str, COMPILE_UCS_LEVEL)
#define FAST_SKIP_SPACES PYYJSON_CONCAT2(fast_skip_spaces, COMPILE_UCS_LEVEL)
#define READ_ROOT PYYJSON_CONCAT2(read_root, COMPILE_UCS_LEVEL)
#define READ_ROOT_SINGLE PYYJSON_CONCAT2(read_root_single, COMPILE_UCS_LEVEL)
#define READ_STR_IN_LOOP PYYJSON_CONCAT2(read_str_in_loop, COMPILE_UCS_LEVEL)
#define PROCESS_TAIL_COPY PYYJSON_CONCAT2(process_tail_copy, COMPILE_UCS_LEVEL)
#define READ_STR_TAIL PYYJSON_CONCAT2(read_str_tail, COMPILE_UCS_LEVEL)
#define READ_TO_HEX_U16 PYYJSON_CONCAT3(read, READ_BIT_SIZE, to_hex_u16)
#define DECODE_UNICODE_INFO PYYJSON_CONCAT2(DecodeUnicodeInfo, COMPILE_UCS_LEVEL)
#define INIT_DECODE_UNICODE_INFO PYYJSON_CONCAT2(init_decode_unicode_info, COMPILE_UCS_LEVEL)
#define UNICODE_DECODE_GET_COPY_COUNT PYYJSON_CONCAT2(unicode_decode_get_copy_count, COMPILE_UCS_LEVEL)
#define DECODE_UNICODE_WRITE_ONE_CHAR PYYJSON_CONCAT2(decode_unicode_write_one_char, COMPILE_UCS_LEVEL)
#define GET_UCS2_WRITER PYYJSON_CONCAT2(get_ucs2_writer, COMPILE_UCS_LEVEL)
#define GET_UCS4_WRITER PYYJSON_CONCAT2(get_ucs4_writer, COMPILE_UCS_LEVEL)
#define GET_CUR_WRITER PYYJSON_CONCAT2(get_cur_writer, COMPILE_UCS_LEVEL)
#define MOVE_WRITER PYYJSON_CONCAT2(move_writer, COMPILE_UCS_LEVEL)
#define CHECK_MAX_CHAR_IN_LOOP PYYJSON_CONCAT2(check_max_char_in_loop, COMPILE_UCS_LEVEL)
#define UPDATE_WRITE_TYPE PYYJSON_CONCAT2(update_write_type, COMPILE_UCS_LEVEL)
#define VERIFY_ESCAPE_HEX PYYJSON_CONCAT2(verify_escape_hex, COMPILE_UCS_LEVEL)
#define DECODE_ESCAPE_UNICODE PYYJSON_CONCAT2(decode_escape_unicode, COMPILE_UCS_LEVEL)
#define DO_SPECIAL PYYJSON_CONCAT2(do_special, COMPILE_UCS_LEVEL)
#define PROCESS_ESCAPE PYYJSON_CONCAT2(process_escape, COMPILE_UCS_LEVEL)
#define DECODE_LOOP_DONE_MAKE_STRING PYYJSON_CONCAT2(decode_loop_done_make_string, COMPILE_UCS_LEVEL)
#define DECODE_SRC_INFO PYYJSON_CONCAT2(DecodeSrcInfo, COMPILE_READ_UCS_LEVEL)
#define CHECK_ESCAPE_IMPL_GET_MASK PYYJSON_CONCAT2(check_escape_impl_get_mask, COMPILE_READ_UCS_LEVEL)
#define GET_DONE_COUNT_FROM_MASK PYYJSON_CONCAT2(get_done_count_from_mask, COMPILE_READ_UCS_LEVEL)
#define WRITE_SIMD_IMPL_TARGET2 PYYJSON_CONCAT3(write_simd_impl, COMPILE_READ_UCS_LEVEL, 2)
#define WRITE_SIMD_IMPL_TARGET4 PYYJSON_CONCAT3(write_simd_impl, COMPILE_READ_UCS_LEVEL, 4)
#define DOWNGRADE_STRING PYYJSON_CONCAT2(downgrade_string, COMPILE_READ_UCS_LEVEL)
#define UCS_BELOW_2_DIRTY PYYJSON_CONCAT2(ucs_below_2_dirty, COMPILE_UCS_LEVEL)
#define UCS_BELOW_4_DIRTY PYYJSON_CONCAT2(ucs_below_4_dirty, COMPILE_UCS_LEVEL)
#define COPY_WITH_ELEVATE_TO_2 PYYJSON_CONCAT2(copy_with_elevate_to_2, COMPILE_UCS_LEVEL)
#define COPY_WITH_ELEVATE_TO_4 PYYJSON_CONCAT2(copy_with_elevate_to_4, COMPILE_UCS_LEVEL)
#define CHECK_AND_RESERVE_STR_BUFFER PYYJSON_CONCAT2(check_and_reserve_str_buffer, COMPILE_UCS_LEVEL)
#define _READ_TRUE PYYJSON_CONCAT2(_read_true, COMPILE_READ_UCS_LEVEL)
#define _READ_FALSE PYYJSON_CONCAT2(_read_false, COMPILE_READ_UCS_LEVEL)
#define _READ_NULL PYYJSON_CONCAT2(_read_null, COMPILE_READ_UCS_LEVEL)
#define _READ_INF PYYJSON_CONCAT2(_read_inf, COMPILE_READ_UCS_LEVEL)
#define _READ_NAN PYYJSON_CONCAT2(_read_nan, COMPILE_READ_UCS_LEVEL)
#define READ_INF_OR_NAN PYYJSON_CONCAT2(read_inf_or_nan, COMPILE_READ_UCS_LEVEL)
#define READ_NUMBER PYYJSON_CONCAT2(read_number, COMPILE_READ_UCS_LEVEL)

force_inline SIMD_MASK_TYPE CHECK_ESCAPE_IMPL_GET_MASK(const _FROM_TYPE *restrict src, SIMD_TYPE *restrict SIMD_VAR);
force_inline u32 GET_DONE_COUNT_FROM_MASK(SIMD_MASK_TYPE mask);
#if COMPILE_READ_UCS_LEVEL <= 2
force_inline void WRITE_SIMD_IMPL_TARGET2(u16 *dst, SIMD_TYPE SIMD_VAR);
#endif
force_inline void WRITE_SIMD_IMPL_TARGET4(u32 *dst, SIMD_TYPE SIMD_VAR);

typedef struct DECODE_UNICODE_INFO {
    void *write_head;
#if COMPILE_UCS_LEVEL <= 1
    u8 *unicode_ucs1;
    Py_ssize_t ucs1_len;
#endif
#if COMPILE_UCS_LEVEL <= 2
    u16 *unicode_ucs2;
    Py_ssize_t ucs2_len;
#endif
    u32 *unicode_ucs4;
    Py_ssize_t ucs4_len;
} DECODE_UNICODE_INFO;

force_inline void INIT_DECODE_UNICODE_INFO(DECODE_UNICODE_INFO *info, u8 *write_head) {
    memset(info, 0, sizeof(DECODE_UNICODE_INFO));
    info->write_head = (void *)write_head;
#if COMPILE_UCS_LEVEL <= 1
    info->unicode_ucs1 = (u8 *)write_head;
#elif COMPILE_UCS_LEVEL <= 2
    info->unicode_ucs2 = (u16 *)write_head;
#else
    info->unicode_ucs4 = (u32 *)write_head;
#endif
}

force_inline Py_ssize_t UNICODE_DECODE_GET_COPY_COUNT(DECODE_UNICODE_INFO *info) {
#define UNICODE_WRITE_PTR_NAME PYYJSON_SIMPLE_CONCAT2(unicode_ucs, COMPILE_READ_UCS_LEVEL)
    assert(info->UNICODE_WRITE_PTR_NAME);
    Py_ssize_t ret = info->UNICODE_WRITE_PTR_NAME - (_FROM_TYPE *)info->write_head;
    assert(ret >= 0);
    return ret;
#undef UNICODE_WRITE_PTR_NAME
}

force_inline void DECODE_UNICODE_WRITE_ONE_CHAR(
        DECODE_UNICODE_INFO *decode_unicode_info,
        int write_as, /* better if known at compile time */
        u32 val /* better if known at compile time */) {
    assert(write_as >= COMPILE_READ_UCS_LEVEL);
#if COMPILE_UCS_LEVEL <= 1
    if (write_as == 1) {
        assert(decode_unicode_info->ucs1_len == 0);
        assert(val <= 0xff);
        *decode_unicode_info->unicode_ucs1++ = (u8)val;
        return;
    }
#endif
#if COMPILE_UCS_LEVEL <= 2
    if (write_as == 2) {
        assert(decode_unicode_info->ucs2_len == 0);
        assert(val <= 0xffff);
        *decode_unicode_info->unicode_ucs2++ = (u16)val;
        return;
    }
#endif
    if (write_as == 4) {
        assert(decode_unicode_info->ucs4_len == 0);
        *decode_unicode_info->unicode_ucs4++ = val;
        return;
    }
    assert(false);
    Py_UNREACHABLE();
}

force_inline u16 *GET_UCS2_WRITER(DECODE_UNICODE_INFO *decode_unicode_info) {
#if COMPILE_UCS_LEVEL <= 2
    return decode_unicode_info->unicode_ucs2;
#else
    return NULL;
#endif
}

force_inline u32 *GET_UCS4_WRITER(DECODE_UNICODE_INFO *decode_unicode_info) {
    return decode_unicode_info->unicode_ucs4;
}

force_inline void *GET_CUR_WRITER(DECODE_UNICODE_INFO *decode_unicode_info) {
#define UNICODE_WRITE_PTR_NAME PYYJSON_SIMPLE_CONCAT2(unicode_ucs, COMPILE_READ_UCS_LEVEL)
    assert(decode_unicode_info->UNICODE_WRITE_PTR_NAME);
    return (void *)decode_unicode_info->UNICODE_WRITE_PTR_NAME;
#undef UNICODE_WRITE_PTR_NAME
}

force_inline void MOVE_WRITER(DECODE_UNICODE_INFO *decode_unicode_info, int write_as, Py_ssize_t len) {
    if (write_as == 1) {
#if COMPILE_UCS_LEVEL <= 1
        decode_unicode_info->unicode_ucs1 += len;
        return;
#endif
    }
    if (write_as == 2) {
#if COMPILE_UCS_LEVEL <= 2
        decode_unicode_info->unicode_ucs2 += len;
        return;
#endif
    }
    if (write_as == 4) {
        decode_unicode_info->unicode_ucs4 += len;
        return;
    }
    assert(false);
    Py_UNREACHABLE();
}

force_inline bool UCS_BELOW_2_DIRTY(DECODE_UNICODE_INFO *decode_unicode_info) {
#if COMPILE_UCS_LEVEL < 2
    return decode_unicode_info->ucs1_len > 0;
#else
    return false;
#endif
}

force_inline bool UCS_BELOW_4_DIRTY(DECODE_UNICODE_INFO *decode_unicode_info) {
#if COMPILE_UCS_LEVEL < 4
    return
#    if COMPILE_UCS_LEVEL <= 1
            decode_unicode_info->ucs1_len > 0 ||
#    endif
            decode_unicode_info->ucs2_len > 0;
#else
    return false;
#endif
}

force_inline void COPY_WITH_ELEVATE_TO_2(DECODE_UNICODE_INFO *decode_unicode_info) {
#if COMPILE_UCS_LEVEL < 2
    assert(decode_unicode_info->ucs1_len > 0);
    long_back_elevate_1_2((u16 *)decode_unicode_info->write_head, (u8 *)decode_unicode_info->write_head, decode_unicode_info->ucs1_len);
#else
    assert(false);
    Py_UNREACHABLE();
#endif
}

force_inline void COPY_WITH_ELEVATE_TO_4(DECODE_UNICODE_INFO *decode_unicode_info) {
#if COMPILE_UCS_LEVEL < 2
    if (decode_unicode_info->ucs2_len > 0) {
        u16 *head_u16 = (u16 *)decode_unicode_info->write_head + decode_unicode_info->ucs1_len;
        long_back_elevate_2_4(((u32 *)decode_unicode_info->write_head) + decode_unicode_info->ucs1_len, head_u16, decode_unicode_info->ucs2_len);
    }
    if (decode_unicode_info->ucs1_len > 0) {
        long_back_elevate_1_4((u32 *)decode_unicode_info->write_head, (u8 *)decode_unicode_info->write_head, decode_unicode_info->ucs1_len);
    }
#elif COMPILE_UCS_LEVEL == 2
    assert(decode_unicode_info->ucs2_len > 0);
    long_back_elevate_2_4((u32 *)decode_unicode_info->write_head, (u16 *)decode_unicode_info->write_head, decode_unicode_info->ucs2_len);
#else
    assert(false);
    Py_UNREACHABLE();
#endif
}

// #if COMPILE_UCS_LEVEL == PYYJSON_STRING_TYPE_UCS4
// force_inline bool _checkmax_ucs4(SIMD_TYPE SIMD_VAR, ReadStrState *read_state) {
// #    if SIMD_BIT_SIZE == 512
//     SIMD_512 t = _mm512_set1_epi32(65535);
//     __mmask16 mask = _mm512_cmpgt_epu32_mask(SIMD_VAR, t);
//     if (unlikely(mask != 0)) {
//         update_max_char_type(read_state, PYYJSON_STRING_TYPE_UCS4);
//         return false;
//     }
//     return true;
// #    else
// #        define CMPGT PYYJSON_CONCAT2(cmpgt_i32, SIMD_BIT_SIZE)
// #        define BROADCAST PYYJSON_CONCAT2(broadcast_32, SIMD_BIT_SIZE)
//     SIMD_TYPE t = BROADCAST(65535);
//     SIMD_TYPE mask = CMPGT(SIMD_VAR, t);
//     if (unlikely(!check_mask_zero(mask))) {
//         update_max_char_type(read_state, PYYJSON_STRING_TYPE_UCS4);
//         return false;
//     }
//     return true;
// #        undef BROADCAST
// #        undef CMPGT
// #    endif
// }
// #endif

force_inline void CHECK_MAX_CHAR_IN_LOOP(
        SIMD_TYPE SIMD_VAR,
        ReadStrState *restrict read_state,
        bool need_mask, /* known at compile time */
        Py_ssize_t index /* only used when need_mask */) {
#if COMPILE_UCS_LEVEL == PYYJSON_STRING_TYPE_ASCII
    return;
#endif
    if (read_state->max_char_type == PYYJSON_STRING_TYPE_UCS4) {
        assert(false); // logic error
    }
    if (need_mask) {
#define LOAD_HEAD_MASK PYYJSON_CONCAT2(read_head_mask_table, READ_BIT_SIZE)
        // need a mask
        const void *mask_addr = LOAD_HEAD_MASK(index);
        SIMD_VAR = SIMD_AND(load_simd(mask_addr), SIMD_VAR);
#undef LOAD_HEAD_MASK
    }
#define CHECKER PYYJSON_SIMPLE_CONCAT3(_ucs, COMPILE_READ_UCS_LEVEL, _checkmax)
    switch (read_state->max_char_type) {
        case PYYJSON_STRING_TYPE_ASCII: { // TODO
#if COMPILE_UCS_LEVEL == PYYJSON_STRING_TYPE_UCS4
            CHECKER(SIMD_VAR, read_state, 0xffff, PYYJSON_STRING_TYPE_UCS4) &&
#endif
#if COMPILE_UCS_LEVEL >= PYYJSON_STRING_TYPE_UCS2
                    CHECKER(SIMD_VAR, read_state, 0xff, PYYJSON_STRING_TYPE_UCS2) &&
#endif
                    CHECKER(SIMD_VAR, read_state, 0x7f, PYYJSON_STRING_TYPE_LATIN1);
            break;
        }
#if COMPILE_UCS_LEVEL > PYYJSON_STRING_TYPE_LATIN1
        case PYYJSON_STRING_TYPE_LATIN1: { // TODO
#    if COMPILE_UCS_LEVEL == PYYJSON_STRING_TYPE_UCS4
            CHECKER(SIMD_VAR, read_state, 0xffff, PYYJSON_STRING_TYPE_UCS4) &&
#    endif
                    CHECKER(SIMD_VAR, read_state, 0xff, PYYJSON_STRING_TYPE_UCS2);
            break;
        }
#endif
#if COMPILE_UCS_LEVEL > PYYJSON_STRING_TYPE_UCS2
        case PYYJSON_STRING_TYPE_UCS2: { // TODO
            CHECKER(SIMD_VAR, read_state, 0xffff, PYYJSON_STRING_TYPE_UCS4);
            break;
        }
#endif
        // below are unreachable
        default: {
            assert(false);
            Py_UNREACHABLE();
        }
    }
#undef CHECKER
}

force_inline void UPDATE_WRITE_TYPE(DECODE_UNICODE_INFO *restrict decode_unicode_info, int write_as, int new_write_as) {
    assert(new_write_as > write_as);
#if COMPILE_UCS_LEVEL == 2
    assert(write_as == 2);
    goto inner_2;
#endif
#if COMPILE_UCS_LEVEL <= 1
    if (write_as == 1) {
        decode_unicode_info->ucs1_len = decode_unicode_info->unicode_ucs1 - (u8 *)decode_unicode_info->write_head;
        decode_unicode_info->unicode_ucs1 = NULL;
        if (new_write_as == 2) {
            decode_unicode_info->unicode_ucs2 = (u16 *)decode_unicode_info->write_head + decode_unicode_info->ucs1_len;
        } else {
            assert(new_write_as == 4);
            decode_unicode_info->unicode_ucs4 = (u32 *)decode_unicode_info->write_head + decode_unicode_info->ucs1_len;
        }
        return;
    }
#endif
#if COMPILE_UCS_LEVEL <= 2
    if (write_as == 2) {
    inner_2:;
        Py_ssize_t total_len = decode_unicode_info->unicode_ucs2 - (u16 *)decode_unicode_info->write_head;
#    if COMPILE_UCS_LEVEL <= 1
        decode_unicode_info->ucs2_len = total_len - decode_unicode_info->ucs1_len;
#    else
        decode_unicode_info->ucs2_len = total_len;
#    endif
        assert(decode_unicode_info->ucs2_len >= 0);
        decode_unicode_info->unicode_ucs2 = NULL;
        assert(new_write_as == 4);
        decode_unicode_info->unicode_ucs4 = (u32 *)decode_unicode_info->write_head + total_len;
        return;
    }
#endif
    assert(false);
    Py_UNREACHABLE();
}

force_inline bool VERIFY_ESCAPE_HEX(DECODE_SRC_INFO *decode_src_info) {
    if (unlikely(decode_src_info->src + 4 > decode_src_info->src_end)) {
        PyErr_SetString(JSONDecodeError, "Unexpected ending when reading escaped sequence in string");
        return false;
    }
    // need to verify the next 4 unicode for u16 and u32, since the size of hex conv table is 256
#if COMPILE_READ_UCS_LEVEL == 2
    u64 to_verify = *(u64 *)decode_src_info->src;
    const u64 verify_mask = 0xff00ff00ff00ff00ULL;
    if (unlikely((to_verify & verify_mask) != 0)) {
        PyErr_SetString(JSONDecodeError, "Invalid escape sequence in string");
        return false;
    }
#elif COMPILE_READ_UCS_LEVEL == 4
    SIMD_128 to_verify = load_128((void *)decode_src_info->src);
    const SIMD_128 verify_mask = broadcast_64_128((i64)0xffffff00ffffff00ULL);
    if (unlikely(!testz_128(to_verify, verify_mask))) {
        PyErr_SetString(JSONDecodeError, "Invalid escape sequence in string");
        return false;
    }
#endif
    return true;
}

/* noinline this to reduce binary size */
force_noinline u32 DECODE_ESCAPE_UNICODE(DECODE_SRC_INFO *restrict decode_src_info) {
    // escape
    switch (*++decode_src_info->src) { // clang-format off
        case '"':  decode_src_info->src++; return '"';
        case '\\': decode_src_info->src++; return '\\';
        case '/':  decode_src_info->src++; return '/';
        case 'b':  decode_src_info->src++; return '\b';
        case 'f':  decode_src_info->src++; return '\f';
        case 'n':  decode_src_info->src++; return '\n';
        case 'r':  decode_src_info->src++; return '\r';
        case 't':  decode_src_info->src++; return '\t';
        // clang-format on
        case 'u': {
            u16 hi;

            decode_src_info->src++;
            if (unlikely(!VERIFY_ESCAPE_HEX(decode_src_info) || !READ_TO_HEX_U16(decode_src_info->src, &hi))) {
                if (unlikely(!PyErr_Occurred())) {
                    PyErr_SetString(JSONDecodeError, "Invalid escape sequence in string");
                }
                return (u32)0xffffffff;
            }
            decode_src_info->src += 4;
            if (likely((hi & 0xF800) != 0xD800)) {
                return hi;
            } else {
                u16 lo;
                /* a non-BMP character, represented as a surrogate pair */
                if (unlikely((hi & 0xFC00) != 0xD800)) {
                    PyErr_SetString(JSONDecodeError, "Invalid high surrogate in string");
                    return (u32)0xffffffff;
                }
                if (unlikely(decode_src_info->src + 6 > decode_src_info->src_end || decode_src_info->src[0] != '\\' || decode_src_info->src[1] != 'u')) {
                    PyErr_SetString(JSONDecodeError, "No low surrogate in string");
                    return (u32)0xffffffff;
                }
                if (unlikely(!READ_TO_HEX_U16(decode_src_info->src + 2, &lo))) {
                    PyErr_SetString(JSONDecodeError, "Invalid escaped sequence in string");
                    return (u32)0xffffffff;
                }
                if (unlikely((lo & 0xFC00) != 0xDC00)) {
                    PyErr_SetString(JSONDecodeError, "Invalid low surrogate in string");
                    return (u32)0xffffffff;
                }
                decode_src_info->src += 6;
                return ((((u32)hi - 0xD800) << 10) | ((u32)lo - 0xDC00)) + 0x10000;
            }
        }
        default: {
            // invalid
            PyErr_SetString(JSONDecodeError, "Invalid escape sequence in string");
            return (u32)0xffffffff;
        }
    }
}

/*
 * Call this when find a quote, slash or control character.
 * In most cases, the function returns an ASCII value.
 */
force_inline SpecialCharReadResult DO_SPECIAL(DECODE_SRC_INFO *restrict decode_src_info) {
    SpecialCharReadResult result;
    _FROM_TYPE u = *decode_src_info->src;
    if (likely(u == _Quote)) {
        // end of string
        decode_src_info->src++;
        result.flag = StrEnd;
        return result;
    } else if (u == _Slash) {
        result.value = DECODE_ESCAPE_UNICODE(decode_src_info);
        result.flag = StrContinue;
        if (unlikely(result.value == (u32)0xffffffff)) {
            result.flag = StrInvalid;
        }
        return result;
    } else if (u < ControlMax) {
        // invalid
        result.flag = StrInvalid;
        return result;
    } else {
        assert(false);
        Py_UNREACHABLE();
    }
}

force_noinline void PROCESS_ESCAPE(
        DECODE_UNICODE_INFO *decode_unicode_info,
        ReadStrState *read_state,
        DECODE_SRC_INFO *decode_src_info,
        u32 value,
        int write_as, // one of 1,2,4
        bool do_copy) {
    /*
     * These flags should be updated:
     *  bool need_copy;
     *  int max_char_type;
     *  bool dont_check_max_char;
     *  bool state_dirty;
    */
    assert(!read_state->state_dirty);
    if (!do_copy) {
        // `!do_copy` means this is the first time we meet an escape in current string.
        // then `write_as` must equals to `COMPILE_READ_UCS_LEVEL`, since all unicode before
        // should be in range of `COMPILE_READ_UCS_LEVEL`
        assert(write_as == COMPILE_READ_UCS_LEVEL);
        Py_ssize_t copy_count = UNICODE_DECODE_GET_COPY_COUNT(decode_unicode_info);
        memcpy(decode_unicode_info->write_head, decode_src_info->src_start, COMPILE_READ_UCS_LEVEL * copy_count);
        read_state->state_dirty = true;
        // write need_copy as true, so in following loops we know that a copy is needed
        read_state->need_copy = true;
    }
    assert(read_state->need_copy);
    if (value > 0xffff) {
        // UCS4
        if (COMPILE_READ_UCS_LEVEL < 4 && write_as < 4) {
            UPDATE_WRITE_TYPE(decode_unicode_info, write_as, 4);
            read_state->max_char_type = PYYJSON_STRING_TYPE_UCS4;
            read_state->dont_check_max_char = true;
            read_state->state_dirty = true;
            DECODE_UNICODE_WRITE_ONE_CHAR(decode_unicode_info, 4, value);
            return;
        }
        assert(read_state->max_char_type == PYYJSON_STRING_TYPE_UCS4);
        assert(read_state->dont_check_max_char);
        // leave `state_dirty` as is
        DECODE_UNICODE_WRITE_ONE_CHAR(decode_unicode_info, 4, value);
        return;
    } else if (value > 0xff) {
        // UCS2
        if (COMPILE_READ_UCS_LEVEL < 2 && write_as < 2) {
            assert(read_state->max_char_type < PYYJSON_STRING_TYPE_UCS2);
            UPDATE_WRITE_TYPE(decode_unicode_info, write_as, 2);
            read_state->max_char_type = PYYJSON_STRING_TYPE_UCS2;
            read_state->dont_check_max_char = true;
            read_state->state_dirty = true;
            DECODE_UNICODE_WRITE_ONE_CHAR(decode_unicode_info, 2, value);
            return;
        }
        if (COMPILE_READ_UCS_LEVEL < 2) { // compile time
            // write_as >= 2
            assert(read_state->max_char_type >= PYYJSON_STRING_TYPE_UCS2);
            assert(read_state->dont_check_max_char);
            DECODE_UNICODE_WRITE_ONE_CHAR(
                    decode_unicode_info,
                    read_state->max_char_type,
                    value);
            // leave `state_dirty` as is
            return;
        }
        // COMPILE_READ_UCS_LEVEL >= 2
        bool updated = 2 > read_state->max_char_type;
        read_state->max_char_type = updated ? 2 : read_state->max_char_type;
        read_state->dont_check_max_char = read_state->max_char_type >= COMPILE_UCS_LEVEL;
        read_state->state_dirty = read_state->state_dirty || updated;
        int new_write_as = COMPILE_READ_UCS_LEVEL == 4 ? 4 : read_state->max_char_type;
        if (COMPILE_READ_UCS_LEVEL == 2 && write_as < new_write_as) {
            UPDATE_WRITE_TYPE(decode_unicode_info, write_as, new_write_as);
        }
        DECODE_UNICODE_WRITE_ONE_CHAR(
                decode_unicode_info,
                new_write_as,
                value);
        return;
    } else if (value > 0x7f) {
        // LATIN1
        if (read_state->max_char_type == PYYJSON_STRING_TYPE_ASCII) {
            read_state->max_char_type = PYYJSON_STRING_TYPE_LATIN1;
            bool old_dont_check_max_char = read_state->dont_check_max_char;
            read_state->dont_check_max_char = PYYJSON_STRING_TYPE_LATIN1 >= COMPILE_UCS_LEVEL;
            read_state->state_dirty = (old_dont_check_max_char != read_state->dont_check_max_char) || read_state->state_dirty;
            DECODE_UNICODE_WRITE_ONE_CHAR(decode_unicode_info, COMPILE_READ_UCS_LEVEL, value);
            return;
        } else {
            assert(read_state->max_char_type >= PYYJSON_STRING_TYPE_LATIN1);
            // leave `state_dirty` as is
            DECODE_UNICODE_WRITE_ONE_CHAR(
                    decode_unicode_info,
                    write_as,
                    value);
            return;
        }
    } else {
        // ascii
        DECODE_UNICODE_WRITE_ONE_CHAR(
                decode_unicode_info,
                write_as,
                value);
        return;
    }
}

force_inline void READ_STR_IN_LOOP(
        DECODE_UNICODE_INFO *restrict decode_unicode_info,
        ReadStrState *restrict read_state,
        DECODE_SRC_INFO *restrict decode_src_info,
        /* some immediate numbers*/
        int write_as, // one of 1,2,4
        bool do_copy,
        bool need_check_max_char) {
    SIMD_TYPE SIMD_VAR;
    SIMD_MASK_TYPE check_mask = CHECK_ESCAPE_IMPL_GET_MASK(decode_src_info->src, &SIMD_VAR);
    if (do_copy) {                               // compile time determined
        if (write_as > COMPILE_READ_UCS_LEVEL) { // compile time determined

            if (write_as == 2) { // compile time determined
#if COMPILE_READ_UCS_LEVEL <= 2
                assert(decode_unicode_info->unicode_ucs2);
                WRITE_SIMD_IMPL_TARGET2(GET_UCS2_WRITER(decode_unicode_info), SIMD_VAR);
#else
                assert(false);
                Py_UNREACHABLE();
#endif
            } else {
                assert(write_as == 4);
                assert(decode_unicode_info->unicode_ucs4);
                WRITE_SIMD_IMPL_TARGET4(GET_UCS4_WRITER(decode_unicode_info), SIMD_VAR);
            }

        } else { // compile time determined
            assert(write_as == COMPILE_READ_UCS_LEVEL);
            write_simd(GET_CUR_WRITER(decode_unicode_info), SIMD_VAR);
        }
    }

    if (check_mask_zero(check_mask)) {
        // no special characters in this slice, won't be an ending
        // should be extremely fast if the string is long enough
        decode_src_info->src += CHECK_COUNT_MAX;
        MOVE_WRITER(decode_unicode_info, write_as, CHECK_COUNT_MAX);
        if (need_check_max_char) CHECK_MAX_CHAR_IN_LOOP(SIMD_VAR, read_state, false, CHECK_COUNT_MAX); // compile time determined
    } else {
        // this is not an *unlikely* case
        // for example, for short keys less than 16 bytes,
        // `QUOTE` will be found and `check_mask_zero` returns false
        u32 done_count = GET_DONE_COUNT_FROM_MASK(check_mask);
        decode_src_info->src += done_count;
        MOVE_WRITER(decode_unicode_info, write_as, done_count);
        SpecialCharReadResult escape_result = DO_SPECIAL(decode_src_info);
        if (likely(escape_result.flag == StrEnd)) {
            if (need_check_max_char) CHECK_MAX_CHAR_IN_LOOP(SIMD_VAR, read_state, true, (Py_ssize_t)done_count);
            read_state->scan_flag = StrEnd;
            read_state->state_dirty = true;
            return;
        }
        if (unlikely(escape_result.flag == StrInvalid)) {
            assert(PyErr_Occurred());
            read_state->scan_flag = StrInvalid;
            read_state->state_dirty = true;
            return;
        }
        // slow path (escape character)
        PROCESS_ESCAPE(decode_unicode_info, read_state, decode_src_info, escape_result.value, write_as, do_copy);
        if (need_check_max_char && read_state->max_char_type < COMPILE_UCS_LEVEL) {
            CHECK_MAX_CHAR_IN_LOOP(SIMD_VAR, read_state, true, (Py_ssize_t)done_count);
        }
    }
}

#if COMPILE_UCS_LEVEL > 1
force_inline void DOWNGRADE_STRING(const void *src_start, Py_ssize_t copy_count, int max_char_type, void *write_buffer_head) {
#    if COMPILE_UCS_LEVEL == 2
    assert(max_char_type == 0 || max_char_type == 1);
    // zip u16 buffer to u8 buffer.
    const _FROM_TYPE *src = (const _FROM_TYPE *)src_start;
    u8 *dst = (u8 *)write_buffer_head;
    SIMD_TYPE SIMD_VAR;
    while (copy_count >= CHECK_COUNT_MAX) {
        SIMD_TYPE SIMD_VAR = load_simd((const void *)src);
        SIMD_REAL_HALF_TYPE half_val = zip_simd_16_to_8(SIMD_VAR);
        write_real_half(dst, half_val);
        // *(SIMD_REAL_HALF_TYPE *)dst = half_val;
        copy_count -= CHECK_COUNT_MAX;
        dst += CHECK_COUNT_MAX;
        src += CHECK_COUNT_MAX;
    }
    if (copy_count) {
        Py_ssize_t additional = CHECK_COUNT_MAX - copy_count;
        src -= additional;
        dst -= additional;
        SIMD_TYPE SIMD_VAR = load_simd((const void *)src);
        SIMD_REAL_HALF_TYPE half_val = zip_simd_16_to_8(SIMD_VAR);
        write_real_half(dst, half_val);
        // *(SIMD_REAL_HALF_TYPE *)dst = half_val;
    }
#    else // COMPILE_UCS_LEVEL == 4
    const _FROM_TYPE *src = (const _FROM_TYPE *)src_start;
    if (max_char_type == 2) {
        u16 *dst = (u16 *)write_buffer_head;
        SIMD_TYPE SIMD_VAR;
        while (copy_count >= CHECK_COUNT_MAX) {
            SIMD_TYPE SIMD_VAR = load_simd((const void *)src);
            SIMD_REAL_HALF_TYPE half_val = zip_simd_32_to_16(SIMD_VAR);
            write_real_half(dst, half_val);
            // *(SIMD_REAL_HALF_TYPE *)dst = half_val;
            copy_count -= CHECK_COUNT_MAX;
            dst += CHECK_COUNT_MAX;
            src += CHECK_COUNT_MAX;
        }
        if (copy_count) {
            Py_ssize_t additional = CHECK_COUNT_MAX - copy_count;
            src -= additional;
            dst -= additional;
            SIMD_TYPE SIMD_VAR = load_simd((const void *)src);
            SIMD_REAL_HALF_TYPE half_val = zip_simd_32_to_16(SIMD_VAR);
            write_real_half(dst, half_val);
            // *(SIMD_REAL_HALF_TYPE *)dst = half_val;
        }
    } else {
        assert(max_char_type <= 1);
        u8 *dst = (u8 *)write_buffer_head;
        SIMD_TYPE SIMD_VAR;
        while (copy_count >= CHECK_COUNT_MAX) {
            SIMD_TYPE SIMD_VAR = load_simd((const void *)src);
            SIMD_REAL_QUARTER_TYPE quar_val = zip_simd_32_to_8(SIMD_VAR);
            write_real_quarter(dst, quar_val);
            // *(SIMD_REAL_QUARTER_TYPE *)dst = quar_val;
            copy_count -= CHECK_COUNT_MAX;
            dst += CHECK_COUNT_MAX;
            src += CHECK_COUNT_MAX;
        }
        if (copy_count) {
            Py_ssize_t additional = CHECK_COUNT_MAX - copy_count;
            src -= additional;
            dst -= additional;
            SIMD_TYPE SIMD_VAR = load_simd((const void *)src);
            SIMD_REAL_QUARTER_TYPE quar_val = zip_simd_32_to_8(SIMD_VAR);
            write_real_quarter(dst, quar_val);
            // *(SIMD_REAL_QUARTER_TYPE *)dst = quar_val;
        }
    }
#    endif
}
#endif

force_inline PyObject *DECODE_LOOP_DONE_MAKE_STRING(
        DECODE_SRC_INFO *restrict decode_src_info,
        DECODE_UNICODE_INFO *restrict decode_unicode_info,
        bool need_copy,
        int max_char_type,
        bool is_key) {
    if (!need_copy) {
        assert(max_char_type <= COMPILE_UCS_LEVEL);
        // fast path for not using the write buffer.
        // create unicode directly from the reader.
        Py_ssize_t copy_count = UNICODE_DECODE_GET_COPY_COUNT(decode_unicode_info);
        if (COMPILE_READ_UCS_LEVEL == 1 || max_char_type == COMPILE_READ_UCS_LEVEL) {
            // simplest case, copy the buffer directly to the unicode object.
            // for COMPILE_UCS_LEVEL == 1: since max_char_type <= COMPILE_UCS_LEVEL == 1, this is also a copy-only case.
            return make_string((const u8 *)decode_src_info->src_start, copy_count, max_char_type, is_key);
        } else {
#if COMPILE_UCS_LEVEL > 1
            // need to zip the buffer down to `max_char_type`.
            // use simd to make this faster.
            DOWNGRADE_STRING((const void *)decode_src_info->src_start, copy_count, max_char_type, (_FROM_TYPE *)decode_unicode_info->write_head);
            // create unicode from the writer.
            return make_string((const u8 *)decode_unicode_info->write_head, copy_count, max_char_type, is_key);
#else
            assert(false);
            Py_UNREACHABLE();
            return NULL;
#endif
        }
    } else {
        if (max_char_type == 4) {
            if (UCS_BELOW_4_DIRTY(decode_unicode_info)) {
                COPY_WITH_ELEVATE_TO_4(decode_unicode_info);
            }
            // not dirty now
            return make_string((const u8 *)decode_unicode_info->write_head, decode_unicode_info->unicode_ucs4 - (u32 *)decode_unicode_info->write_head, 4, is_key);
        } else if (max_char_type == 2) {
#if COMPILE_UCS_LEVEL == 4
            // downgrade insitu
            Py_ssize_t copy_count = UNICODE_DECODE_GET_COPY_COUNT(decode_unicode_info);
            DOWNGRADE_STRING((const void *)decode_unicode_info->write_head, copy_count, 2, (_FROM_TYPE *)decode_unicode_info->write_head);
            return make_string((const u8 *)decode_unicode_info->write_head, copy_count, 2, is_key);
#else
            if (UCS_BELOW_2_DIRTY(decode_unicode_info)) {
                COPY_WITH_ELEVATE_TO_2(decode_unicode_info);
            }
            // not dirty now
            return make_string((const u8 *)decode_unicode_info->write_head, decode_unicode_info->unicode_ucs2 - (u16 *)decode_unicode_info->write_head, 2, is_key);
#endif
        } else if (max_char_type <= 1) {
#if COMPILE_UCS_LEVEL > 1
            // downgrade insitu
            Py_ssize_t copy_count = UNICODE_DECODE_GET_COPY_COUNT(decode_unicode_info);
            DOWNGRADE_STRING((const void *)decode_unicode_info->write_head, copy_count, 1, (_FROM_TYPE *)decode_unicode_info->write_head);
            return make_string((const u8 *)decode_unicode_info->write_head, copy_count, 1, is_key);
#else
            return make_string((const u8 *)decode_unicode_info->write_head, decode_unicode_info->unicode_ucs1 - (u8 *)decode_unicode_info->write_head, 1, is_key);
#endif
        } else {
            assert(false);
            Py_UNREACHABLE();
            return NULL;
        }
    }
}

force_inline void PROCESS_TAIL_COPY(
        int write_as,
        int really_write_count,
        DECODE_SRC_INFO *restrict decode_src_info,
        DECODE_UNICODE_INFO *restrict decode_unicode_info) {
#if COMPILE_UCS_LEVEL <= 1
    if (write_as <= 1) {
        assert(decode_unicode_info->unicode_ucs1);
#    define TAIL_WRITER PYYJSON_CONCAT3(tail_write_simd_impl, COMPILE_READ_UCS_LEVEL, 1)
        TAIL_WRITER(decode_src_info->src, decode_unicode_info->unicode_ucs1, really_write_count);
        // decode_unicode_info->unicode_ucs1 += really_write_count;
#    undef TAIL_WRITER
        return;
    }
#endif
#if COMPILE_UCS_LEVEL <= 2
    if (write_as == 2) {
#    define TAIL_WRITER PYYJSON_CONCAT3(tail_write_simd_impl, COMPILE_READ_UCS_LEVEL, 2)
        assert(decode_unicode_info->unicode_ucs2);
        TAIL_WRITER(decode_src_info->src, decode_unicode_info->unicode_ucs2, really_write_count);
        // decode_unicode_info->unicode_ucs2 += really_write_count;
#    undef TAIL_WRITER
        return;
    }
#endif
#define TAIL_WRITER PYYJSON_CONCAT3(tail_write_simd_impl, COMPILE_READ_UCS_LEVEL, 4)
    assert(decode_unicode_info->unicode_ucs4);
    TAIL_WRITER(decode_src_info->src, decode_unicode_info->unicode_ucs4, really_write_count);
    // decode_unicode_info->unicode_ucs4 += really_write_count;
#undef TAIL_WRITER
    //     if (!really_write_count) return;
    //     switch (write_as / COMPILE_READ_UCS_LEVEL) {
    //         case 1: {
    //             // write_as equal to COMPILE_READ_UCS_LEVEL
    //             // no elevate
    //             // TODO
    //             break;
    //         }
    //         case 2: {
    // // 2->4 or 1->2
    // #define EXTRACTOR PYYJSON_CONCAT3(extract, SIMD_BIT_SIZE, two_parts)
    // #if COMPILE_READ_UCS_LEVEL == 1
    // #    define ELEVATOR PYYJSON_CONCAT2(elevate_1_2_to, SIMD_BIT_SIZE)
    // #else
    // #    define ELEVATOR PYYJSON_CONCAT2(elevate_2_4_to, SIMD_BIT_SIZE)
    // #endif
    // // TODO
    // #define TAIL_PARTIAL_WRITER
    //             assert(write_as == COMPILE_READ_UCS_LEVEL * 2);
    //             SIMD_HALF_TYPE base[2];
    //             SIMD_TYPE elv[2];
    //             Py_ssize_t split_count[2];
    //             const Py_ssize_t _PerWrite = CHECK_COUNT_MAX / 2;
    //             EXTRACTOR(SIMD_VAR, &base[0], &base[1]);
    //             split_tail_len_two_parts(really_write_count, CHECK_COUNT_MAX, &split_count[0], &split_count[1]);
    //             elv[0] = ELEVATOR(base[0]);
    //             elv[1] = ELEVATOR(base[1]);
    //             TAIL_PARTIAL_WRITER(dst, elv[0], split_count[0]);
    //             move_voidp_n_bytes(dst, _PerWrite * COMPILE_READ_UCS_LEVEL * 2);
    //             TAIL_PARTIAL_WRITER(dst, elv[1], split_count[1]);
    //             move_voidp_n_bytes(dst, _PerWrite * COMPILE_READ_UCS_LEVEL * 2);
    // #undef TAIL_PARTIAL_WRITER
    // #undef ELEVATOR
    // #undef EXTRACTOR
    //             break;
    //         }
    //         case 4: {
    //             // 1->4
    //             // TODO
    //             break;
    //         }
    //     }
    //     if (write_as > COMPILE_READ_UCS_LEVEL) {
    //         if (write_as == 2) { // 1->2
    // #if COMPILE_READ_UCS_LEVEL <= 2
    //             // TODO
    //             assert(decode_unicode_info->unicode_ucs2);
    //             WRITE_SIMD_IMPL_TARGET2(GET_UCS2_WRITER(decode_unicode_info), SIMD_VAR);
    // #else
    //             assert(false);
    //             Py_UNREACHABLE();
    // #endif
    //         } else {
    //             // 1/2 -> 4
    //             assert(write_as == 4);
    //             assert(decode_unicode_info->unicode_ucs4);
    //             u32 *store_start = GET_UCS4_WRITER(decode_unicode_info) - invalid_head_count;
    //             // TODO
    //             // WRITE_SIMD_IMPL_TARGET4(GET_UCS4_WRITER(decode_unicode_info), SIMD_VAR);
    //         }
    //     } else {
    //         // n -> n
    //         assert(write_as == COMPILE_READ_UCS_LEVEL);
    //         u8 *store_start = ((u8 *)GET_CUR_WRITER(decode_unicode_info)) - invalid_head_count * COMPILE_READ_UCS_LEVEL;
    //         // TODO
    //         // write_simd(, SIMD_VAR);
    //     }
    //     // if we have blendv, write with blendv
    //     // if not, write with runtime bitshift
    // #if PYYJSON_HAS_BLENDV

    // #else

    // #endif
}

force_inline void READ_STR_TAIL(
        DECODE_SRC_INFO *restrict decode_src_info,
        DECODE_UNICODE_INFO *restrict decode_unicode_info,
        ReadStrState *read_state,
        int write_as, // one of 1,2,4
        bool do_copy,
        bool need_check_max_char) {
// TODO
#if SIMD_BIT_SIZE == 512

#else
    static_assert(sizeof(SIMD_MASK_TYPE) == sizeof(SIMD_TYPE), "sizeof(SIMD_MASK_TYPE) == sizeof(SIMD_TYPE)");
    // load backward
    assert(decode_src_info->src + CHECK_COUNT_MAX > decode_src_info->src_end);
    SIMD_TYPE SIMD_VAR;
    // simd_load_head points to the addr to load
    // always assume that the 32 bytes before `src` is readable
    const _FROM_TYPE *simd_load_head = decode_src_info->src_end - CHECK_COUNT_MAX;
    SIMD_MASK_TYPE check_mask = CHECK_ESCAPE_IMPL_GET_MASK(simd_load_head, &SIMD_VAR);
    Py_ssize_t invalid_head_count = decode_src_info->src - simd_load_head;
    SIMD_MASK_TYPE tail_mask;
    // process `check_mask`, removing the invalid head content
    {
        const void *tail_mask_addr = PYYJSON_CONCAT2(read_tail_mask_table, READ_BIT_SIZE)(invalid_head_count);
        tail_mask = load_simd_aligned(tail_mask_addr);
        check_mask = SIMD_AND(tail_mask, check_mask);
    }
    // the read buffer is ended, there should be a '"' here
    if (likely(!check_mask_zero(check_mask))) {
        u32 done_count = GET_DONE_COUNT_FROM_MASK(check_mask);
        //
        Py_ssize_t really_write_count = (Py_ssize_t)done_count - invalid_head_count;
        if (do_copy && really_write_count) {
            PROCESS_TAIL_COPY(write_as, really_write_count, decode_src_info, decode_unicode_info);
        }
        // move reader and writer
        decode_src_info->src += really_write_count;
        MOVE_WRITER(decode_unicode_info, write_as, really_write_count);
        // get the special value (expecting '"')
        SpecialCharReadResult escape_result = DO_SPECIAL(decode_src_info);
        if (likely(escape_result.flag == StrEnd)) {
            if (need_check_max_char) {
                // the first `invalid_head_count` unicodes are not valid, remove this part using AND with tail_mask
                CHECK_MAX_CHAR_IN_LOOP(SIMD_AND(SIMD_VAR, tail_mask), read_state, true, (Py_ssize_t)done_count);
            }
            read_state->scan_flag = StrEnd;
            read_state->state_dirty = true;
            return;
        }
        if (unlikely(escape_result.flag == StrInvalid)) {
            assert(PyErr_Occurred());
            read_state->scan_flag = StrInvalid;
            read_state->state_dirty = true;
            return;
        }
        // slow path (escape character)
        PROCESS_ESCAPE(decode_unicode_info, read_state, decode_src_info, escape_result.value, write_as, do_copy);
        if (need_check_max_char && read_state->max_char_type < COMPILE_UCS_LEVEL) {
            // the first `invalid_head_count` unicodes are not valid, remove this part using AND with tail_mask
            CHECK_MAX_CHAR_IN_LOOP(SIMD_AND(SIMD_VAR, tail_mask), read_state, true, (Py_ssize_t)done_count);
        }
    } else {
        // there is no '"' until the end, the string must be invalid
        PyErr_SetString(JSONDecodeError, "Unexpected ending when reading string");
        read_state->scan_flag = StrInvalid;
    }
#endif
}

/**
 Read a JSON string.
 @param reader The head pointer of string before '"' prefix (inout).
 @param lst JSON last position.
 @param inv Allow invalid unicode.
 @param val The string value to be written.
 @param msg The error message pointer.
 @return Whether success.
 */
force_inline PyObject *READ_STR(
        const _FROM_TYPE **restrict reader_addr, /*IN-OUT*/
        const _FROM_TYPE *_reader_end,
        _FROM_TYPE *_temp_write_buffer,
        bool is_key) {
    PyObject *ret;
    DECODE_UNICODE_INFO _decode_unicode_info;
    ReadStrState _read_state;
    _FROM_TYPE *const temp_write_buffer = _temp_write_buffer;
    INIT_DECODE_UNICODE_INFO(&_decode_unicode_info, (u8 *)temp_write_buffer);
    init_read_state(&_read_state);
    DECODE_SRC_INFO _decode_src_info = {
            .src = *reader_addr,
            .src_start = *reader_addr,
            .src_end = _reader_end,
    };

    if (unlikely(_decode_src_info.src > _decode_src_info.src_end - CHECK_COUNT_MAX)) goto read_tail;
#if COMPILE_UCS_LEVEL == PYYJSON_STRING_TYPE_ASCII
    goto loop_1_f_f;
#elif COMPILE_UCS_LEVEL == PYYJSON_STRING_TYPE_LATIN1
    goto loop_1_f_t;
#elif COMPILE_UCS_LEVEL == PYYJSON_STRING_TYPE_UCS2
    goto loop_2_f_t;
#elif COMPILE_UCS_LEVEL == PYYJSON_STRING_TYPE_UCS4
    goto loop_4_f_t;
#endif
    assert(false);
    Py_UNREACHABLE();

// three immediate numbers:
// write_as = max(_read_state.max_char_type, COMPILE_READ_UCS_LEVEL)
// need_copy == <escape is met>
// need_check_max_char = max_char_type < COMPILE_UCS_LEVEL
// additional note:
//   1. need_copy == false => max_char_type <= COMPILE_UCS_LEVEL
//   2. COMPILE_READ_UCS_LEVEL = COMPILE_UCS_LEVEL ? COMPILE_UCS_LEVEL : 1
#if COMPILE_UCS_LEVEL <= 1
loop_1_f_f:;
    {
        // in this case:
        // write_as == 1
        // need_copy == false
        // need_check_max_char == false
        // this implies max_char_type == COMPILE_UCS_LEVEL && max_char_type <= 1
        assert(_read_state.max_char_type == COMPILE_UCS_LEVEL && _read_state.max_char_type <= 1);
        // BEGIN
        assert(_decode_src_info.src <= _decode_src_info.src_end - CHECK_COUNT_MAX);
        while (_decode_src_info.src <= _decode_src_info.src_end - CHECK_COUNT_MAX) {
            READ_STR_IN_LOOP(&_decode_unicode_info,
                             &_read_state,
                             &_decode_src_info,
                             /* some immediate numbers*/
                             1, false, false);
            if (unlikely(_read_state.state_dirty)) {
                _read_state.state_dirty = false;
                if (_read_state.scan_flag == StrEnd) goto done;
                if (unlikely(_read_state.scan_flag == StrInvalid)) goto fail;
                if (unlikely(_decode_src_info.src > _decode_src_info.src_end - CHECK_COUNT_MAX)) break;
                // escape, or max char updated

                // clang-format off
                #include "decode_loop_dirty/1_f_f.inl.c"
                // clang-format on
            }
        }
        goto read_tail;
        // END
    }
#endif
#if COMPILE_UCS_LEVEL == 1
loop_1_f_t:;
    {
        // in this case:
        // write_as == 1
        // need_copy == false
        // need_check_max_char == true
        // this implies max_char_type == 0 && COMPILE_UCS_LEVEL == 1
        assert(_read_state.max_char_type == 0 && COMPILE_UCS_LEVEL == 1);
        // BEGIN
        assert(_decode_src_info.src <= _decode_src_info.src_end - CHECK_COUNT_MAX);
        while (_decode_src_info.src <= _decode_src_info.src_end - CHECK_COUNT_MAX) {
            READ_STR_IN_LOOP(&_decode_unicode_info,
                             &_read_state,
                             &_decode_src_info,
                             /* some immediate numbers*/
                             1, false, true);
            if (unlikely(_read_state.state_dirty)) {
                _read_state.state_dirty = false;
                if (_read_state.scan_flag == StrEnd) goto done;
                if (unlikely(_read_state.scan_flag == StrInvalid)) goto fail;
                if (unlikely(_decode_src_info.src > _decode_src_info.src_end - CHECK_COUNT_MAX)) break;
                // escape, or max char updated

                // clang-format off
                #include "decode_loop_dirty/1_f_t.inl.c"
                // clang-format on
            }
        }
        goto read_tail;
        // END
    }
#endif
#if COMPILE_UCS_LEVEL <= 1
loop_1_t_f:;
    {
        // in this case:
        // write_as == 1
        // need_copy == true
        // need_check_max_char == false
        // this implies 1 >= max_char_type >= COMPILE_UCS_LEVEL
        assert(_read_state.max_char_type >= COMPILE_UCS_LEVEL && _read_state.max_char_type <= 1);
        // BEGIN
        assert(_decode_src_info.src <= _decode_src_info.src_end - CHECK_COUNT_MAX);
        while (_decode_src_info.src <= _decode_src_info.src_end - CHECK_COUNT_MAX) {
            READ_STR_IN_LOOP(&_decode_unicode_info,
                             &_read_state,
                             &_decode_src_info,
                             /* some immediate numbers*/
                             1, true, false);
            if (unlikely(_read_state.state_dirty)) {
                _read_state.state_dirty = false;
                if (_read_state.scan_flag == StrEnd) goto done;
                if (unlikely(_read_state.scan_flag == StrInvalid)) goto fail;
                if (unlikely(_decode_src_info.src > _decode_src_info.src_end - CHECK_COUNT_MAX)) break;
                // escape, or max char updated

                // clang-format off
                #include "decode_loop_dirty/1_t_f.inl.c"
                // clang-format on
            }
        }
        goto read_tail;
        // END
    }
#endif
#if COMPILE_UCS_LEVEL == 1
loop_1_t_t:;
    {
        // in this case:
        // write_as == 1
        // need_copy == true
        // need_check_max_char == true
        // this implies max_char_type == 0 && COMPILE_UCS_LEVEL == 1
        assert(_read_state.max_char_type == COMPILE_UCS_LEVEL && _read_state.max_char_type <= 1);
        // BEGIN
        assert(_decode_src_info.src <= _decode_src_info.src_end - CHECK_COUNT_MAX);
        while (_decode_src_info.src <= _decode_src_info.src_end - CHECK_COUNT_MAX) {
            READ_STR_IN_LOOP(&_decode_unicode_info,
                             &_read_state,
                             &_decode_src_info,
                             /* some immediate numbers*/
                             1, true, true);
            if (unlikely(_read_state.state_dirty)) {
                _read_state.state_dirty = false;
                if (_read_state.scan_flag == StrEnd) goto done;
                if (unlikely(_read_state.scan_flag == StrInvalid)) goto fail;
                if (unlikely(_decode_src_info.src > _decode_src_info.src_end - CHECK_COUNT_MAX)) break;
                // escape, or max char updated

                // clang-format off
                #include "decode_loop_dirty/1_t_t.inl.c"
                // clang-format on
            }
        }
        goto read_tail;
        // END
    }
#endif
#if COMPILE_UCS_LEVEL == 2
loop_2_f_f:;
    {
        // in this case:
        // write_as == 2
        // need_copy == false
        // need_check_max_char == false
        // this implies 2 == max_char_type == COMPILE_UCS_LEVEL
        assert(_read_state.max_char_type == 2 && _read_state.max_char_type <= 2);
        // BEGIN
        assert(_decode_src_info.src <= _decode_src_info.src_end - CHECK_COUNT_MAX);
        while (_decode_src_info.src <= _decode_src_info.src_end - CHECK_COUNT_MAX) {
            READ_STR_IN_LOOP(&_decode_unicode_info,
                             &_read_state,
                             &_decode_src_info,
                             /* some immediate numbers*/
                             2, false, false);
            if (unlikely(_read_state.state_dirty)) {
                _read_state.state_dirty = false;
                if (_read_state.scan_flag == StrEnd) goto done;
                if (unlikely(_read_state.scan_flag == StrInvalid)) goto fail;
                if (unlikely(_decode_src_info.src > _decode_src_info.src_end - CHECK_COUNT_MAX)) break;
                // escape, or max char updated

                // clang-format off
                #include "decode_loop_dirty/2_f_f.inl.c"
                // clang-format on
            }
        }
        goto read_tail;
        // END
    }
#endif
#if COMPILE_UCS_LEVEL == 2
loop_2_f_t:;
    {
        // in this case:
        // write_as == 2
        // need_copy == false
        // need_check_max_char == true
        // this implies max_char_type < COMPILE_UCS_LEVEL == 2
        assert(_read_state.max_char_type < COMPILE_UCS_LEVEL && COMPILE_UCS_LEVEL <= 2);
        // BEGIN
        assert(_decode_src_info.src <= _decode_src_info.src_end - CHECK_COUNT_MAX);
        while (_decode_src_info.src <= _decode_src_info.src_end - CHECK_COUNT_MAX) {
            READ_STR_IN_LOOP(&_decode_unicode_info,
                             &_read_state,
                             &_decode_src_info,
                             /* some immediate numbers*/
                             2, false, true);
            if (unlikely(_read_state.state_dirty)) {
                _read_state.state_dirty = false;
                if (_read_state.scan_flag == StrEnd) goto done;
                if (unlikely(_read_state.scan_flag == StrInvalid)) goto fail;
                if (unlikely(_decode_src_info.src > _decode_src_info.src_end - CHECK_COUNT_MAX)) break;
                // escape, or max char updated

                // clang-format off
                #include "decode_loop_dirty/2_f_t.inl.c"
                // clang-format on
            }
        }
        goto read_tail;
        // END
    }
#endif
#if COMPILE_UCS_LEVEL <= 2
loop_2_t_f:;
    {
        // in this case:
        // write_as == 2
        // need_copy == true
        // need_check_max_char == false
        // this implies max_char_type == 2 >= COMPILE_UCS_LEVEL
        assert(_read_state.max_char_type >= COMPILE_UCS_LEVEL && _read_state.max_char_type == 2);
        // BEGIN
        assert(_decode_src_info.src <= _decode_src_info.src_end - CHECK_COUNT_MAX);
        while (_decode_src_info.src <= _decode_src_info.src_end - CHECK_COUNT_MAX) {
            READ_STR_IN_LOOP(&_decode_unicode_info,
                             &_read_state,
                             &_decode_src_info,
                             /* some immediate numbers*/
                             2, true, false);
            if (unlikely(_read_state.state_dirty)) {
                _read_state.state_dirty = false;
                if (_read_state.scan_flag == StrEnd) goto done;
                if (unlikely(_read_state.scan_flag == StrInvalid)) goto fail;
                if (unlikely(_decode_src_info.src > _decode_src_info.src_end - CHECK_COUNT_MAX)) break;
                // escape, or max char updated

                // clang-format off
                #include "decode_loop_dirty/2_t_f.inl.c"
                // clang-format on
            }
        }
        goto read_tail;
        // END
    }
#endif
#if COMPILE_UCS_LEVEL == 2
loop_2_t_t:;
    {
        // in this case:
        // write_as == 2
        // need_copy == true
        // need_check_max_char == true
        // this implies max_char_type < COMPILE_UCS_LEVEL == 2
        assert(_read_state.max_char_type < COMPILE_UCS_LEVEL && COMPILE_UCS_LEVEL == 2);
        // BEGIN
        assert(_decode_src_info.src <= _decode_src_info.src_end - CHECK_COUNT_MAX);
        while (_decode_src_info.src <= _decode_src_info.src_end - CHECK_COUNT_MAX) {
            READ_STR_IN_LOOP(&_decode_unicode_info,
                             &_read_state,
                             &_decode_src_info,
                             /* some immediate numbers*/
                             2, true, true);
            if (unlikely(_read_state.state_dirty)) {
                _read_state.state_dirty = false;
                if (_read_state.scan_flag == StrEnd) goto done;
                if (unlikely(_read_state.scan_flag == StrInvalid)) goto fail;
                if (unlikely(_decode_src_info.src > _decode_src_info.src_end - CHECK_COUNT_MAX)) break;
                // escape, or max char updated

                // clang-format off
                #include "decode_loop_dirty/2_t_t.inl.c"
                // clang-format on
            }
        }
        goto read_tail;
        // END
    }
#endif
#if COMPILE_UCS_LEVEL == 4
loop_4_f_f:;
    {
        // in this case:
        // write_as == 4
        // need_copy == false
        // need_check_max_char == false
        // this implies 4 == max_char_type
        assert(_read_state.max_char_type == 4);
        // BEGIN
        assert(_decode_src_info.src <= _decode_src_info.src_end - CHECK_COUNT_MAX);
        while (_decode_src_info.src <= _decode_src_info.src_end - CHECK_COUNT_MAX) {
            READ_STR_IN_LOOP(&_decode_unicode_info,
                             &_read_state,
                             &_decode_src_info,
                             /* some immediate numbers*/
                             4, false, false);
            if (unlikely(_read_state.state_dirty)) {
                _read_state.state_dirty = false;
                if (_read_state.scan_flag == StrEnd) goto done;
                if (unlikely(_read_state.scan_flag == StrInvalid)) goto fail;
                if (unlikely(_decode_src_info.src > _decode_src_info.src_end - CHECK_COUNT_MAX)) break;
                // escape, or max char updated

                // clang-format off
                #include "decode_loop_dirty/4_f_f.inl.c"
                // clang-format on
            }
        }
        goto read_tail;
        // END
    }
#endif
#if COMPILE_UCS_LEVEL == 4
loop_4_f_t:;
    {
        // in this case:
        // write_as == 4
        // need_copy == false
        // need_check_max_char == true
        // this implies max_char_type < COMPILE_UCS_LEVEL == 4
        assert(_read_state.max_char_type < 4);
        // BEGIN
        assert(_decode_src_info.src <= _decode_src_info.src_end - CHECK_COUNT_MAX);
        while (_decode_src_info.src <= _decode_src_info.src_end - CHECK_COUNT_MAX) {
            READ_STR_IN_LOOP(&_decode_unicode_info,
                             &_read_state,
                             &_decode_src_info,
                             /* some immediate numbers*/
                             4, false, true);
            if (unlikely(_read_state.state_dirty)) {
                _read_state.state_dirty = false;
                if (_read_state.scan_flag == StrEnd) goto done;
                if (unlikely(_read_state.scan_flag == StrInvalid)) goto fail;
                if (unlikely(_decode_src_info.src > _decode_src_info.src_end - CHECK_COUNT_MAX)) break;
                // escape, or max char updated

                // clang-format off
                #include "decode_loop_dirty/4_f_t.inl.c"
                // clang-format on
            }
        }
        goto read_tail;
        // END
    }
#endif
loop_4_t_f:;
    {
        // in this case:
        // write_as == 4
        // need_copy == true
        // need_check_max_char == false
        // this implies max_char_type == 4 >= COMPILE_UCS_LEVEL
        // also, **won't goto other labels from here**
        assert(_read_state.max_char_type >= COMPILE_UCS_LEVEL && _read_state.max_char_type == 4);
        // BEGIN
        assert(_decode_src_info.src <= _decode_src_info.src_end - CHECK_COUNT_MAX);
        while (_decode_src_info.src <= _decode_src_info.src_end - CHECK_COUNT_MAX) {
            READ_STR_IN_LOOP(&_decode_unicode_info,
                             &_read_state,
                             &_decode_src_info,
                             /* some immediate numbers*/
                             4, true, false);
            if (_read_state.scan_flag == StrEnd) goto done;
            if (unlikely(_read_state.scan_flag == StrInvalid)) goto fail;
            if (unlikely(_decode_src_info.src > _decode_src_info.src_end - CHECK_COUNT_MAX)) break;
        }
        goto read_tail;
        // END
    }
#if COMPILE_UCS_LEVEL == 4
loop_4_t_t:;
    {
        // in this case:
        // write_as == 4
        // need_copy == true
        // need_check_max_char == true
        // this implies max_char_type < COMPILE_UCS_LEVEL == 4
        assert(_read_state.max_char_type < 4);
        // BEGIN
        assert(_decode_src_info.src <= _decode_src_info.src_end - CHECK_COUNT_MAX);
        while (_decode_src_info.src <= _decode_src_info.src_end - CHECK_COUNT_MAX) {
            READ_STR_IN_LOOP(&_decode_unicode_info,
                             &_read_state,
                             &_decode_src_info,
                             /* some immediate numbers*/
                             4, true, true);
            if (unlikely(_read_state.state_dirty)) {
                _read_state.state_dirty = false;
                if (_read_state.scan_flag == StrEnd) goto done;
                if (unlikely(_read_state.scan_flag == StrInvalid)) goto fail;
                if (unlikely(_decode_src_info.src > _decode_src_info.src_end - CHECK_COUNT_MAX)) break;
                // escape, or max char updated

                // clang-format off
                #include "decode_loop_dirty/4_t_t.inl.c"
                // clang-format on
            }
        }
        goto read_tail;
        // END
    }
#endif
read_tail:;
    // this is the really *unlikely* case
    {
        READ_STR_TAIL(&_decode_src_info, &_decode_unicode_info, &_read_state, PYYJSON_MAX(_read_state.max_char_type, COMPILE_READ_UCS_LEVEL), _read_state.need_copy, _read_state.max_char_type < COMPILE_UCS_LEVEL);
        if (likely(_read_state.scan_flag == StrEnd)) goto done;
        if (unlikely(_read_state.scan_flag == StrInvalid)) goto fail;
        goto read_tail;
    }
done:;
    ret = DECODE_LOOP_DONE_MAKE_STRING(
            &_decode_src_info,
            &_decode_unicode_info,
            _read_state.need_copy,
            _read_state.max_char_type,
            is_key);
    if (unlikely(!ret)) goto fail;

success_cleanup:;
    *reader_addr = _decode_src_info.src;
    assert(ret);
    return ret;

fail:;
    return NULL;
}

force_noinline void FAST_SKIP_SPACES(const _FROM_TYPE **cur_addr, const _FROM_TYPE *end) {
#define SET1 PYYJSON_CONCAT3(broadcast, READ_BIT_SIZE, SIMD_BIT_SIZE)
    const SIMD_TYPE template = SET1(' ');
#undef SET1
    const _FROM_TYPE *cur = *cur_addr;
loop:;
    if (likely(cur + CHECK_COUNT_MAX < end)) {
        SIMD_TYPE SIMD_VAR = load_simd((const void *)cur);
#define CMPNEQ PYYJSON_CONCAT3(cmpneq, READ_BIT_SIZE, SIMD_BIT_SIZE)
        SIMD_MASK_TYPE m = CMPNEQ(SIMD_VAR, template);
#undef CMPNEQ
        if (check_mask_zero(m)) {
            cur += CHECK_COUNT_MAX;
            goto loop;
        } else {
            u32 done_count = GET_DONE_COUNT_FROM_MASK(m);
            cur += done_count;
        }
    } else {
        static _FROM_TYPE _t[2] = {' ', ' '};
        while (true) REPEAT_CALL_16({
            if (likely(cur + 2 <= end && 0 == memcmp((const void *)cur, (const void *)_t, sizeof(_t)))) cur += 2;
            else
                break;
        })
        if (*cur == ' ') cur++;
    }
    *cur_addr = cur;
    assert(*cur != ' ');
}

force_inline bool CHECK_AND_RESERVE_STR_BUFFER(Py_ssize_t len, _FROM_TYPE **buffer_head_addr, bool *need_dealloc) {
    // consider the max length of the buffer we need
    // assume that each string has an escape of ucs4, we need buffer with size
    // sizeof(ucs4) * len == 4 * len
    // reserve additional TAIL_PADDING bytes before and after the buffer for convenience,
    // i.e. additional 2 * TAIL_PADDING bytes
    if (len > ((Py_ssize_t)PY_SSIZE_T_MAX - TAIL_PADDING * 2) / 4) {
        return false;
    }
    static_assert(((Py_ssize_t)PYYJSON_STRING_BUFFER_SIZE - TAIL_PADDING * 2) > 4, "((Py_ssize_t)PYYJSON_STRING_BUFFER_SIZE - 128) > 4");
    Py_ssize_t new_buffer_size = 4 * len + 2 * TAIL_PADDING;
    if (new_buffer_size > PYYJSON_STRING_BUFFER_SIZE) {
        // malloc new buffer
        u8 *new_buffer = (u8 *)malloc(new_buffer_size);
        if (!new_buffer) return false;
        *buffer_head_addr = (_FROM_TYPE *)(new_buffer + TAIL_PADDING);
        *need_dealloc = true;
    } else {
        *buffer_head_addr = (_FROM_TYPE *)(pyyjson_string_buffer + TAIL_PADDING);
        *need_dealloc = false;
    }
    return true;
}

/** Read JSON document (accept all style, but optimized for pretty). */
force_noinline PyObject *READ_ROOT(const _FROM_TYPE *dat, Py_ssize_t len) {
    // check unicode is valid
    // assert(PyUnicode_Check(unicode_root));
    // assert(((PyASCIIObject *)unicode_root)->state.kind == COMPILE_READ_UCS_LEVEL);
    // assert((((PyASCIIObject *)unicode_root)->state.ascii != false) == (COMPILE_UCS_LEVEL == 0));
    // Py_ssize_t len = ((PyASCIIObject *)unicode_root)->length;
    // assert(len > 0);
    // init `dat` ptr
    //     const _FROM_TYPE *const dat =
    // #if COMPILE_UCS_LEVEL == 0
    //             (u8 *)(((PyASCIIObject *)unicode_root) + 1);
    // #else
    //             (_FROM_TYPE *)(((PyCompactUnicodeObject *)unicode_root) + 1);
    // #endif
    //
    const _FROM_TYPE *cur = dat;
    const _FROM_TYPE *const end = cur + len;
    // container stack info
    DecodeCtnStackInfo _decode_ctn_info;
    DecodeCtnStackInfo *decode_ctn_info = &_decode_ctn_info;
    // object stack info
    DecodeObjStackInfo _decode_obj_stack_info;
    DecodeObjStackInfo *const decode_obj_stack_info = &_decode_obj_stack_info;
    memset(decode_ctn_info, 0, sizeof(DecodeCtnStackInfo));
    memset(decode_obj_stack_info, 0, sizeof(DecodeObjStackInfo));
    // init
    if (!init_decode_ctn_stack_info(decode_ctn_info) || !init_decode_obj_stack_info(decode_obj_stack_info)) goto failed_cleanup;
    _FROM_TYPE *string_buffer_head;
    bool need_dealloc = false;
    if (unlikely(!CHECK_AND_RESERVE_STR_BUFFER(len, &string_buffer_head, &need_dealloc))) {
        goto fail_alloc;
    }

    if (*cur++ == '{') {
        set_decode_ctn(decode_ctn_info->ctn, 0, false);
        if (*cur == '\n') cur++;
        goto obj_key_begin;
    } else {
        set_decode_ctn(decode_ctn_info->ctn, 0, true);
        if (*cur == '\n') cur++;
        goto arr_val_begin;
    }

arr_begin:
    /* save current container */
    /* create a new array value, save parent container offset */
    if (unlikely(!ctn_grow_check(decode_ctn_info))) goto fail_ctn_grow;
    set_decode_ctn(decode_ctn_info->ctn, 0, true);

    /* push the new array value as current container */
    if (*cur == '\n') cur++;

arr_val_begin:
    if (cur < end && cur[0] == ' ') {
        cur++;
        if (*cur == ' ')
            FAST_SKIP_SPACES(&cur, end);
    }
    // #if PYYJSON_IS_REAL_GCC
    //     while (true) REPEAT_CALL_16({
    //         if (byte_match_2((void *)cur, "  ")) cur += 2;
    //         else
    //             break;
    //     })
    // #else
    //     while (true) REPEAT_CALL_16({
    //         if (likely(byte_match_2(cur, "  "))) cur += 2;
    //         else
    //             break;
    //     })
    // #endif

    if (*cur == '{') {
        cur++;
        goto obj_begin;
    }
    if (*cur == '[') {
        cur++;
        goto arr_begin;
    }
    if (*cur <= U8MAX && char_is_number(*cur)) {
        PyObject *number_obj = READ_NUMBER(&cur, end);
        if (likely(number_obj && pyyjson_push_obj(decode_obj_stack_info, number_obj))) {
            incr_decode_ctn_size(decode_ctn_info->ctn);
            goto arr_val_end;
        }
        goto fail_number;
    }
    if (*cur == '"') {
        cur++;
        PyObject *str_obj = READ_STR(&cur, end, string_buffer_head, false);
        if (likely(str_obj && pyyjson_push_obj(decode_obj_stack_info, str_obj))) {
            incr_decode_ctn_size(decode_ctn_info->ctn);
            goto arr_val_end;
        }
        goto fail_string;
    }
    if (*cur == 't') {
        if (likely(_READ_TRUE(&cur, end) && pyyjson_decode_true(decode_obj_stack_info))) {
            incr_decode_ctn_size(decode_ctn_info->ctn);
            goto arr_val_end;
        }
        goto fail_literal_true;
    }
    if (*cur == 'f') {
        if (likely(_READ_FALSE(&cur, end) && pyyjson_decode_false(decode_obj_stack_info))) {
            incr_decode_ctn_size(decode_ctn_info->ctn);
            goto arr_val_end;
        }
        goto fail_literal_false;
    }
    if (*cur == 'n') {
        if (likely(_READ_NULL(&cur, end) && pyyjson_decode_null(decode_obj_stack_info))) {
            incr_decode_ctn_size(decode_ctn_info->ctn);
            goto arr_val_end;
        }
        if (likely(_READ_NAN(&cur, end) && pyyjson_decode_nan(decode_obj_stack_info, false))) {
            incr_decode_ctn_size(decode_ctn_info->ctn);
            goto arr_val_end;
        }
        goto fail_literal_null;
    }
    if (*cur == ']') {
        cur++;
        if (likely(get_decode_ctn_len(decode_ctn_info->ctn) == 0)) goto arr_end;
        while (*cur != ',') cur--;
        goto fail_trailing_comma;
    }
    if (char_is_space(*cur)) {
        FAST_SKIP_SPACES(&cur, end);
        if (char_is_space(*cur)) cur++;
        // while (char_is_space(*++cur));
        goto arr_val_begin;
    }
    if ((*cur == 'i' || *cur == 'I' || *cur == 'N')) {
        PyObject *number_obj = READ_INF_OR_NAN(false, &cur, end);
        if (likely(number_obj && pyyjson_push_obj(decode_obj_stack_info, number_obj))) {
            incr_decode_ctn_size(decode_ctn_info->ctn);
            goto arr_val_end;
        }
        goto fail_character_val;
    }

    goto fail_character_val;

arr_val_end:;
    {
        static _FROM_TYPE _t[2] = {',', '\n'};
        if (cur < end && 0 == memcmp((void *)cur, _t, sizeof(_t))) {
            cur += 2;
            goto arr_val_begin;
        }
    }
    if (*cur == ',') {
        cur++;
        goto arr_val_begin;
    }
    if (*cur == ']') {
        cur++;
        goto arr_end;
    }
    if (char_is_space(*cur)) {
        cur++;
        if (*cur == ' ') FAST_SKIP_SPACES(&cur, end);
        if (char_is_space(*cur)) cur++;
        // while (char_is_space(*++cur));
        goto arr_val_end;
    }

    goto fail_character_arr_end;

arr_end:
    assert(decode_ctn_is_arr(decode_ctn_info->ctn));
    if (!pyyjson_decode_arr(decode_obj_stack_info, get_decode_ctn_len(decode_ctn_info->ctn))) goto failed_cleanup;
    /* pop parent as current container */
    if (unlikely(decode_ctn_info->ctn-- == decode_ctn_info->ctn_start)) {
        goto doc_end;
    }

    incr_decode_ctn_size(decode_ctn_info->ctn);
    if (*cur == '\n') cur++;
    if (!decode_ctn_is_arr(decode_ctn_info->ctn)) {
        goto obj_val_end;
    } else {
        goto arr_val_end;
    }

obj_begin:
    /* push container */
    if (unlikely(!ctn_grow_check(decode_ctn_info))) goto fail_ctn_grow;
    set_decode_ctn(decode_ctn_info->ctn, 0, false);
    if (*cur == '\n') cur++;

obj_key_begin:
    if (cur < end && cur[0] == ' ') {
        cur++;
        if (*cur == ' ')
            FAST_SKIP_SPACES(&cur, end);
    }
    // #if PYYJSON_IS_REAL_GCC
    //     while (true) REPEAT_CALL_16({
    //         if (byte_match_2((void *)cur, "  ")) cur += 2;
    //         else
    //             break;
    //     })
    // #else
    //     while (true) REPEAT_CALL_16({
    //         if (likely(byte_match_2(cur, "  "))) cur += 2;
    //         else
    //             break;
    //     })
    // #endif
    if (likely(*cur == '"')) {
        cur++;
        PyObject *str_obj = READ_STR(&cur, end, string_buffer_head, true);
        ;
        if (likely(str_obj && pyyjson_push_obj(decode_obj_stack_info, str_obj))) {
            goto obj_key_end;
        }
        goto fail_string;
    }
    if (likely(*cur == '}')) {
        cur++;
        if (likely(get_decode_ctn_len(decode_ctn_info->ctn) == 0)) goto obj_end;
        goto fail_trailing_comma;
    }
    if (char_is_space(*cur)) {
        FAST_SKIP_SPACES(&cur, end);
        if (char_is_space(*cur)) cur++;
        // while (char_is_space(*++cur));
        goto obj_key_begin;
    }
    goto fail_character_obj_key;

obj_key_end:;
    {
        static _FROM_TYPE _t[2] = {':', ' '};
        if (cur < end && 0 == memcmp((const void *)cur, (const void *)_t, sizeof(_t))) {
            cur += 2;
            goto obj_val_begin;
        }
    }
    if (*cur == ':') {
        cur++;
        goto obj_val_begin;
    }
    if (char_is_space(*cur)) {
        FAST_SKIP_SPACES(&cur, end);
        if (char_is_space(*cur)) cur++;
        // while (char_is_space(*++cur));
        goto obj_key_end;
    }
    goto fail_character_obj_sep;

obj_val_begin:
    if (*cur == '"') {
        cur++;
        PyObject *str_obj = READ_STR(&cur, end, string_buffer_head, false);
        ;
        if (likely(str_obj && pyyjson_push_obj(decode_obj_stack_info, str_obj))) {
            incr_decode_ctn_size(decode_ctn_info->ctn);
            goto obj_val_end;
        }
        goto fail_string;
    }
    if (char_is_number(*cur)) {
        PyObject *number_obj = READ_NUMBER(&cur, end);
        if (likely(number_obj && pyyjson_push_obj(decode_obj_stack_info, number_obj))) {
            incr_decode_ctn_size(decode_ctn_info->ctn);
            goto obj_val_end;
        }
        goto fail_number;
    }
    if (*cur == '{') {
        cur++;
        goto obj_begin;
    }
    if (*cur == '[') {
        cur++;
        goto arr_begin;
    }
    if (*cur == 't') {
        if (likely(_READ_TRUE(&cur, end) && pyyjson_decode_true(decode_obj_stack_info))) {
            incr_decode_ctn_size(decode_ctn_info->ctn);
            goto obj_val_end;
        }
        goto fail_literal_true;
    }
    if (*cur == 'f') {
        if (likely(_READ_FALSE(&cur, end) && pyyjson_decode_false(decode_obj_stack_info))) {
            incr_decode_ctn_size(decode_ctn_info->ctn);
            goto obj_val_end;
        }
        goto fail_literal_false;
    }
    if (*cur == 'n') {
        if (likely(_READ_NULL(&cur, end) && pyyjson_decode_null(decode_obj_stack_info))) {
            incr_decode_ctn_size(decode_ctn_info->ctn);
            goto obj_val_end;
        }
        if (likely(_READ_NAN(&cur, end) && pyyjson_decode_nan(decode_obj_stack_info, false))) {
            incr_decode_ctn_size(decode_ctn_info->ctn);
            goto obj_val_end;
        }
        goto fail_literal_null;
    }
    if (char_is_space(*cur)) {
        FAST_SKIP_SPACES(&cur, end);
        if (char_is_space(*cur)) cur++;
        // while (char_is_space(*++cur));
        goto obj_val_begin;
    }
    if ((*cur == 'i' || *cur == 'I' || *cur == 'N')) {
        PyObject *number_obj = READ_INF_OR_NAN(false, &cur, end);
        if (likely(number_obj && pyyjson_push_obj(decode_obj_stack_info, number_obj))) {
            incr_decode_ctn_size(decode_ctn_info->ctn);
            goto obj_val_end;
        }
        goto fail_character_val;
    }

    goto fail_character_val;

obj_val_end:;
    {
        static _FROM_TYPE _t[2] = {',', '\n'};
        if (cur < end && 0 == memcmp((const void *)cur, (const void *)_t, sizeof(_t))) {
            cur += 2;
            goto obj_key_begin;
        }
    }
    if (likely(*cur == ',')) {
        cur++;
        goto obj_key_begin;
    }
    if (likely(*cur == '}')) {
        cur++;
        goto obj_end;
    }
    if (char_is_space(*cur)) {
        FAST_SKIP_SPACES(&cur, end);
        if (char_is_space(*cur)) cur++;
        // while (char_is_space(*++cur));
        goto obj_val_end;
    }

    goto fail_character_obj_end;

obj_end:
    assert(!decode_ctn_is_arr(decode_ctn_info->ctn));
    if (unlikely(!pyyjson_decode_obj(decode_obj_stack_info, get_decode_ctn_len(decode_ctn_info->ctn)))) goto failed_cleanup;
    /* pop container */
    /* point to the next value */
    if (unlikely(decode_ctn_info->ctn-- == decode_ctn_info->ctn_start)) {
        goto doc_end;
    }
    incr_decode_ctn_size(decode_ctn_info->ctn);
    if (*cur == '\n') cur++;
    if (decode_ctn_is_arr(decode_ctn_info->ctn)) {
        goto arr_val_end;
    } else {
        goto obj_val_end;
    }

doc_end:
    /* check invalid contents after json document */
    if (unlikely(cur < end)) {
        FAST_SKIP_SPACES(&cur, end);
        if (char_is_space(*cur)) cur++;
        // while (char_is_space(*cur)) cur++;
        if (unlikely(cur < end)) goto fail_garbage;
    }

success:;
    PyObject *obj = *decode_obj_stack_info->result_stack;
    assert(decode_ctn_info->ctn == decode_ctn_info->ctn_start - 1);
    assert(decode_obj_stack_info->cur_write_result_addr == decode_obj_stack_info->result_stack + 1);
    assert(obj && !PyErr_Occurred());
    assert(obj->ob_refcnt == 1);
    // free string buffer
    if (need_dealloc) {
        free((void *)((u8 *)string_buffer_head - TAIL_PADDING));
    }
    // free obj stack buffer if allocated dynamically
    if (unlikely(decode_obj_stack_info->result_stack_end - decode_obj_stack_info->result_stack > PYYJSON_DECODE_OBJ_BUFFER_INIT_SIZE)) {
        free(decode_obj_stack_info->result_stack);
    }

    return obj;

#define return_err(_pos, _type, _msg)                                                                             \
    do {                                                                                                          \
        if (_type == JSONDecodeError) {                                                                           \
            PyErr_Format(JSONDecodeError, "%s, at position %zu", _msg, ((_FROM_TYPE *)_pos) - (_FROM_TYPE *)dat); \
        } else {                                                                                                  \
            PyErr_SetString(_type, _msg);                                                                         \
        }                                                                                                         \
        goto failed_cleanup;                                                                                      \
    } while (0)

fail_string:
    return_err(cur, JSONDecodeError, "invalid string");
fail_number:
    return_err(cur, JSONDecodeError, "invalid number");
fail_alloc:
    return_err(cur, PyExc_MemoryError,
               "memory allocation failed");
fail_trailing_comma:
    return_err(cur, JSONDecodeError,
               "trailing comma is not allowed");
fail_literal_true:
    return_err(cur, JSONDecodeError,
               "invalid literal, expected a valid literal such as 'true'");
fail_literal_false:
    return_err(cur, JSONDecodeError,
               "invalid literal, expected a valid literal such as 'false'");
fail_literal_null:
    return_err(cur, JSONDecodeError,
               "invalid literal, expected a valid literal such as 'null'");
fail_character_val:
    return_err(cur, JSONDecodeError,
               "unexpected character, expected a valid JSON value");
fail_character_arr_end:
    return_err(cur, JSONDecodeError,
               "unexpected character, expected a comma or a closing bracket");
fail_character_obj_key:
    return_err(cur, JSONDecodeError,
               "unexpected character, expected a string for object key");
fail_character_obj_sep:
    return_err(cur, JSONDecodeError,
               "unexpected character, expected a colon after object key");
fail_character_obj_end:
    return_err(cur, JSONDecodeError,
               "unexpected character, expected a comma or a closing brace");
fail_comment:
    return_err(cur, JSONDecodeError,
               "unclosed multiline comment");
fail_garbage:
    return_err(cur, JSONDecodeError,
               "unexpected content after document");
fail_ctn_grow:
    return_err(cur, JSONDecodeError,
               "max recursion exceeded");

failed_cleanup:
    for (PyObject **obj_ptr = decode_obj_stack_info->result_stack; obj_ptr < decode_obj_stack_info->cur_write_result_addr; obj_ptr++) {
        Py_XDECREF(*obj_ptr);
    }
    // free string buffer
    if (need_dealloc) {
        free((void *)((u8 *)string_buffer_head - TAIL_PADDING));
    }
    // free obj stack buffer if allocated dynamically
    if (unlikely(decode_obj_stack_info->result_stack_end - decode_obj_stack_info->result_stack > PYYJSON_DECODE_OBJ_BUFFER_INIT_SIZE)) {
        free(decode_obj_stack_info->result_stack);
    }
    return NULL;
#undef return_err
}

/** Read single value JSON document. */
force_noinline PyObject *READ_ROOT_SINGLE(const _FROM_TYPE *dat, Py_ssize_t len) {
#define return_err(_pos, _type, _msg)                                                             \
    do {                                                                                          \
        if (_type == JSONDecodeError) {                                                           \
            PyErr_Format(JSONDecodeError, "%s, at position %zu", _msg, ((u8 *)_pos) - (u8 *)dat); \
        } else {                                                                                  \
            PyErr_SetString(_type, _msg);                                                         \
        }                                                                                         \
        goto fail_cleanup;                                                                        \
    } while (0)

    // check unicode is valid
    // assert(PyUnicode_Check(unicode_root));
    // assert(((PyASCIIObject *)unicode_root)->state.kind == COMPILE_READ_UCS_LEVEL);
    // assert((((PyASCIIObject *)unicode_root)->state.ascii != false) == (COMPILE_UCS_LEVEL == 0));
    // Py_ssize_t len = ((PyASCIIObject *)unicode_root)->length;
    assert(len > 0);
    // init `dat` ptr
    //     const _FROM_TYPE *const dat =
    // #if COMPILE_UCS_LEVEL == 0
    //             (u8 *)(((PyASCIIObject *)unicode_root) + 1);
    // #else
    //             (_FROM_TYPE *)(((PyCompactUnicodeObject *)unicode_root) + 1);
    // #endif
    //
    const _FROM_TYPE *cur = dat;
    const _FROM_TYPE *const end = cur + len;
    // const u8 *cur = (const u8 *)dat;
    // const u8 *const end = cur + len;

    PyObject *ret = NULL;

    if (*cur <= U8MAX && char_is_number(*cur)) {
        ret = READ_NUMBER(&cur, end);
        if (likely(ret)) goto single_end;
        goto fail_number;
    }
    if (*cur == '"') {
        // u8 *write_buffer;
        _FROM_TYPE *string_buffer_head;
        bool need_dealloc = false;
        CHECK_AND_RESERVE_STR_BUFFER(len, &string_buffer_head, &need_dealloc);
        cur++;
        ret = READ_STR(&cur, end, string_buffer_head, false);
        if (need_dealloc) {
            free((void *)((u8 *)string_buffer_head - TAIL_PADDING));
        }
        if (likely(ret)) goto single_end;
        goto fail_string;
    }
    if (*cur == 't') {
        if (likely(_READ_TRUE(&cur, end))) {
            Py_Immortal_IncRef(Py_True);
            ret = Py_True;
            goto single_end;
        }
        goto fail_literal_true;
    }
    if (*cur == 'f') {
        if (likely(_READ_FALSE(&cur, end))) {
            Py_Immortal_IncRef(Py_False);
            ret = Py_False;
            goto single_end;
        }
        goto fail_literal_false;
    }
    if (*cur == 'n') {
        if (likely(_READ_NULL(&cur, end))) {
            Py_Immortal_IncRef(Py_None);
            ret = Py_None;
            goto single_end;
        }
        if (_READ_NAN(&cur, end)) {
            ret = PyFloat_FromDouble(fabs(Py_NAN));
            if (likely(ret)) goto single_end;
        }
        goto fail_literal_null;
    }
    {
        ret = READ_INF_OR_NAN(false, &cur, end);
        if (likely(ret)) goto single_end;
    }
    goto fail_character;

single_end:
    assert(ret);
    if (unlikely(cur < end)) {
        FAST_SKIP_SPACES(&cur, end);
        if (char_is_space(*cur)) cur++;
        // while (char_is_space(*cur)) cur++;
        if (unlikely(cur < end)) goto fail_garbage;
    }
    return ret;

fail_string:
    return_err(cur, JSONDecodeError, "invalid string");
fail_number:
    return_err(cur, JSONDecodeError, "invalid number");
fail_alloc:
    return_err(cur, PyExc_MemoryError,
               "memory allocation failed");
fail_literal_true:
    return_err(cur, JSONDecodeError,
               "invalid literal, expected a valid literal such as 'true'");
fail_literal_false:
    return_err(cur, JSONDecodeError,
               "invalid literal, expected a valid literal such as 'false'");
fail_literal_null:
    return_err(cur, JSONDecodeError,
               "invalid literal, expected a valid literal such as 'null'");
fail_character:
    return_err(cur, JSONDecodeError,
               "unexpected character, expected a valid root value");
fail_comment:
    return_err(cur, JSONDecodeError,
               "unclosed multiline comment");
fail_garbage:
    return_err(cur, JSONDecodeError,
               "unexpected content after document");
fail_cleanup:
    Py_XDECREF(ret);
    return NULL;
#undef return_err
}

force_noinline PyObject *PYYJSON_DECODE_STR(PyUnicodeObject *in_unicode) {
    // some checks
    assert(in_unicode);
    PyASCIIObject *ascii_head = PYYJSON_STATIC_CAST(PyASCIIObject *, in_unicode);
    assert((ascii_head->state.ascii ? 0 : ascii_head->state.kind) == COMPILE_UCS_LEVEL);
    if (unlikely(!ascii_head->length)) {
        PyErr_Format(JSONDecodeError, "input data is empty");
        return NULL;
    }
#if COMPILE_UCS_LEVEL > 0
    const _FROM_TYPE *buffer = PYYJSON_STATIC_CAST(_FROM_TYPE *, PYYJSON_STATIC_CAST(PyCompactUnicodeObject *, in_unicode) + 1);
#else
    const _FROM_TYPE *buffer = PYYJSON_STATIC_CAST(_FROM_TYPE *, ascii_head + 1);
#endif
    assert(buffer);
    assert(ascii_head->length > 0);

    const _FROM_TYPE *const end = buffer + ascii_head->length;
    PyObject *ret;
    assert(*end == 0);

    /* skip empty contents before json document */
    if (unlikely(*buffer <= U8MAX && char_is_space_or_comment(*buffer))) {
        if (likely(*buffer <= U8MAX && char_is_space(*buffer))) {
            while ((*++buffer) <= U8MAX && char_is_space(*buffer));
        }
        if (unlikely(buffer >= end)) {
            PyErr_Format(JSONDecodeError, "input data is empty");
            return NULL;
        }
    }

    /* read json document */
    if (likely(*buffer <= U8MAX && char_is_container(*buffer))) {
        ret = READ_ROOT(buffer, end - buffer);
    } else {
        ret = READ_ROOT_SINGLE(buffer, end - buffer);
    }

    return ret;
}

#undef READ_INF_OR_NAN
#undef READ_NUMBER
#undef READ_INF_OR_NAN
#undef _READ_NAN
#undef _READ_INF
#undef _READ_NULL
#undef _READ_FALSE
#undef _READ_TRUE
#undef CHECK_AND_RESERVE_STR_BUFFER
#undef COPY_WITH_ELEVATE_TO_4
#undef COPY_WITH_ELEVATE_TO_2
#undef UCS_BELOW_4_DIRTY
#undef UCS_BELOW_2_DIRTY
#undef DOWNGRADE_STRING
#undef WRITE_SIMD_IMPL_TARGET4
#undef WRITE_SIMD_IMPL_TARGET2
#undef GET_DONE_COUNT_FROM_MASK
#undef CHECK_ESCAPE_IMPL_GET_MASK
#undef DECODE_SRC_INFO
#undef DECODE_LOOP_DONE_MAKE_STRING
#undef PROCESS_ESCAPE
#undef DO_SPECIAL
#undef DECODE_ESCAPE_UNICODE
#undef VERIFY_ESCAPE_HEX
#undef UPDATE_WRITE_TYPE
#undef CHECK_MAX_CHAR_IN_LOOP
#undef MOVE_WRITER
#undef GET_CUR_WRITER
#undef GET_UCS4_WRITER
#undef GET_UCS2_WRITER
#undef DECODE_UNICODE_WRITE_ONE_CHAR
#undef UNICODE_DECODE_GET_COPY_COUNT
#undef INIT_DECODE_UNICODE_INFO
#undef DECODE_UNICODE_INFO
#undef READ_TO_HEX_U16
#undef READ_STR_TAIL
#undef PROCESS_TAIL_COPY
#undef READ_STR_IN_LOOP
#undef READ_ROOT_SINGLE
#undef READ_ROOT
#undef FAST_SKIP_SPACES
#undef READ_STR
#undef PYYJSON_DECODE_STR
//
#undef COMPILE_READ_UCS_LEVEL
