#ifdef PYYJSON_CLANGD_DUMMY
#    include "decode/str/str.h"
#    include "decode_str_common.h"
#    include "pyyjson.h"
#    include "simd/cvt.h"
#    include "simd/long_cvt.h"
#    include "simd/mask_table.h"
#    include "simd/simd_impl.h"
#    ifndef COMPILE_SIMD_BITS
#        define COMPILE_SIMD_BITS 512
#    endif
#endif


#if COMPILE_UCS_LEVEL == 0
#    define COMPILE_READ_UCS_LEVEL 1
#else
#    define COMPILE_READ_UCS_LEVEL COMPILE_UCS_LEVEL
#endif
//
#include "compile_context/sr_in.inl.h"

#define PYYJSON_DECODE_STR PYYJSON_CONCAT2(pyyjson_decode_str, COMPILE_UCS_LEVEL)
#define SHOULD_READ_PRETTY PYYJSON_CONCAT2(should_read_pretty, COMPILE_UCS_LEVEL)
#define READ_STR PYYJSON_CONCAT2(read_str, COMPILE_UCS_LEVEL)
#define READ_ROOT_PRETTY PYYJSON_CONCAT2(read_root_pretty, COMPILE_UCS_LEVEL)
#define READ_ROOT_MINIFY PYYJSON_CONCAT2(read_root_minify, COMPILE_UCS_LEVEL)
#define READ_ROOT_SINGLE PYYJSON_CONCAT2(read_root_single, COMPILE_UCS_LEVEL)
#define READ_STR_IN_LOOP PYYJSON_CONCAT2(read_str_in_loop, COMPILE_UCS_LEVEL)
#define PROCESS_TAIL_COPY PYYJSON_CONCAT2(process_tail_copy, COMPILE_UCS_LEVEL)
#define READ_STR_TAIL PYYJSON_CONCAT2(read_str_tail, COMPILE_UCS_LEVEL)
#define DECODE_UNICODE_INFO PYYJSON_CONCAT2(DecodeUnicodeInfo, COMPILE_UCS_LEVEL)
#define INIT_DECODE_UNICODE_INFO PYYJSON_CONCAT2(init_decode_unicode_info, COMPILE_UCS_LEVEL)
#define UNICODE_DECODE_GET_COPY_COUNT PYYJSON_CONCAT2(unicode_decode_get_copy_count, COMPILE_UCS_LEVEL)
#define DECODE_UNICODE_WRITE_ONE_CHAR PYYJSON_CONCAT2(decode_unicode_write_one_char, COMPILE_UCS_LEVEL)
#define GET_UCS2_WRITER PYYJSON_CONCAT2(get_ucs2_writer, COMPILE_UCS_LEVEL)
#define GET_UCS4_WRITER PYYJSON_CONCAT2(get_ucs4_writer, COMPILE_UCS_LEVEL)
#define GET_CUR_WRITER PYYJSON_CONCAT2(get_cur_writer, COMPILE_UCS_LEVEL)
#define MOVE_WRITER PYYJSON_CONCAT2(move_writer, COMPILE_UCS_LEVEL)
#define UPDATE_WRITE_TYPE PYYJSON_CONCAT2(update_write_type, COMPILE_UCS_LEVEL)
#define DECODE_ESCAPE_UNICODE PYYJSON_CONCAT2(decode_escape_unicode, COMPILE_UCS_LEVEL)
#define DO_SPECIAL PYYJSON_CONCAT2(do_special, COMPILE_UCS_LEVEL)
#define PROCESS_ESCAPE PYYJSON_CONCAT2(process_escape, COMPILE_UCS_LEVEL)
#define DECODE_LOOP_DONE_MAKE_STRING PYYJSON_CONCAT2(decode_loop_done_make_string, COMPILE_UCS_LEVEL)
#define GET_DONE_COUNT_FROM_MASK PYYJSON_CONCAT2(get_done_count_from_mask, COMPILE_READ_UCS_LEVEL)
#define WRITE_SIMD_IMPL_TARGET2 PYYJSON_CONCAT5(cvt_to, dst, _src_t, u16, COMPILE_SIMD_BITS)
#define WRITE_SIMD_IMPL_TARGET4 PYYJSON_CONCAT5(cvt_to, dst, _src_t, u32, COMPILE_SIMD_BITS)
#define UCS_BELOW_2_DIRTY PYYJSON_CONCAT2(ucs_below_2_dirty, COMPILE_UCS_LEVEL)
#define UCS_BELOW_4_DIRTY PYYJSON_CONCAT2(ucs_below_4_dirty, COMPILE_UCS_LEVEL)
#define COPY_WITH_ELEVATE_TO_2 PYYJSON_CONCAT2(copy_with_elevate_to_2, COMPILE_UCS_LEVEL)
#define COPY_WITH_ELEVATE_TO_4 PYYJSON_CONCAT2(copy_with_elevate_to_4, COMPILE_UCS_LEVEL)
#define CHECK_AND_RESERVE_STR_BUFFER PYYJSON_CONCAT2(check_and_reserve_str_buffer, COMPILE_UCS_LEVEL)
#define READ_NUMBER MAKE_R_NAME(read_number)


