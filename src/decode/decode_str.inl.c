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


#define READ_STR PYYJSON_CONCAT2(read_str, COMPILE_UCS_LEVEL)
#define READ_STR_IN_LOOP PYYJSON_CONCAT2(read_str_in_loop, COMPILE_UCS_LEVEL)
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
                if (unlikely(decode_src_info->src + 6 > decode_src_info->src_end || !byte_match_2(decode_src_info->src, "\\u"))) {
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
                assert(decode_unicode_info->unicode_ucs2);
#if COMPILE_READ_UCS_LEVEL <= 2
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
        // read_state->scan_flag = StrContinue;
    } else {
        // this is not an *unlikely* case
        // for example, for short keys less than 16 bytes,
        // `QUOTE` will be found and `check_mask_zero` returns false
        u32 done_count = GET_DONE_COUNT_FROM_MASK(check_mask);
        decode_src_info->src += done_count;
        MOVE_WRITER(decode_unicode_info, write_as, done_count);
        SpecialCharReadResult escape_result = DO_SPECIAL(decode_src_info);
        // *write_scan_flag = escape_result.flag;
        if (likely(escape_result.flag == StrEnd)) {
            if (need_check_max_char) CHECK_MAX_CHAR_IN_LOOP(SIMD_VAR, read_state, true, (Py_ssize_t)done_count);
            read_state->scan_flag = StrEnd;
            return;
        }
        if (unlikely(escape_result.flag == StrInvalid)) {
            assert(PyErr_Occurred());
            read_state->scan_flag = StrInvalid;
            return;
        }
        // slow path (escape character)
        PROCESS_ESCAPE(decode_unicode_info, read_state, decode_src_info, escape_result.value, write_as, do_copy);
        // read_state->scan_flag = StrContinue;
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
        *(SIMD_REAL_HALF_TYPE *)dst = half_val;
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
        *(SIMD_REAL_HALF_TYPE *)dst = half_val;
    }
#    else // COMPILE_UCS_LEVEL == 4
    const _FROM_TYPE *src = (const _FROM_TYPE *)src_start;
    if (max_char_type == 2) {
        u16 *dst = (u16 *)write_buffer_head;
        SIMD_TYPE SIMD_VAR;
        while (copy_count >= CHECK_COUNT_MAX) {
            SIMD_TYPE SIMD_VAR = load_simd((const void *)src);
            SIMD_REAL_HALF_TYPE half_val = zip_simd_32_to_16(SIMD_VAR);
            *(SIMD_REAL_HALF_TYPE *)dst = half_val;
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
            *(SIMD_REAL_HALF_TYPE *)dst = half_val;
        }
    } else {
        assert(max_char_type == 1);
        u8 *dst = (u8 *)write_buffer_head;
        SIMD_TYPE SIMD_VAR;
        while (copy_count >= CHECK_COUNT_MAX) {
            SIMD_TYPE SIMD_VAR = load_simd((const void *)src);
            SIMD_REAL_QUARTER_TYPE quar_val = zip_simd_32_to_8(SIMD_VAR);
            *(SIMD_REAL_QUARTER_TYPE *)dst = quar_val;
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
            *(SIMD_REAL_QUARTER_TYPE *)dst = quar_val;
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
            return make_string((const u8 *)decode_unicode_info->write_head, decode_unicode_info->ucs4_len, 4, is_key);
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
            return make_string((const u8 *)decode_unicode_info->write_head, decode_unicode_info->ucs2_len, 2, is_key);
#endif
        } else if (max_char_type <= 1) {
#if COMPILE_UCS_LEVEL > 1
            // downgrade insitu
            Py_ssize_t copy_count = UNICODE_DECODE_GET_COPY_COUNT(decode_unicode_info);
            DOWNGRADE_STRING((const void *)decode_unicode_info->write_head, copy_count, 1, (_FROM_TYPE *)decode_unicode_info->write_head);
            return make_string((const u8 *)decode_unicode_info->write_head, copy_count, 1, is_key);
#else
            return make_string((const u8 *)decode_unicode_info->write_head, decode_unicode_info->ucs1_len, 1, is_key);
#endif
        } else {
            assert(false);
            Py_UNREACHABLE();
            return NULL;
        }
    }
}

force_inline void READ_STR_TAIL(
        DECODE_SRC_INFO *restrict decode_src_info,
        DECODE_UNICODE_INFO *restrict decode_unicode_info,
        ReadStrState *read_state) {
    // TODO
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
force_noinline PyObject *READ_STR(
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
        assert(_read_state.max_char_type >= COMPILE_UCS_LEVEL && COMPILE_UCS_LEVEL <= 2);
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
        READ_STR_TAIL(&_decode_src_info, &_decode_unicode_info, &_read_state);
        if (unlikely(_read_state.scan_flag == StrInvalid)) goto fail;
        goto done;
    }
done:;
    ret = DECODE_LOOP_DONE_MAKE_STRING(
            &_decode_src_info,
            &_decode_unicode_info,
            _read_state.need_copy,
            _read_state.max_char_type,
            is_key);
    // if (_read_state.need_copy) {
    //     // fast path for not using the write buffer.
    //     // create unicode directly from the reader.
    //     if (max_char_type == COMPILE_UCS_LEVEL || COMPILE_UCS_LEVEL == 1) {
    //         // simplest case, copy the buffer directly to the unicode object.
    //         // for COMPILE_UCS_LEVEL == 1: since max_char_type <= COMPILE_UCS_LEVEL == 1, this is also a copy-only case.
    //         ret = make_string((const u8 *)reader_start, reader - reader_start, max_char_type, is_key);
    //         if (unlikely(!ret)) goto fail;
    //         goto success_cleanup;
    //     } else {
    //         // need to zip the buffer down to `max_char_type`.
    //         // use simd to make this faster.
    //         DOWNGRADE_STRING((const void *)reader_start, reader - reader_start, max_char_type, dst_start);
    //         // create unicode from the write buffer.
    //         ret = make_string((const u8 *)dst_start, reader - reader_start, max_char_type, is_key);
    //         if (unlikely(!ret)) goto fail;
    //         goto success_cleanup;
    //     }
    // } else {
    //     if (!(max_char_type == COMPILE_UCS_LEVEL || COMPILE_UCS_LEVEL == 1)) {
    //         // downgrade write buffer insitu
    //         DOWNGRADE_STRING((const void *)dst_start, writer - dst_start, max_char_type, dst_start);
    //     }
    //     ret = make_string((const u8 *)dst_start, writer - dst_start, max_char_type, is_key);
    //     if (unlikely(!ret)) goto fail;
    //     goto success_cleanup;
    // }
    if (unlikely(!ret)) goto fail;

success_cleanup:;
    *reader_addr = _decode_src_info.src;
    assert(ret);
    return ret;

fail:;
    return NULL;
}

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
#undef READ_STR_IN_LOOP
#undef READ_STR
//
#undef COMPILE_READ_UCS_LEVEL