force_inline u32 GET_DONE_COUNT_FROM_MASK(SIMD_MASK_TYPE mask);

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
    Py_ssize_t ret = info->UNICODE_WRITE_PTR_NAME - (_src_t *)info->write_head;
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
    SIMD_NAME_MODIFIER(long_back_cvt_noinline_u8_u16)((u16 *)decode_unicode_info->write_head, (u8 *)decode_unicode_info->write_head, decode_unicode_info->ucs1_len);
#else
    assert(false);
    Py_UNREACHABLE();
#endif
}

force_inline void COPY_WITH_ELEVATE_TO_4(DECODE_UNICODE_INFO *decode_unicode_info) {
#if COMPILE_UCS_LEVEL < 2
    if (decode_unicode_info->ucs2_len > 0) {
        u16 *head_u16 = (u16 *)decode_unicode_info->write_head + decode_unicode_info->ucs1_len;
        SIMD_NAME_MODIFIER(long_back_cvt_noinline_u16_u32)(((u32 *)decode_unicode_info->write_head) + decode_unicode_info->ucs1_len, head_u16, decode_unicode_info->ucs2_len);
    }
    if (decode_unicode_info->ucs1_len > 0) {
        SIMD_NAME_MODIFIER(long_back_cvt_noinline_u8_u32)((u32 *)decode_unicode_info->write_head, (u8 *)decode_unicode_info->write_head, decode_unicode_info->ucs1_len);
    }
#elif COMPILE_UCS_LEVEL == 2
    assert(decode_unicode_info->ucs2_len > 0);
    SIMD_NAME_MODIFIER(long_back_cvt_noinline_u16_u32)((u32 *)decode_unicode_info->write_head, (u16 *)decode_unicode_info->write_head, decode_unicode_info->ucs2_len);
#else
    assert(false);
    Py_UNREACHABLE();
#endif
}

// force_inline void check_vector_max_char(
//         SIMD_TYPE vec,
//         ReadStrState *restrict read_state,
//         bool need_mask, /* known at compile time */
//         Py_ssize_t index /* only used when need_mask */) {
// #if COMPILE_UCS_LEVEL == PYYJSON_STRING_TYPE_ASCII
//     return;
// #endif
//     if (read_state->max_char_type == PYYJSON_STRING_TYPE_UCS4) {
//         assert(false); // logic error
//     }
//     if (need_mask) {
// #define LOAD_HEAD_MASK PYYJSON_CONCAT2(read_head_mask_table, READ_BIT_SIZE)
//         // need a mask
//         const void *mask_addr = LOAD_HEAD_MASK(index);
//         vec = SIMD_AND(load_simd(mask_addr), vec);
// #undef LOAD_HEAD_MASK
//     }
// #define CHECKER PYYJSON_SIMPLE_CONCAT3(_ucs, COMPILE_READ_UCS_LEVEL, _checkmax)
//     switch (read_state->max_char_type) {
//         case PYYJSON_STRING_TYPE_ASCII: {
// #if COMPILE_UCS_LEVEL == PYYJSON_STRING_TYPE_UCS4
//             CHECKER(vec, read_state, 0xffff, PYYJSON_STRING_TYPE_UCS4) &&
// #endif
// #if COMPILE_UCS_LEVEL >= PYYJSON_STRING_TYPE_UCS2
//                     CHECKER(vec, read_state, 0xff, PYYJSON_STRING_TYPE_UCS2) &&
// #endif
//                     CHECKER(vec, read_state, 0x7f, PYYJSON_STRING_TYPE_LATIN1);
//             break;
//         }
// #if COMPILE_UCS_LEVEL > PYYJSON_STRING_TYPE_LATIN1
//         case PYYJSON_STRING_TYPE_LATIN1: {
// #    if COMPILE_UCS_LEVEL == PYYJSON_STRING_TYPE_UCS4
//             CHECKER(vec, read_state, 0xffff, PYYJSON_STRING_TYPE_UCS4) &&
// #    endif
//                     CHECKER(vec, read_state, 0xff, PYYJSON_STRING_TYPE_UCS2);
//             break;
//         }
// #endif
// #if COMPILE_UCS_LEVEL > PYYJSON_STRING_TYPE_UCS2
//         case PYYJSON_STRING_TYPE_UCS2: {
//             CHECKER(vec, read_state, 0xffff, PYYJSON_STRING_TYPE_UCS4);
//             break;
//         }
// #endif
//         // below are unreachable
//         default: {
//             assert(false);
//             Py_UNREACHABLE();
//         }
//     }
// #undef CHECKER
// }

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

// force_inline bool verify_escape_hex(DecodeSrcInfo *decode_src_info, int offset) {
//     if (unlikely(decode_src_info->src + 4 + offset > decode_src_info->src_end)) {
//         PyErr_SetString(JSONDecodeError, "Unexpected ending when reading escaped sequence in string");
//         return false;
//     }
//     // need to verify the next 4 unicode for u16 and u32, since the size of hex conv table is 256
// #if COMPILE_READ_UCS_LEVEL == 2
//     u64 to_verify = *(u64 *)(decode_src_info->src + offset);
//     const u64 verify_mask = 0xff00ff00ff00ff00ULL;
//     if (unlikely((to_verify & verify_mask) != 0)) {
//         PyErr_SetString(JSONDecodeError, "Invalid escape sequence in string");
//         return false;
//     }
// #elif COMPILE_READ_UCS_LEVEL == 4
//     SIMD_128 to_verify = load_128((void *)(decode_src_info->src + offset));
//     const SIMD_128 verify_mask = broadcast_u64_128((i64)0xffffff00ffffff00ULL);
//     if (unlikely(!testz_128(to_verify, verify_mask))) {
//         PyErr_SetString(JSONDecodeError, "Invalid escape sequence in string");
//         return false;
//     }
// #endif
//     return true;
// }

/* noinline this to reduce binary size */
static force_noinline u32 DECODE_ESCAPE_UNICODE(DecodeSrcInfo *restrict decode_src_info) {
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
            if (unlikely(!verify_escape_hex(decode_src_info, 0) || !read_to_hex(decode_src_info->src, &hi))) {
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
                if (unlikely(!verify_escape_hex(decode_src_info, 2) || !read_to_hex(decode_src_info->src + 2, &lo))) {
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
force_inline SpecialCharReadResult DO_SPECIAL(DecodeSrcInfo *restrict decode_src_info) {
    SpecialCharReadResult result;
    _src_t u = *decode_src_info->src;
    if (likely(u == _Quote)) {
        // end of string
        decode_src_info->src++;
        result.flag = StrEnd;
        return result;
    } else if (u == _Slash) {
        result.value = DECODE_ESCAPE_UNICODE(decode_src_info);
        result.flag = StrContinue;
        if (unlikely(result.value == (u32)0xffffffff)) {
            assert(PyErr_Occurred());
            result.flag = StrInvalid;
        }
        return result;
    } else if (u < ControlMax) {
        // invalid
        PyErr_SetString(JSONDecodeError, "Invalid control character in string");
        result.flag = StrInvalid;
        return result;
    } else {
        assert(false);
        Py_UNREACHABLE();
    }
}

static force_noinline void PROCESS_ESCAPE(
        DECODE_UNICODE_INFO *decode_unicode_info,
        ReadStrState *read_state,
        DecodeSrcInfo *decode_src_info,
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
        pyyjson_memcpy(decode_unicode_info->write_head, decode_src_info->src_start, COMPILE_READ_UCS_LEVEL * copy_count);
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
        if (COMPILE_READ_UCS_LEVEL < 4) { // compile time
            // write as is 4
            assert(read_state->max_char_type == PYYJSON_STRING_TYPE_UCS4);
            assert(read_state->dont_check_max_char);
            // leave `state_dirty` as is
            DECODE_UNICODE_WRITE_ONE_CHAR(decode_unicode_info, 4, value);
            return;
        }
        // COMPILE_READ_UCS_LEVEL == 4, write_as == 4
        bool updated = 4 > read_state->max_char_type;
        read_state->max_char_type = 4;
        read_state->dont_check_max_char = true;
        read_state->state_dirty = read_state->state_dirty || updated;
        // write_as not updated
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
        DecodeSrcInfo *restrict decode_src_info,
        /* some immediate numbers*/
        int write_as, // one of 1,2,4
        bool do_copy,
        bool need_check_max_char) {
    vector_a vec = *(vector_u *)decode_src_info->src;
#if COMPILE_SIMD_BITS == 512
    avx512_bitmask_t check_mask = get_escape_bitmask(vec);
    bool checked = check_mask == 0;
#else
    vector_a check_mask = get_escape_mask(vec);
    bool checked = testz(check_mask);
#endif
    if (do_copy) {                               // compile time determined
        if (write_as > COMPILE_READ_UCS_LEVEL) { // compile time determined

            if (write_as == 2) { // compile time determined
#if COMPILE_READ_UCS_LEVEL <= 2
                assert(decode_unicode_info->unicode_ucs2);
                WRITE_SIMD_IMPL_TARGET2(GET_UCS2_WRITER(decode_unicode_info), vec);
#else
                assert(false);
                Py_UNREACHABLE();
#endif
            } else {
                assert(write_as == 4);
                assert(decode_unicode_info->unicode_ucs4);
                WRITE_SIMD_IMPL_TARGET4(GET_UCS4_WRITER(decode_unicode_info), vec);
            }

        } else { // compile time determined
            assert(write_as == COMPILE_READ_UCS_LEVEL);
            *(vector_u *)GET_CUR_WRITER(decode_unicode_info) = vec;
            // write_simd(GET_CUR_WRITER(decode_unicode_info), vec);
            // cvt_to_dst(GET_CUR_WRITER(decode_unicode_info), vec);
        }
    }

    if (checked) {
        // no special characters in this slice, won't be an ending
        // should be extremely fast if the string is long enough
        decode_src_info->src += READ_BATCH_COUNT;
        MOVE_WRITER(decode_unicode_info, write_as, READ_BATCH_COUNT);
        if (need_check_max_char) check_vector_max_char(vec, read_state, false, READ_BATCH_COUNT); // compile time determined
    } else {
        // this is not an *unlikely* case
        // for example, for short keys less than 16 bytes,
        // `QUOTE` will be found and `check_mask_zero` returns false
#if COMPILE_SIMD_BITS == 512
        usize done_count = escape_bitmask_to_done_count(check_mask);
#else
        usize done_count = escape_mask_to_done_count(check_mask);
#endif
        // u32 done_count = GET_DONE_COUNT_FROM_MASK(check_mask);
        decode_src_info->src += done_count;
        MOVE_WRITER(decode_unicode_info, write_as, done_count);
        SpecialCharReadResult escape_result = DO_SPECIAL(decode_src_info);
        if (likely(escape_result.flag == StrEnd)) {
            if (need_check_max_char) check_vector_max_char(vec, read_state, true, (Py_ssize_t)done_count);
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
            check_vector_max_char(vec, read_state, true, (Py_ssize_t)done_count);
        }
    }
}

force_inline PyObject *DECODE_LOOP_DONE_MAKE_STRING(
        DecodeSrcInfo *restrict decode_src_info,
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
#    if COMPILE_UCS_LEVEL == 4
            if (max_char_type == 2) {
                PYYJSON_CONCAT2(long_cvt_u32_u16, COMPILE_SIMD_BITS)((u16 *)decode_unicode_info->write_head, decode_src_info->src_start, copy_count);
                // downgrade_string_4_2(decode_src_info->src_start, copy_count, (u16 *)decode_unicode_info->write_head);
            } else
#    endif
                PYYJSON_CONCAT5(long, cvt, _src_t, u8, COMPILE_SIMD_BITS)((u8 *)decode_unicode_info->write_head, decode_src_info->src_start, copy_count);
            // #    define DOWNGRADER PYYJSON_CONCAT3(downgrade_string, COMPILE_UCS_LEVEL, 1)
            //                 DOWNGRADER(decode_src_info->src_start, copy_count, (u8 *)decode_unicode_info->write_head);
            // #    undef DOWNGRADER
            // DOWNGRADE_STRING((const void *)decode_src_info->src_start, copy_count, max_char_type, (_src_t *)decode_unicode_info->write_head);
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
            PYYJSON_CONCAT2(long_cvt_u32_u16, COMPILE_SIMD_BITS)((u16 *)decode_unicode_info->write_head, decode_unicode_info->write_head, copy_count);
            // downgrade_string_4_2(decode_unicode_info->write_head, copy_count, (u16 *)decode_unicode_info->write_head);
            // DOWNGRADE_STRING((const void *)decode_unicode_info->write_head, copy_count, 2, (_src_t *)decode_unicode_info->write_head);
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
            PYYJSON_CONCAT5(long, cvt, _src_t, u8, COMPILE_SIMD_BITS)((u8 *)decode_unicode_info->write_head, decode_unicode_info->write_head, copy_count);
            // #    define DOWNGRADER PYYJSON_CONCAT3(downgrade_string, COMPILE_READ_UCS_LEVEL, 1)
            //             DOWNGRADER(decode_unicode_info->write_head, copy_count, (u8 *)decode_unicode_info->write_head);
            // #    undef DOWNGRADER
            // DOWNGRADE_STRING((const void *)decode_unicode_info->write_head, copy_count, 1, (_src_t *)decode_unicode_info->write_head);
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
        DecodeSrcInfo *restrict decode_src_info,
        DECODE_UNICODE_INFO *restrict decode_unicode_info) {
#if COMPILE_UCS_LEVEL <= 1
    if (write_as <= 1) {
        assert(decode_unicode_info->unicode_ucs1);
        u8 *dst = decode_unicode_info->unicode_ucs1;
        PYYJSON_CONCAT2(trailing_copy_with_cvt_u8_u8, COMPILE_SIMD_BITS)(&dst, decode_src_info->src, really_write_count);
        // #    define TAIL_WRITER PYYJSON_CONCAT3(tail_write_simd_impl, COMPILE_READ_UCS_LEVEL, 1)
        //         TAIL_WRITER(decode_src_info->src, decode_unicode_info->unicode_ucs1, really_write_count);
        //         // decode_unicode_info->unicode_ucs1 += really_write_count;
        // #    undef TAIL_WRITER
        return;
    }
#endif
#if COMPILE_UCS_LEVEL <= 2
    if (write_as == 2) {
        // #    define TAIL_WRITER PYYJSON_CONCAT3(tail_write_simd_impl, COMPILE_READ_UCS_LEVEL, 2)
        assert(decode_unicode_info->unicode_ucs2);
        u16 *dst = decode_unicode_info->unicode_ucs2;
        PYYJSON_CONCAT5(trailing_copy_with, cvt, _src_t, u16, COMPILE_SIMD_BITS)(&dst, decode_src_info->src, really_write_count);
        // TAIL_WRITER(decode_src_info->src, decode_unicode_info->unicode_ucs2, really_write_count);
        // decode_unicode_info->unicode_ucs2 += really_write_count;
        // #    undef TAIL_WRITER
        return;
    }
#endif
    // #define TAIL_WRITER PYYJSON_CONCAT3(tail_write_simd_impl, COMPILE_READ_UCS_LEVEL, 4)

    assert(decode_unicode_info->unicode_ucs4);
    u32 *dst = decode_unicode_info->unicode_ucs4;
    PYYJSON_CONCAT5(trailing_copy_with, cvt, _src_t, u32, COMPILE_SIMD_BITS)(&dst, decode_src_info->src, really_write_count);
    // TAIL_WRITER(decode_src_info->src, decode_unicode_info->unicode_ucs4, really_write_count);
    // decode_unicode_info->unicode_ucs4 += really_write_count;
    // #undef TAIL_WRITER
}

force_inline void READ_STR_TAIL(
        DecodeSrcInfo *restrict decode_src_info,
        DECODE_UNICODE_INFO *restrict decode_unicode_info,
        ReadStrState *read_state,
        int write_as, // one of 1,2,4
        bool do_copy,
        bool need_check_max_char) {
#if COMPILE_SIMD_BITS == 512
    // load use maskz
    // #    define _MASKZ_LOADU PYYJSON_SIMPLE_CONCAT2(_mm512_maskz_loadu_epi, READ_BIT_SIZE)
    u64 rw_mask;
    avx512_bitmask_t tail_mask;
    rw_mask = len_to_maskz(decode_src_info->src_end - decode_src_info->src);
    // rw_mask = ((u64)1 << (usize)(decode_src_info->src_end - decode_src_info->src)) - 1;
    SIMD_512 vec = maskz_loadu(rw_mask, (const void *)decode_src_info->src);
    // #    undef _MASKZ_LOADU
    tail_mask = get_escape_bitmask(vec) & rw_mask;
    if (likely(tail_mask)) {
        // u32 done_count = GET_DONE_COUNT_FROM_MASK(tail_mask);
        usize done_count = escape_bitmask_to_done_count(tail_mask);
        if (do_copy && done_count) {
            PROCESS_TAIL_COPY(write_as, (Py_ssize_t)done_count, decode_src_info, decode_unicode_info);
        }
        // move reader and writer
        decode_src_info->src += done_count;
        MOVE_WRITER(decode_unicode_info, write_as, done_count);
        // get the special value (expecting '"')
        SpecialCharReadResult escape_result = DO_SPECIAL(decode_src_info);
        if (likely(escape_result.flag == StrEnd)) {
            if (need_check_max_char) {
                check_vector_max_char(vec, read_state, true, (Py_ssize_t)done_count);
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
            check_vector_max_char(vec, read_state, true, (Py_ssize_t)done_count);
        }
    } else {
        // there is no '"' until the end, the string must be invalid
        PyErr_SetString(JSONDecodeError, "Unexpected ending when reading string");
        read_state->scan_flag = StrInvalid;
    }
#else
    static_assert(sizeof(SIMD_MASK_TYPE) == sizeof(SIMD_TYPE), "sizeof(SIMD_MASK_TYPE) == sizeof(SIMD_TYPE)");
    // load backward
    assert(decode_src_info->src + READ_BATCH_COUNT > decode_src_info->src_end);
    vector_a vec;
    // simd_load_head points to the addr to load
    // always assume that the 32 bytes before `src` is readable
    const _src_t *simd_load_head = decode_src_info->src_end - READ_BATCH_COUNT;
    vec = *(const vector_u *)simd_load_head;
    vector_a check_mask = get_escape_mask(vec);
    Py_ssize_t invalid_head_count = decode_src_info->src - simd_load_head;
    vector_a tail_mask;
    // process `check_mask`, removing the invalid head content
    {
        const void *tail_mask_addr = PYYJSON_CONCAT2(read_tail_mask_table, READ_BIT_SIZE)(invalid_head_count);
        tail_mask = *(const vector_a *)tail_mask_addr; //load_simd_aligned(tail_mask_addr);
        check_mask = tail_mask & check_mask;
        // check_mask = SIMD_AND(tail_mask, check_mask);
    }
    // the read buffer is ended, there should be a '"' here
    if (likely(!testz(check_mask))) {
        u32 done_count = escape_mask_to_done_count(check_mask);
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
                check_vector_max_char(vec & tail_mask, read_state, true, (Py_ssize_t)done_count);
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
            check_vector_max_char(vec & tail_mask, read_state, true, (Py_ssize_t)done_count);
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
static force_noinline PyObject *READ_STR(
        const _src_t **restrict reader_addr, /*IN-OUT*/
        const _src_t *_reader_end,
        _src_t *_temp_write_buffer,
        bool is_key) {
    PyObject *ret;
    DECODE_UNICODE_INFO _decode_unicode_info;
    ReadStrState _read_state;
    _src_t *const temp_write_buffer = _temp_write_buffer;
    INIT_DECODE_UNICODE_INFO(&_decode_unicode_info, (u8 *)temp_write_buffer);
    init_read_state(&_read_state);
    DecodeSrcInfo _decode_src_info = {
            .src = *reader_addr,
            .src_start = *reader_addr,
            .src_end = _reader_end,
    };

    const _src_t *const last_src_batch = _decode_src_info.src_end - READ_BATCH_COUNT;

    if (unlikely(_decode_src_info.src > last_src_batch)) goto read_tail;
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
        assert(_decode_src_info.src <= last_src_batch);
        while (_decode_src_info.src <= last_src_batch) {
            READ_STR_IN_LOOP(&_decode_unicode_info,
                             &_read_state,
                             &_decode_src_info,
                             /* some immediate numbers*/
                             1, false, false);
            if (unlikely(_read_state.state_dirty)) {
                _read_state.state_dirty = false;
                if (_read_state.scan_flag == StrEnd) goto done;
                if (unlikely(_read_state.scan_flag == StrInvalid)) goto fail;
                if (unlikely(_decode_src_info.src > last_src_batch)) break;
                // escape, or max char updated

                // clang-format off
                #include "decode_loop_dirty.inl.h"
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
        assert(_decode_src_info.src <= last_src_batch);
        while (_decode_src_info.src <= last_src_batch) {
            READ_STR_IN_LOOP(&_decode_unicode_info,
                             &_read_state,
                             &_decode_src_info,
                             /* some immediate numbers*/
                             1, false, true);
            if (unlikely(_read_state.state_dirty)) {
                _read_state.state_dirty = false;
                if (_read_state.scan_flag == StrEnd) goto done;
                if (unlikely(_read_state.scan_flag == StrInvalid)) goto fail;
                if (unlikely(_decode_src_info.src > last_src_batch)) break;
                // escape, or max char updated

                // clang-format off
                #include "decode_loop_dirty.inl.h"
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
        assert(_decode_src_info.src <= last_src_batch);
        while (_decode_src_info.src <= last_src_batch) {
            READ_STR_IN_LOOP(&_decode_unicode_info,
                             &_read_state,
                             &_decode_src_info,
                             /* some immediate numbers*/
                             1, true, false);
            if (unlikely(_read_state.state_dirty)) {
                _read_state.state_dirty = false;
                if (_read_state.scan_flag == StrEnd) goto done;
                if (unlikely(_read_state.scan_flag == StrInvalid)) goto fail;
                if (unlikely(_decode_src_info.src > last_src_batch)) break;
                // escape, or max char updated

                // clang-format off
                #include "decode_loop_dirty.inl.h"
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
        assert(_read_state.max_char_type == 0);
        // BEGIN
        assert(_decode_src_info.src <= last_src_batch);
        while (_decode_src_info.src <= last_src_batch) {
            READ_STR_IN_LOOP(&_decode_unicode_info,
                             &_read_state,
                             &_decode_src_info,
                             /* some immediate numbers*/
                             1, true, true);
            if (unlikely(_read_state.state_dirty)) {
                _read_state.state_dirty = false;
                if (_read_state.scan_flag == StrEnd) goto done;
                if (unlikely(_read_state.scan_flag == StrInvalid)) goto fail;
                if (unlikely(_decode_src_info.src > last_src_batch)) break;
                // escape, or max char updated

                // clang-format off
                #include "decode_loop_dirty.inl.h"
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
        assert(_decode_src_info.src <= last_src_batch);
        while (_decode_src_info.src <= last_src_batch) {
            READ_STR_IN_LOOP(&_decode_unicode_info,
                             &_read_state,
                             &_decode_src_info,
                             /* some immediate numbers*/
                             2, false, false);
            if (unlikely(_read_state.state_dirty)) {
                _read_state.state_dirty = false;
                if (_read_state.scan_flag == StrEnd) goto done;
                if (unlikely(_read_state.scan_flag == StrInvalid)) goto fail;
                if (unlikely(_decode_src_info.src > last_src_batch)) break;
                // escape, or max char updated

                // clang-format off
                #include "decode_loop_dirty.inl.h"
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
        assert(_decode_src_info.src <= last_src_batch);
        while (_decode_src_info.src <= last_src_batch) {
            READ_STR_IN_LOOP(&_decode_unicode_info,
                             &_read_state,
                             &_decode_src_info,
                             /* some immediate numbers*/
                             2, false, true);
            if (unlikely(_read_state.state_dirty)) {
                _read_state.state_dirty = false;
                if (_read_state.scan_flag == StrEnd) goto done;
                if (unlikely(_read_state.scan_flag == StrInvalid)) goto fail;
                if (unlikely(_decode_src_info.src > last_src_batch)) break;
                // escape, or max char updated

                // clang-format off
                #include "decode_loop_dirty.inl.h"
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
        assert(_decode_src_info.src <= last_src_batch);
        while (_decode_src_info.src <= last_src_batch) {
            READ_STR_IN_LOOP(&_decode_unicode_info,
                             &_read_state,
                             &_decode_src_info,
                             /* some immediate numbers*/
                             2, true, false);
            if (unlikely(_read_state.state_dirty)) {
                _read_state.state_dirty = false;
                if (_read_state.scan_flag == StrEnd) goto done;
                if (unlikely(_read_state.scan_flag == StrInvalid)) goto fail;
                if (unlikely(_decode_src_info.src > last_src_batch)) break;
                // escape, or max char updated

                // clang-format off
                #include "decode_loop_dirty.inl.h"
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
        assert(_decode_src_info.src <= last_src_batch);
        while (_decode_src_info.src <= last_src_batch) {
            READ_STR_IN_LOOP(&_decode_unicode_info,
                             &_read_state,
                             &_decode_src_info,
                             /* some immediate numbers*/
                             2, true, true);
            if (unlikely(_read_state.state_dirty)) {
                _read_state.state_dirty = false;
                if (_read_state.scan_flag == StrEnd) goto done;
                if (unlikely(_read_state.scan_flag == StrInvalid)) goto fail;
                if (unlikely(_decode_src_info.src > last_src_batch)) break;
                // escape, or max char updated

                // clang-format off
                #include "decode_loop_dirty.inl.h"
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
        assert(_decode_src_info.src <= last_src_batch);
        while (_decode_src_info.src <= last_src_batch) {
            READ_STR_IN_LOOP(&_decode_unicode_info,
                             &_read_state,
                             &_decode_src_info,
                             /* some immediate numbers*/
                             4, false, false);
            if (unlikely(_read_state.state_dirty)) {
                _read_state.state_dirty = false;
                if (_read_state.scan_flag == StrEnd) goto done;
                if (unlikely(_read_state.scan_flag == StrInvalid)) goto fail;
                if (unlikely(_decode_src_info.src > last_src_batch)) break;
                // escape, or max char updated

                // clang-format off
                #include "decode_loop_dirty.inl.h"
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
        assert(_decode_src_info.src <= last_src_batch);
        while (_decode_src_info.src <= last_src_batch) {
            READ_STR_IN_LOOP(&_decode_unicode_info,
                             &_read_state,
                             &_decode_src_info,
                             /* some immediate numbers*/
                             4, false, true);
            if (unlikely(_read_state.state_dirty)) {
                _read_state.state_dirty = false;
                if (_read_state.scan_flag == StrEnd) goto done;
                if (unlikely(_read_state.scan_flag == StrInvalid)) goto fail;
                if (unlikely(_decode_src_info.src > last_src_batch)) break;
                // escape, or max char updated

                // clang-format off
                #include "decode_loop_dirty.inl.h"
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
        assert(_decode_src_info.src <= last_src_batch);
        while (_decode_src_info.src <= last_src_batch) {
            READ_STR_IN_LOOP(&_decode_unicode_info,
                             &_read_state,
                             &_decode_src_info,
                             /* some immediate numbers*/
                             4, true, false);
            if (_read_state.scan_flag == StrEnd) goto done;
            if (unlikely(_read_state.scan_flag == StrInvalid)) goto fail;
            if (unlikely(_decode_src_info.src > last_src_batch)) break;
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
        assert(_decode_src_info.src <= last_src_batch);
        while (_decode_src_info.src <= last_src_batch) {
            READ_STR_IN_LOOP(&_decode_unicode_info,
                             &_read_state,
                             &_decode_src_info,
                             /* some immediate numbers*/
                             4, true, true);
            if (unlikely(_read_state.state_dirty)) {
                _read_state.state_dirty = false;
                if (_read_state.scan_flag == StrEnd) goto done;
                if (unlikely(_read_state.scan_flag == StrInvalid)) goto fail;
                if (unlikely(_decode_src_info.src > last_src_batch)) break;
                // escape, or max char updated

                // clang-format off
                #include "decode_loop_dirty.inl.h"
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
        _read_state.state_dirty = false;
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

// force_inline bool cmpeq_2chars(const _src_t *cur, const _src_t *_template, const _src_t *end) {
//     return cur + 2 <= end && 0 == memcmp((const void *)cur, (const void *)_template, 2 * sizeof(_src_t));
// }

// force_inline void fast_skip_spaces(const _src_t **cur_addr, const _src_t *end) {
// #define SET1 PYYJSON_CONCAT3(broadcast, READ_BIT_SIZE, COMPILE_SIMD_BITS)
//     const vector_a template = broadcast(' ');
// #undef SET1
//     const _src_t *cur = *cur_addr;
//     assert(*cur == ' ');
// loop:;
//     if (likely(cur + READ_BATCH_COUNT < end)) {
//         vector_a vec = *(const vector_u *)cur;
// #define CMPNEQ PYYJSON_CONCAT3(cmpneq, READ_BIT_SIZE, COMPILE_SIMD_BITS)
//         SIMD_MASK_TYPE m = CMPNEQ(vec, template);
// #undef CMPNEQ
//         if (check_mask_zero(m)) {
//             cur += READ_BATCH_COUNT;
//             goto loop;
//         } else {
//             u32 done_count = GET_DONE_COUNT_FROM_MASK(m);
//             cur += done_count;
//         }
//     } else {
//         static _src_t _t[2] = {' ', ' '};
//         while (true) REPEAT_CALL_16({
//             if (cmpeq_2chars(cur, _t, end)) cur += 2;
//             else
//                 break;
//         })
//         if (*cur == ' ') cur++;
//     }
//     *cur_addr = cur;
//     assert(*cur != ' ');
// }

force_inline bool CHECK_AND_RESERVE_STR_BUFFER(Py_ssize_t len, _src_t **buffer_head_addr, bool *need_dealloc) {
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
        *buffer_head_addr = (_src_t *)(new_buffer + TAIL_PADDING);
        *need_dealloc = true;
    } else {
        *buffer_head_addr = (_src_t *)(pyyjson_string_buffer + TAIL_PADDING);
        *need_dealloc = false;
    }
    return true;
}

#define READ_ROOT_IMPL READ_ROOT_PRETTY
#define DECODE_READ_PRETTY 1
#include "decode_str_root.inl.h"
#undef DECODE_READ_PRETTY
#undef READ_ROOT_IMPL
//
#define READ_ROOT_IMPL READ_ROOT_MINIFY
#define DECODE_READ_PRETTY 0
#include "decode_str_root.inl.h"
#undef DECODE_READ_PRETTY
#undef READ_ROOT_IMPL

/** Read single value JSON document. */
static force_noinline PyObject *READ_ROOT_SINGLE(const _src_t *dat, Py_ssize_t len) {
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
    //     const _src_t *const dat =
    // #if COMPILE_UCS_LEVEL == 0
    //             (u8 *)(((PyASCIIObject *)unicode_root) + 1);
    // #else
    //             (_src_t *)(((PyCompactUnicodeObject *)unicode_root) + 1);
    // #endif
    //
    const _src_t *cur = dat;
    const _src_t *const end = cur + len;
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
        _src_t *string_buffer_head;
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
        if (likely(_read_true(&cur, end))) {
            Py_Immortal_IncRef(Py_True);
            ret = Py_True;
            goto single_end;
        }
        goto fail_literal_true;
    }
    if (*cur == 'f') {
        if (likely(_read_false(&cur, end))) {
            Py_Immortal_IncRef(Py_False);
            ret = Py_False;
            goto single_end;
        }
        goto fail_literal_false;
    }
    if (*cur == 'n') {
        if (likely(_read_null(&cur, end))) {
            Py_Immortal_IncRef(Py_None);
            ret = Py_None;
            goto single_end;
        }
        if (_read_nan(&cur, end)) {
            ret = PyFloat_FromDouble(fabs(Py_NAN));
            if (likely(ret)) goto single_end;
        }
        goto fail_literal_null;
    }
    {
        ret = read_inf_or_nan(false, &cur, end);
        if (likely(ret)) goto single_end;
    }
    goto fail_character;

single_end:
    assert(ret);
    if (unlikely(cur < end)) {
        if (*cur == ' ') fast_skip_spaces(&cur, end);
        if (*cur <= U8MAX && char_is_space(*cur)) {
            do {
                cur++;
            } while (*cur <= U8MAX && char_is_space(*cur));
        }
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

force_inline bool SHOULD_READ_PRETTY(const _src_t *buffer, const _src_t *end) {
    if (end - buffer > 3) {
        // check if can use pretty read
        _src_t second, third;
        second = buffer[1];
        third = buffer[2];
        if (second == '\n' || third == '\n') {
            // likely to hit
            return true;
        }
        if (second <= U8MAX && third <= U8MAX && char_is_space(second) && char_is_space(third)) {
            return true;
        }
    }
    return false;
}

static force_noinline PyObject *PYYJSON_DECODE_STR(PyUnicodeObject *in_unicode) {
    // some checks
    assert(in_unicode);
    PyASCIIObject *ascii_head = PYYJSON_CAST(PyASCIIObject *, in_unicode);
    assert((ascii_head->state.ascii ? 0 : ascii_head->state.kind) == COMPILE_UCS_LEVEL);
    if (unlikely(!ascii_head->length)) {
        PyErr_Format(JSONDecodeError, "input data is empty");
        return NULL;
    }
#if COMPILE_UCS_LEVEL > 0
    const _src_t *buffer = PYYJSON_CAST(_src_t *, PYYJSON_CAST(PyCompactUnicodeObject *, in_unicode) + 1);
#else
    const _src_t *buffer = PYYJSON_CAST(_src_t *, ascii_head + 1);
#endif
    assert(buffer);
    assert(ascii_head->length > 0);

    const _src_t *const end = buffer + ascii_head->length;
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
        //     bool should_read_pretty = false;
        //     if (end - buffer > 3) {
        //         // check if can use pretty read
        //         _src_t second, third;
        //         second = buffer[1];
        //         third = buffer[2];
        //         if (second == '\n' || third == '\n') {
        //             should_read_pretty = true;
        //             goto start_doc_read;
        //         }
        //         if (second <= U8MAX && third <= U8MAX && char_is_space(second) && char_is_space(third)) {
        //             should_read_pretty = true;
        //             goto start_doc_read;
        //         }
        //     }
        // //
        // start_doc_read:;
        if (SHOULD_READ_PRETTY(buffer, end)) {
            ret = READ_ROOT_PRETTY(buffer, end - buffer);
        } else {
            ret = READ_ROOT_MINIFY(buffer, end - buffer);
        }
    } else {
        ret = READ_ROOT_SINGLE(buffer, end - buffer);
    }

    return ret;
}

#undef READ_NUMBER
#undef CHECK_AND_RESERVE_STR_BUFFER
#undef COPY_WITH_ELEVATE_TO_4
#undef COPY_WITH_ELEVATE_TO_2
#undef UCS_BELOW_4_DIRTY
#undef UCS_BELOW_2_DIRTY
#undef WRITE_SIMD_IMPL_TARGET4
#undef WRITE_SIMD_IMPL_TARGET2
#undef GET_DONE_COUNT_FROM_MASK
#undef DECODE_LOOP_DONE_MAKE_STRING
#undef PROCESS_ESCAPE
#undef DO_SPECIAL
#undef DECODE_ESCAPE_UNICODE
#undef UPDATE_WRITE_TYPE
#undef MOVE_WRITER
#undef GET_CUR_WRITER
#undef GET_UCS4_WRITER
#undef GET_UCS2_WRITER
#undef DECODE_UNICODE_WRITE_ONE_CHAR
#undef UNICODE_DECODE_GET_COPY_COUNT
#undef INIT_DECODE_UNICODE_INFO
#undef DECODE_UNICODE_INFO
// #undef read_to_hex
#undef READ_STR_TAIL
#undef PROCESS_TAIL_COPY
#undef READ_STR_IN_LOOP
#undef READ_ROOT_SINGLE
#undef READ_ROOT_PRETTY
// #undef fast_skip_spaces
// #undef cmpeq_2chars
#undef READ_STR
#undef SHOULD_READ_PRETTY
#undef PYYJSON_DECODE_STR
//
#undef COMPILE_READ_UCS_LEVEL
#include "compile_context/sr_out.inl.h"
