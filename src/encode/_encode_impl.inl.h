#ifdef PYYJSON_CLANGD_DUMMY
#    ifndef COMPILE_UCS_LEVEL
#        define COMPILE_UCS_LEVEL 0
#    endif
#    ifndef COMPILE_INDENT_LEVEL
#        define COMPILE_INDENT_LEVEL 2
#    endif
#endif

#ifndef COMPILE_UCS_LEVEL
#    error "COMPILE_UCS_LEVEL is not defined"
#endif

#ifndef COMPILE_INDENT_LEVEL
#    error "COMPILE_INDENT_LEVEL is not defined"
#endif

#if COMPILE_UCS_LEVEL <= 1
#    define COMPILE_READ_UCS_LEVEL 1
#    define COMPILE_WRITE_UCS_LEVEL 1
#else
#    define COMPILE_READ_UCS_LEVEL COMPILE_UCS_LEVEL
#    define COMPILE_WRITE_UCS_LEVEL COMPILE_UCS_LEVEL
#endif
#include "unicode/indent_wrap.h"
//
#include "commondef/iw_in.inl.h"
//
#include "commondef/w_out.inl.h"
//
#include "commondef/sw_in.inl.h"


#define WRITE_INDENT_RETURN_IF_FAIL(_unicode_buffer_info_, _cur_nested_depth_, _is_in_obj_, _additional_reserve_count_)                         \
    do {                                                                                                                                        \
        if (unlikely(!unicode_indent_writer(_unicode_buffer_info_, _cur_nested_depth_, _is_in_obj_, _additional_reserve_count_))) return false; \
    } while (0)


#define _PREPARE_UNICODE_WRITE PYYJSON_CONCAT3(prepare_unicode_write, COMPILE_INDENT_LEVEL, COMPILE_UCS_LEVEL)

force_inline void _PREPARE_UNICODE_WRITE(PyObject *obj, EncodeUnicodeBufferInfo *unicode_buffer_info, UnicodeInfo *restrict unicode_info, Py_ssize_t *out_len, int *read_kind, int *write_kind) {
    Py_ssize_t out_len_val = PyUnicode_GET_LENGTH(obj);
    *out_len = out_len_val;
    int read_kind_val = PyUnicode_KIND(obj);
    *read_kind = read_kind_val;

#if COMPILE_UCS_LEVEL == 4
    *write_kind = 4;
#elif COMPILE_UCS_LEVEL == 2
    *write_kind = 2;
    if (unlikely(read_kind_val == 4)) {
        memorize_ucs2_to_ucs4(unicode_buffer_info, unicode_info);
        *write_kind = 4;
    }
#elif COMPILE_UCS_LEVEL == 1
    *write_kind = 1;
    if (unlikely(read_kind_val == 2)) {
        memorize_ucs1_to_ucs2(unicode_buffer_info, unicode_info);
        *write_kind = 2;
    } else if (unlikely(read_kind_val == 4)) {
        memorize_ucs1_to_ucs4(unicode_buffer_info, unicode_info);
        *write_kind = 4;
    }
#elif COMPILE_UCS_LEVEL == 0
    *write_kind = 0;
    if (unlikely(read_kind_val == 2)) {
        memorize_ascii_to_ucs2(unicode_buffer_info, unicode_info);
        *write_kind = 2;
    } else if (unlikely(read_kind_val == 4)) {
        memorize_ascii_to_ucs4(unicode_buffer_info, unicode_info);
        *write_kind = 4;
    } else if (unlikely(!PyUnicode_IS_ASCII(obj))) {
        memorize_ascii_to_ucs1(unicode_buffer_info, unicode_info);
        *write_kind = 1;
    }
#endif
}

#define UNICODE_BUFFER_APPEND_KEY PYYJSON_CONCAT3(unicode_buffer_append_key, COMPILE_INDENT_LEVEL, COMPILE_UCS_LEVEL)

force_inline bool UNICODE_BUFFER_APPEND_KEY(PyObject *key, EncodeUnicodeBufferInfo *unicode_buffer_info, UnicodeInfo *unicode_info, Py_ssize_t cur_nested_depth) {
    Py_ssize_t len;
    int kind, write_kind;
    // bool _c;
    _PREPARE_UNICODE_WRITE(key, unicode_buffer_info, unicode_info, &len, &kind, &write_kind);

    switch (write_kind) {
#if COMPILE_UCS_LEVEL < 1
        case 0:
#endif
#if COMPILE_UCS_LEVEL < 2
        case 1: {
            RETURN_ON_UNLIKELY_ERR(!PYYJSON_CONCAT4(unicode_buffer_append_key_internal, COMPILE_INDENT_LEVEL, 1, 1)(key, len, unicode_buffer_info, cur_nested_depth));
            break;
        }
#endif
#if COMPILE_UCS_LEVEL < 4
        case 2: {
            switch (kind) {
                case 1: {
                    RETURN_ON_UNLIKELY_ERR(!PYYJSON_CONCAT4(unicode_buffer_append_key_internal, COMPILE_INDENT_LEVEL, 1, 2)(key, len, unicode_buffer_info, cur_nested_depth));
                    break;
                }
                case 2: {
                    RETURN_ON_UNLIKELY_ERR(!PYYJSON_CONCAT4(unicode_buffer_append_key_internal, COMPILE_INDENT_LEVEL, 2, 2)(key, len, unicode_buffer_info, cur_nested_depth));
                    break;
                }
                default: {
                    assert(false);
                    Py_UNREACHABLE();
                }
            }
            break;
        }
#endif
        case 4: {
            switch (kind) {
                case 1: {
                    RETURN_ON_UNLIKELY_ERR(!PYYJSON_CONCAT4(unicode_buffer_append_key_internal, COMPILE_INDENT_LEVEL, 1, 4)(key, len, unicode_buffer_info, cur_nested_depth));
                    break;
                }
                case 2: {
                    RETURN_ON_UNLIKELY_ERR(!PYYJSON_CONCAT4(unicode_buffer_append_key_internal, COMPILE_INDENT_LEVEL, 2, 4)(key, len, unicode_buffer_info, cur_nested_depth));
                    break;
                }
                case 4: {
                    RETURN_ON_UNLIKELY_ERR(!PYYJSON_CONCAT4(unicode_buffer_append_key_internal, COMPILE_INDENT_LEVEL, 4, 4)(key, len, unicode_buffer_info, cur_nested_depth));
                    break;
                }
                default: {
                    assert(false);
                    Py_UNREACHABLE();
                }
            }
            break;
        }
        default: {
            assert(false);
            Py_UNREACHABLE();
        }
    }
    return true;
}

#define UNICODE_BUFFER_APPEND_STR PYYJSON_CONCAT3(unicode_buffer_append_str, COMPILE_INDENT_LEVEL, COMPILE_UCS_LEVEL)

force_inline bool UNICODE_BUFFER_APPEND_STR(PyObject *val, EncodeUnicodeBufferInfo *unicode_buffer_info, UnicodeInfo *unicode_info, Py_ssize_t cur_nested_depth, bool is_in_obj) {
    Py_ssize_t len;
    int kind, write_kind;
    // bool _c;
    _PREPARE_UNICODE_WRITE(val, unicode_buffer_info, unicode_info, &len, &kind, &write_kind);
    switch (write_kind) {
#if COMPILE_UCS_LEVEL < 1
        case 0:
#endif
#if COMPILE_UCS_LEVEL < 2
        case 1: {
            RETURN_ON_UNLIKELY_ERR(!PYYJSON_CONCAT4(unicode_buffer_append_str_internal, COMPILE_INDENT_LEVEL, 1, 1)(val, len, unicode_buffer_info, cur_nested_depth, is_in_obj));
            break;
        }
#endif
#if COMPILE_UCS_LEVEL < 4
        case 2: {
            switch (kind) {
                case 1: {
                    RETURN_ON_UNLIKELY_ERR(!PYYJSON_CONCAT4(unicode_buffer_append_str_internal, COMPILE_INDENT_LEVEL, 1, 2)(val, len, unicode_buffer_info, cur_nested_depth, is_in_obj));
                    break;
                }
                case 2: {
                    RETURN_ON_UNLIKELY_ERR(!PYYJSON_CONCAT4(unicode_buffer_append_str_internal, COMPILE_INDENT_LEVEL, 2, 2)(val, len, unicode_buffer_info, cur_nested_depth, is_in_obj));
                    break;
                }
                default: {
                    assert(false);
                    Py_UNREACHABLE();
                }
            }
            break;
        }
#endif
        case 4: {
            switch (kind) {
                case 1: {
                    RETURN_ON_UNLIKELY_ERR(!PYYJSON_CONCAT4(unicode_buffer_append_str_internal, COMPILE_INDENT_LEVEL, 1, 4)(val, len, unicode_buffer_info, cur_nested_depth, is_in_obj));
                    break;
                }
                case 2: {
                    RETURN_ON_UNLIKELY_ERR(!PYYJSON_CONCAT4(unicode_buffer_append_str_internal, COMPILE_INDENT_LEVEL, 2, 4)(val, len, unicode_buffer_info, cur_nested_depth, is_in_obj));
                    break;
                }
                case 4: {
                    RETURN_ON_UNLIKELY_ERR(!PYYJSON_CONCAT4(unicode_buffer_append_str_internal, COMPILE_INDENT_LEVEL, 4, 4)(val, len, unicode_buffer_info, cur_nested_depth, is_in_obj));
                    break;
                }
                default: {
                    assert(false);
                    Py_UNREACHABLE();
                }
            }
            break;
        }
        default: {
            assert(false);
            Py_UNREACHABLE();
        }
    }
    return true;
}

#define UNICODE_BUFFER_APPEND_LONG PYYJSON_CONCAT3(unicode_buffer_append_long, COMPILE_INDENT_LEVEL, COMPILE_UCS_LEVEL)

force_inline bool UNICODE_BUFFER_APPEND_LONG(EncodeUnicodeBufferInfo *unicode_buffer_info, Py_ssize_t cur_nested_depth, PyObject *val, bool is_in_obj) {
    assert(PyLong_CheckExact(val));
    // 32 < TAIL_PADDING == 64 so this is enough
    WRITE_INDENT_RETURN_IF_FAIL(unicode_buffer_info, cur_nested_depth, is_in_obj, TAIL_PADDING);

    if (pylong_is_zero(val)) {
        _dst_t *writer = _WRITER(unicode_buffer_info);
        *writer++ = '0';
        *writer++ = ',';
        _WRITER(unicode_buffer_info) += 2;
    } else {
        u64 v;
        usize sign;
        if (pylong_is_unsigned(val)) {
            if (unlikely(!pylong_value_unsigned(val, &v))) {
                PyErr_SetString(JSONEncodeError, "convert value to unsigned long long failed");
                return false;
            }
            sign = 0;
        } else {
            i64 v2;
            if (unlikely(!pylong_value_signed(val, &v2))) {
                PyErr_SetString(JSONEncodeError, "convert value to long long failed");
                return false;
            }
            assert(v2 <= 0);
            v = -v2;
            sign = 1;
        }
        WRITE_UNICODE_U64(&_WRITER(unicode_buffer_info), v, sign);
        *_WRITER(unicode_buffer_info)++ = ',';
    }
    assert(check_unicode_writer_valid(unicode_buffer_info));
    return true;
}

#define WRITE_UNICODE_FALSE PYYJSON_CONCAT3(write_unicode_false, COMPILE_INDENT_LEVEL, COMPILE_UCS_LEVEL)

force_inline void WRITE_UNICODE_FALSE(_dst_t **writer_addr) {
    _dst_t *writer = *writer_addr;
    //   6,12,24
    //-> 8,16,24/32 (64)
    //-> 8,12,24 (32)
    *writer++ = 'f';
    *writer++ = 'a';
    *writer++ = 'l';
    *writer++ = 's';
    *writer++ = 'e';
    *writer++ = ',';
    _dst_t *writer2 = writer;
#if COMPILE_UCS_LEVEL == 1
    *writer2++ = 0;
    *writer2++ = 0;
#elif COMPILE_UCS_LEVEL == 2
#    if SIZEOF_VOID_P == 8
    *writer2++ = 0;
    *writer2++ = 0;
#    endif // SIZEOF_VOID_P
#else      // COMPILE_UCS_LEVEL == 4
#    if __AVX__
    *writer2++ = 0;
    *writer2++ = 0;
#    endif // __AVX__
#endif     // COMPILE_UCS_LEVEL
    *writer_addr = writer;
}

#define UNICODE_BUFFER_APPEND_FALSE PYYJSON_CONCAT3(unicode_buffer_append_false, COMPILE_INDENT_LEVEL, COMPILE_UCS_LEVEL)

force_inline bool UNICODE_BUFFER_APPEND_FALSE(EncodeUnicodeBufferInfo *unicode_buffer_info, Py_ssize_t cur_nested_depth, bool is_in_obj) {
    WRITE_INDENT_RETURN_IF_FAIL(unicode_buffer_info, cur_nested_depth, is_in_obj, TAIL_PADDING);
    WRITE_UNICODE_FALSE(&_WRITER(unicode_buffer_info));
    return true;
}

#define WRITE_UNICODE_TRUE PYYJSON_CONCAT3(write_unicode_true, COMPILE_INDENT_LEVEL, COMPILE_UCS_LEVEL)

force_inline void WRITE_UNICODE_TRUE(_dst_t **writer_addr) {
    _dst_t *writer = *writer_addr;
    _dst_t *writer2 = writer;
    //   5,10,20
    //-> 8,16,24/32 (64)
    //-> 8,12,20 (32)
    *writer++ = 't';
    *writer++ = 'r';
    *writer++ = 'u';
    *writer++ = 'e';
    *writer++ = ',';
#if COMPILE_UCS_LEVEL == 1
    *writer++ = 0;
    *writer++ = 0;
    *writer++ = 0;
#elif COMPILE_UCS_LEVEL == 2
    *writer++ = 0;
#    if SIZEOF_VOID_P == 8
    *writer++ = 0;
    *writer++ = 0;
#    endif // SIZEOF_VOID_P == 8
#else      // COMPILE_UCS_LEVEL == 4
#    if SIZEOF_VOID_P == 8
    *writer++ = 0;
#        if __AVX__
    *writer++ = 0;
    *writer++ = 0;
#        endif // __AVX__
#    endif
#endif // COMPILE_UCS_LEVEL
    *writer_addr = writer2 + 5;
}

#define UNICODE_BUFFER_APPEND_TRUE PYYJSON_CONCAT3(unicode_buffer_append_true, COMPILE_INDENT_LEVEL, COMPILE_UCS_LEVEL)

force_inline bool UNICODE_BUFFER_APPEND_TRUE(EncodeUnicodeBufferInfo *unicode_buffer_info, Py_ssize_t cur_nested_depth, bool is_in_obj) {
    WRITE_INDENT_RETURN_IF_FAIL(unicode_buffer_info, cur_nested_depth, is_in_obj, TAIL_PADDING);
    WRITE_UNICODE_TRUE(&_WRITER(unicode_buffer_info));
    return true;
}

#define WRITE_UNICODE_NULL PYYJSON_CONCAT3(write_unicode_null, COMPILE_INDENT_LEVEL, COMPILE_UCS_LEVEL)

force_inline void WRITE_UNICODE_NULL(_dst_t **writer_addr) {
    _dst_t *writer = *writer_addr;
    _dst_t *writer2 = writer;
    //   5,10,20
    //-> 8,16,24/32 (64)
    //-> 8,12,20 (32)
    *writer++ = 'n';
    *writer++ = 'u';
    *writer++ = 'l';
    *writer++ = 'l';
    *writer++ = ',';
#if COMPILE_UCS_LEVEL == 1
    *writer++ = 0;
    *writer++ = 0;
    *writer++ = 0;
#elif COMPILE_UCS_LEVEL == 2
    *writer++ = 0;
#    if SIZEOF_VOID_P == 8
    *writer++ = 0;
    *writer++ = 0;
#    endif // SIZEOF_VOID_P == 8
#else      // COMPILE_UCS_LEVEL == 4
#    if SIZEOF_VOID_P == 8
    *writer++ = 0;
#        if __AVX__
    *writer++ = 0;
    *writer++ = 0;
#        endif // __AVX__
#    endif
#endif // COMPILE_UCS_LEVEL
    *writer_addr = writer2 + 5;
}

#define UNICODE_BUFFER_APPEND_NULL PYYJSON_CONCAT3(unicode_buffer_append_null, COMPILE_INDENT_LEVEL, COMPILE_UCS_LEVEL)

force_inline bool UNICODE_BUFFER_APPEND_NULL(EncodeUnicodeBufferInfo *unicode_buffer_info, Py_ssize_t cur_nested_depth, bool is_in_obj) {
    WRITE_INDENT_RETURN_IF_FAIL(unicode_buffer_info, cur_nested_depth, is_in_obj, TAIL_PADDING);
    WRITE_UNICODE_NULL(&_WRITER(unicode_buffer_info));
    return true;
}

#define UNICODE_BUFFER_APPEND_FLOAT PYYJSON_CONCAT3(unicode_buffer_append_float, COMPILE_INDENT_LEVEL, COMPILE_UCS_LEVEL)

force_inline bool UNICODE_BUFFER_APPEND_FLOAT(EncodeUnicodeBufferInfo *unicode_buffer_info, Py_ssize_t cur_nested_depth, PyObject *val, bool is_in_obj) {
    WRITE_INDENT_RETURN_IF_FAIL(unicode_buffer_info, cur_nested_depth, is_in_obj, TAIL_PADDING);
    double v = PyFloat_AS_DOUBLE(val);
    u64 raw = *PYYJSON_CAST(u64 *, &v); //(u64 *)&v;
    WRITE_UNICODE_F64(&_WRITER(unicode_buffer_info), raw);
    *_WRITER(unicode_buffer_info)++ = ',';
    return true;
}

#define WRITE_UNICODE_EMPTY_ARR PYYJSON_CONCAT3(write_unicode_empty_arr, COMPILE_INDENT_LEVEL, COMPILE_UCS_LEVEL)

force_inline void WRITE_UNICODE_EMPTY_ARR(_dst_t **writer_addr) {
    _dst_t *writer = *writer_addr;
    //   3,6,12
    //-> 4,8,16 (64)
    //-> 4,8,12 (32)
    *writer++ = '[';
    *writer++ = ']';
    *writer++ = ',';
#if SIZEOF_VOID_P == 8 || COMPILE_UCS_LEVEL != 4
    *writer = 0;
#endif
    *writer_addr = writer;
}

#define UNICODE_BUFFER_APPEND_EMPTY_ARR PYYJSON_CONCAT3(unicode_buffer_append_empty_arr, COMPILE_INDENT_LEVEL, COMPILE_UCS_LEVEL)

force_inline bool UNICODE_BUFFER_APPEND_EMPTY_ARR(EncodeUnicodeBufferInfo *unicode_buffer_info, Py_ssize_t cur_nested_depth, bool is_in_obj) {
    WRITE_INDENT_RETURN_IF_FAIL(unicode_buffer_info, cur_nested_depth, is_in_obj, TAIL_PADDING);
    WRITE_UNICODE_EMPTY_ARR(&_WRITER(unicode_buffer_info));
    return true;
}

#define WRITE_UNICODE_ARR_BEGIN PYYJSON_CONCAT3(write_unicode_arr_begin, COMPILE_INDENT_LEVEL, COMPILE_UCS_LEVEL)

force_inline void WRITE_UNICODE_ARR_BEGIN(_dst_t **writer_addr) {
    *(*writer_addr)++ = '[';
}

#define UNICODE_BUFFER_APPEND_ARR_BEGIN PYYJSON_CONCAT3(unicode_buffer_append_arr_begin, COMPILE_INDENT_LEVEL, COMPILE_UCS_LEVEL)

force_inline bool UNICODE_BUFFER_APPEND_ARR_BEGIN(EncodeUnicodeBufferInfo *unicode_buffer_info, Py_ssize_t cur_nested_depth, bool is_in_obj) {
    WRITE_INDENT_RETURN_IF_FAIL(unicode_buffer_info, cur_nested_depth, is_in_obj, TAIL_PADDING);
    WRITE_UNICODE_ARR_BEGIN(&_WRITER(unicode_buffer_info));
    return true;
}

#define WRITE_UNICODE_EMPTY_OBJ PYYJSON_CONCAT3(write_unicode_empty_obj, COMPILE_INDENT_LEVEL, COMPILE_UCS_LEVEL)

force_inline void WRITE_UNICODE_EMPTY_OBJ(_dst_t **writer_addr) {
    _dst_t *writer = *writer_addr;
    //   3,6,12
    //-> 4,8,16 (64)
    //-> 4,8,12 (32)
    *writer++ = '{';
    *writer++ = '}';
    *writer++ = ',';
#if SIZEOF_VOID_P == 8 || COMPILE_UCS_LEVEL != 4
    *writer = 0;
#endif
    *writer_addr = writer;
}

#define UNICODE_BUFFER_APPEND_EMPTY_OBJ PYYJSON_CONCAT3(unicode_buffer_append_empty_obj, COMPILE_INDENT_LEVEL, COMPILE_UCS_LEVEL)

force_inline bool UNICODE_BUFFER_APPEND_EMPTY_OBJ(EncodeUnicodeBufferInfo *unicode_buffer_info, Py_ssize_t cur_nested_depth, bool is_in_obj) {
    WRITE_INDENT_RETURN_IF_FAIL(unicode_buffer_info, cur_nested_depth, is_in_obj, TAIL_PADDING);
    WRITE_UNICODE_EMPTY_OBJ(&_WRITER(unicode_buffer_info));
    return true;
}

#define WRITE_UNICODE_OBJ_BEGIN PYYJSON_CONCAT3(write_unicode_obj_begin, COMPILE_INDENT_LEVEL, COMPILE_UCS_LEVEL)

force_inline void WRITE_UNICODE_OBJ_BEGIN(_dst_t **writer_addr) {
    *(*writer_addr)++ = '{';
}

#define UNICODE_BUFFER_APPEND_OBJ_BEGIN PYYJSON_CONCAT3(unicode_buffer_append_obj_begin, COMPILE_INDENT_LEVEL, COMPILE_UCS_LEVEL)

force_inline bool UNICODE_BUFFER_APPEND_OBJ_BEGIN(EncodeUnicodeBufferInfo *unicode_buffer_info, Py_ssize_t cur_nested_depth, bool is_in_obj) {
    WRITE_INDENT_RETURN_IF_FAIL(unicode_buffer_info, cur_nested_depth, is_in_obj, TAIL_PADDING);
    WRITE_UNICODE_OBJ_BEGIN(&_WRITER(unicode_buffer_info));
    return true;
}

#define WRITE_UNICODE_OBJ_END PYYJSON_CONCAT3(write_unicode_obj_end, COMPILE_INDENT_LEVEL, COMPILE_UCS_LEVEL)

force_inline void WRITE_UNICODE_OBJ_END(_dst_t **writer_addr) {
    _dst_t *writer = *writer_addr;
    *writer++ = '}';
    *writer++ = ',';
    *writer_addr = writer;
}

#define UNICODE_BUFFER_APPEND_OBJ_END PYYJSON_CONCAT3(unicode_buffer_append_obj_end, COMPILE_INDENT_LEVEL, COMPILE_UCS_LEVEL)

force_inline bool UNICODE_BUFFER_APPEND_OBJ_END(EncodeUnicodeBufferInfo *unicode_buffer_info, Py_ssize_t cur_nested_depth) {
    // remove last comma
    (_WRITER(unicode_buffer_info))--;
    // this is not a *value*, the indent is always needed. i.e. `is_in_obj` should always pass false
    WRITE_INDENT_RETURN_IF_FAIL(unicode_buffer_info, cur_nested_depth, false, TAIL_PADDING);
    WRITE_UNICODE_OBJ_END(&_WRITER(unicode_buffer_info));
    return true;
}

#define WRITE_UNICODE_ARR_END PYYJSON_CONCAT3(write_unicode_arr_end, COMPILE_INDENT_LEVEL, COMPILE_UCS_LEVEL)

force_inline void WRITE_UNICODE_ARR_END(_dst_t **writer_addr) {
    _dst_t *writer = *writer_addr;
    *writer++ = ']';
    *writer++ = ',';
    *writer_addr = writer;
}

#define UNICODE_BUFFER_APPEND_ARR_END PYYJSON_CONCAT3(unicode_buffer_append_arr_end, COMPILE_INDENT_LEVEL, COMPILE_UCS_LEVEL)

force_inline bool UNICODE_BUFFER_APPEND_ARR_END(EncodeUnicodeBufferInfo *unicode_buffer_info, Py_ssize_t cur_nested_depth) {
    // remove last comma
    (_WRITER(unicode_buffer_info))--;
    // this is not a *value*, the indent is always needed. i.e. `is_in_obj` should always pass false
    WRITE_INDENT_RETURN_IF_FAIL(unicode_buffer_info, cur_nested_depth, false, TAIL_PADDING);
    WRITE_UNICODE_ARR_END(&_WRITER(unicode_buffer_info));
    return true;
}

#define GET_UNICODE_BUFFER_FINAL_LEN PYYJSON_CONCAT2(get_unicode_buffer_final_len, COMPILE_UCS_LEVEL)
#if COMPILE_INDENT_LEVEL == 0
// avoid compile again
force_inline Py_ssize_t GET_UNICODE_BUFFER_FINAL_LEN(EncodeUnicodeBufferInfo *unicode_buffer_info) {
#    if COMPILE_UCS_LEVEL == 0
    return unicode_buffer_info->writer.writer_u8 - (u8 *)GET_VEC_ASCII_START(unicode_buffer_info);
#    elif COMPILE_UCS_LEVEL == 1
    return unicode_buffer_info->writer.writer_u8 - (u8 *)GET_VEC_COMPACT_START(unicode_buffer_info);
#    elif COMPILE_UCS_LEVEL == 2
    return unicode_buffer_info->writer.writer_u16 - (u16 *)GET_VEC_COMPACT_START(unicode_buffer_info);
#    elif COMPILE_UCS_LEVEL == 4
    return unicode_buffer_info->writer.writer_u32 - (u32 *)GET_VEC_COMPACT_START(unicode_buffer_info);
#    endif
}
#endif

#define ENCODE_PROCESS_VAL PYYJSON_CONCAT3(encode_process_val, COMPILE_INDENT_LEVEL, COMPILE_UCS_LEVEL)

force_inline EncodeValJumpFlag ENCODE_PROCESS_VAL(
        EncodeUnicodeBufferInfo *unicode_buffer_info, PyObject *val,
        EncodeStackVars *stack_vars, bool is_in_obj) {
#define CTN_SIZE_GROW()                                                               \
    do {                                                                              \
        if (unlikely(stack_vars->cur_nested_depth == PYYJSON_ENCODE_MAX_RECURSION)) { \
            PyErr_SetString(JSONEncodeError, "Too many nested structures");           \
            return JumpFlag_Fail;                                                     \
        }                                                                             \
    } while (0)
#define RETURN_JUMP_FAIL_ON_UNLIKELY_ERR(_cond_) \
    do {                                         \
        if (unlikely(_cond_)) {                  \
            return JumpFlag_Fail;                \
        }                                        \
    } while (0)

    PyFastTypes fast_type = fast_type_check(val);

    switch (fast_type) {
        case T_Unicode: {
            RETURN_JUMP_FAIL_ON_UNLIKELY_ERR(!UNICODE_BUFFER_APPEND_STR(val, unicode_buffer_info, &stack_vars->unicode_info, stack_vars->cur_nested_depth, is_in_obj));
#if COMPILE_UCS_LEVEL < 1
            if (unlikely(stack_vars->unicode_info.cur_ucs_type == 1)) {
                return is_in_obj ? JumpFlag_Elevate1_ObjVal : JumpFlag_Elevate1_ArrVal;
            }
#endif
#if COMPILE_UCS_LEVEL < 2
            if (unlikely(stack_vars->unicode_info.cur_ucs_type == 2)) {
                return is_in_obj ? JumpFlag_Elevate2_ObjVal : JumpFlag_Elevate2_ArrVal;
            }
#endif
#if COMPILE_UCS_LEVEL < 4
            if (unlikely(stack_vars->unicode_info.cur_ucs_type == 4)) {
                return is_in_obj ? JumpFlag_Elevate4_ObjVal : JumpFlag_Elevate4_ArrVal;
            }
#endif
            break;
        }
        case T_Long: {
            RETURN_JUMP_FAIL_ON_UNLIKELY_ERR(!UNICODE_BUFFER_APPEND_LONG(unicode_buffer_info, stack_vars->cur_nested_depth, val, is_in_obj));
            break;
        }
        case T_False: {
            RETURN_JUMP_FAIL_ON_UNLIKELY_ERR(!UNICODE_BUFFER_APPEND_FALSE(unicode_buffer_info, stack_vars->cur_nested_depth, is_in_obj));
            break;
        }
        case T_True: {
            RETURN_JUMP_FAIL_ON_UNLIKELY_ERR(!UNICODE_BUFFER_APPEND_TRUE(unicode_buffer_info, stack_vars->cur_nested_depth, is_in_obj));
            break;
        }
        case T_None: {
            RETURN_JUMP_FAIL_ON_UNLIKELY_ERR(!UNICODE_BUFFER_APPEND_NULL(unicode_buffer_info, stack_vars->cur_nested_depth, is_in_obj));
            break;
        }
        case T_Float: {
            RETURN_JUMP_FAIL_ON_UNLIKELY_ERR(!UNICODE_BUFFER_APPEND_FLOAT(unicode_buffer_info, stack_vars->cur_nested_depth, val, is_in_obj));
            break;
        }
        case T_List: {
            Py_ssize_t this_list_size = PyList_GET_SIZE(val);
            if (unlikely(this_list_size == 0)) {
                RETURN_JUMP_FAIL_ON_UNLIKELY_ERR(!UNICODE_BUFFER_APPEND_EMPTY_ARR(unicode_buffer_info, stack_vars->cur_nested_depth, is_in_obj));
            } else {
                RETURN_JUMP_FAIL_ON_UNLIKELY_ERR(!UNICODE_BUFFER_APPEND_ARR_BEGIN(unicode_buffer_info, stack_vars->cur_nested_depth, is_in_obj));
                CTN_SIZE_GROW();
                EncodeCtnWithIndex *cur_write_ctn = stack_vars->ctn_stack + (stack_vars->cur_nested_depth++);
                cur_write_ctn->ctn = stack_vars->cur_obj;
                cur_write_ctn->index = stack_vars->cur_pos;
                stack_vars->cur_obj = val;
                stack_vars->cur_pos = 0;
                stack_vars->cur_list_size = this_list_size;
                return JumpFlag_ArrValBegin;
            }
            break;
        }
        case T_Dict: {
            if (unlikely(PyDict_GET_SIZE(val) == 0)) {
                RETURN_JUMP_FAIL_ON_UNLIKELY_ERR(!UNICODE_BUFFER_APPEND_EMPTY_OBJ(unicode_buffer_info, stack_vars->cur_nested_depth, is_in_obj));
            } else {
                RETURN_JUMP_FAIL_ON_UNLIKELY_ERR(!UNICODE_BUFFER_APPEND_OBJ_BEGIN(unicode_buffer_info, stack_vars->cur_nested_depth, is_in_obj));
                CTN_SIZE_GROW();
                EncodeCtnWithIndex *cur_write_ctn = stack_vars->ctn_stack + (stack_vars->cur_nested_depth++);
                cur_write_ctn->ctn = stack_vars->cur_obj;
                cur_write_ctn->index = stack_vars->cur_pos;
                stack_vars->cur_obj = val;
                stack_vars->cur_pos = 0;
                return JumpFlag_DictPairBegin;
            }
            break;
        }
        case T_Tuple: {
            Py_ssize_t this_list_size = PyTuple_Size(val);
            if (unlikely(this_list_size == 0)) {
                RETURN_JUMP_FAIL_ON_UNLIKELY_ERR(!UNICODE_BUFFER_APPEND_EMPTY_ARR(unicode_buffer_info, stack_vars->cur_nested_depth, is_in_obj));
            } else {
                RETURN_JUMP_FAIL_ON_UNLIKELY_ERR(!UNICODE_BUFFER_APPEND_ARR_BEGIN(unicode_buffer_info, stack_vars->cur_nested_depth, is_in_obj));
                CTN_SIZE_GROW();
                EncodeCtnWithIndex *cur_write_ctn = stack_vars->ctn_stack + (stack_vars->cur_nested_depth++);
                cur_write_ctn->ctn = stack_vars->cur_obj;
                cur_write_ctn->index = stack_vars->cur_pos;
                stack_vars->cur_obj = val;
                stack_vars->cur_pos = 0;
                stack_vars->cur_list_size = this_list_size;
                return JumpFlag_TupleValBegin;
            }
            break;
        }
        default: {
            PyErr_SetString(JSONEncodeError, "Unsupported type");
            return JumpFlag_Fail;
        }
    }

    return JumpFlag_Default;
#undef RETURN_JUMP_FAIL_ON_UNLIKELY_ERR
#undef CTN_SIZE_GROW
}

#define PYYJSON_DUMPS_OBJ PYYJSON_CONCAT3(pyyjson_dumps_obj, COMPILE_INDENT_LEVEL, COMPILE_UCS_LEVEL)

static force_noinline PyObject *
PYYJSON_DUMPS_OBJ(
#if COMPILE_UCS_LEVEL > 0
        EncodeStackVars _stack_vars,
        EncodeUnicodeBufferInfo _unicode_buffer_info,
        EncodeCallFlag encode_call_flag
#else
        PyObject *in_obj
#endif
) {
#define GOTO_FAIL_ON_UNLIKELY_ERR(_condition) \
    do {                                      \
        if (unlikely(_condition)) goto fail;  \
    } while (0)

#if COMPILE_UCS_LEVEL == 0
    EncodeUnicodeBufferInfo _unicode_buffer_info;
    EncodeStackVars _stack_vars;
    GOTO_FAIL_ON_UNLIKELY_ERR(!init_unicode_buffer(&_unicode_buffer_info) || !init_stack_vars(&_stack_vars, in_obj));

    // this is the starting, we don't need an indent before container.
    // so is_in_obj always pass true
    if (PyDict_CheckExact(_stack_vars.cur_obj)) {
        if (unlikely(PyDict_GET_SIZE(_stack_vars.cur_obj) == 0)) {
            bool _c = UNICODE_BUFFER_APPEND_EMPTY_OBJ(&_unicode_buffer_info, _stack_vars.cur_nested_depth, true);
            assert(_c);
            goto success;
        }
        {
            bool _c = UNICODE_BUFFER_APPEND_OBJ_BEGIN(&_unicode_buffer_info, _stack_vars.cur_nested_depth, true);
            assert(_c);
        }
        assert(!_stack_vars.cur_nested_depth);
        _stack_vars.cur_nested_depth = 1;
        // NOTE: ctn_stack[0] is always invalid
        goto dict_pair_begin;
    } else if (PyList_CheckExact(_stack_vars.cur_obj)) {
        _stack_vars.cur_list_size = PyList_GET_SIZE(_stack_vars.cur_obj);
        if (unlikely(_stack_vars.cur_list_size == 0)) {
            bool _c = UNICODE_BUFFER_APPEND_EMPTY_ARR(&_unicode_buffer_info, _stack_vars.cur_nested_depth, true);
            assert(_c);
            goto success;
        }
        {
            bool _c = UNICODE_BUFFER_APPEND_ARR_BEGIN(&_unicode_buffer_info, _stack_vars.cur_nested_depth, true);
            assert(_c);
        }
        assert(!_stack_vars.cur_nested_depth);
        _stack_vars.cur_nested_depth = 1;
        // NOTE: ctn_stack[0] is always invalid
        _stack_vars.cur_is_tuple = false;
        goto arr_val_begin;
    } else {
        if (unlikely(!PyTuple_CheckExact(_stack_vars.cur_obj))) {
            goto fail_ctntype;
        }
        _stack_vars.cur_list_size = PyTuple_GET_SIZE(_stack_vars.cur_obj);
        if (unlikely(_stack_vars.cur_list_size == 0)) {
            bool _c = UNICODE_BUFFER_APPEND_EMPTY_ARR(&_unicode_buffer_info, _stack_vars.cur_nested_depth, true);
            assert(_c);
            goto success;
        }
        {
            bool _c = UNICODE_BUFFER_APPEND_ARR_BEGIN(&_unicode_buffer_info, _stack_vars.cur_nested_depth, true);
            assert(_c);
        }
        assert(!_stack_vars.cur_nested_depth);
        _stack_vars.cur_nested_depth = 1;
        _stack_vars.cur_is_tuple = true;
        goto arr_val_begin;
    }

    Py_UNREACHABLE();
#else
    switch (encode_call_flag) {
        case CallFlag_ArrVal: {
            if (PyList_CheckExact(_stack_vars.cur_obj)) {
                _stack_vars.cur_is_tuple = false;
                goto arr_val_begin;
            } else {
                _stack_vars.cur_is_tuple = true;
                goto arr_val_begin;
            }
            break;
        }
        case CallFlag_ObjVal: {
            goto dict_pair_begin;
            break;
        }
        case CallFlag_Key: {
            goto dict_key_done;
            break;
        }
        default: {
            Py_UNREACHABLE();
            break;
        }
    }
    Py_UNREACHABLE();
#endif

dict_pair_begin:;
    assert(PyDict_GET_SIZE(_stack_vars.cur_obj) != 0);
    if (pydict_next(_stack_vars.cur_obj, &_stack_vars.cur_pos, &_stack_vars.key, &_stack_vars.val)) {
        if (unlikely(!PyUnicode_CheckExact(_stack_vars.key))) {
            goto fail_keytype;
        }
        GOTO_FAIL_ON_UNLIKELY_ERR(!UNICODE_BUFFER_APPEND_KEY(_stack_vars.key, &_unicode_buffer_info, &_stack_vars.unicode_info, _stack_vars.cur_nested_depth));
        {
#if COMPILE_UCS_LEVEL < 1
            if (unlikely(_stack_vars.unicode_info.cur_ucs_type == 1)) {
                return PYYJSON_CONCAT3(pyyjson_dumps_obj, COMPILE_INDENT_LEVEL, 1)(_stack_vars, _unicode_buffer_info, CallFlag_Key);
            }
#endif
#if COMPILE_UCS_LEVEL < 2
            if (unlikely(_stack_vars.unicode_info.cur_ucs_type == 2)) {
                return PYYJSON_CONCAT3(pyyjson_dumps_obj, COMPILE_INDENT_LEVEL, 2)(_stack_vars, _unicode_buffer_info, CallFlag_Key);
            }
#endif
#if COMPILE_UCS_LEVEL < 4
            if (unlikely(_stack_vars.unicode_info.cur_ucs_type == 4)) {
                return PYYJSON_CONCAT3(pyyjson_dumps_obj, COMPILE_INDENT_LEVEL, 4)(_stack_vars, _unicode_buffer_info, CallFlag_Key);
            }
#endif
        }
    dict_key_done:;
        //
        EncodeValJumpFlag jump_flag = ENCODE_PROCESS_VAL(&_unicode_buffer_info, _stack_vars.val, &_stack_vars, true);
        switch ((jump_flag)) {
            case JumpFlag_Default: {
                break;
            }
            case JumpFlag_ArrValBegin: {
                _stack_vars.cur_is_tuple = false;
                goto arr_val_begin;
            }
            case JumpFlag_DictPairBegin: {
                goto dict_pair_begin;
            }
            case JumpFlag_TupleValBegin: {
                _stack_vars.cur_is_tuple = true;
                goto arr_val_begin;
            }
            case JumpFlag_Fail: {
                goto fail;
            }
#if COMPILE_UCS_LEVEL < 1
            case JumpFlag_Elevate1_ObjVal: {
                return PYYJSON_CONCAT3(pyyjson_dumps_obj, COMPILE_INDENT_LEVEL, 1)(_stack_vars, _unicode_buffer_info, CallFlag_ObjVal);
            }
#endif
#if COMPILE_UCS_LEVEL < 2
            case JumpFlag_Elevate2_ObjVal: {
                return PYYJSON_CONCAT3(pyyjson_dumps_obj, COMPILE_INDENT_LEVEL, 2)(_stack_vars, _unicode_buffer_info, CallFlag_ObjVal);
            }
#endif
#if COMPILE_UCS_LEVEL < 4
            case JumpFlag_Elevate4_ObjVal: {
                return PYYJSON_CONCAT3(pyyjson_dumps_obj, COMPILE_INDENT_LEVEL, 4)(_stack_vars, _unicode_buffer_info, CallFlag_ObjVal);
            }
#endif
            default: {
                Py_UNREACHABLE();
            }
        }
        goto dict_pair_begin;
    } else {
        // dict end
        assert(_stack_vars.cur_nested_depth);
        EncodeCtnWithIndex *last_pos = _stack_vars.ctn_stack + (--_stack_vars.cur_nested_depth);

        GOTO_FAIL_ON_UNLIKELY_ERR(!UNICODE_BUFFER_APPEND_OBJ_END(&_unicode_buffer_info, _stack_vars.cur_nested_depth));
        if (unlikely(_stack_vars.cur_nested_depth == 0)) {
            goto success;
        }

        // update cur_obj and cur_pos
        _stack_vars.cur_obj = last_pos->ctn;
        _stack_vars.cur_pos = last_pos->index;

        if (PyDict_CheckExact(_stack_vars.cur_obj)) {
            goto dict_pair_begin;
        } else if (PyList_CheckExact(_stack_vars.cur_obj)) {
            _stack_vars.cur_list_size = PyList_GET_SIZE(_stack_vars.cur_obj);
            _stack_vars.cur_is_tuple = false;
            goto arr_val_begin;
        } else {
            assert(PyTuple_CheckExact(_stack_vars.cur_obj));
            _stack_vars.cur_list_size = PyTuple_GET_SIZE(_stack_vars.cur_obj);
            _stack_vars.cur_is_tuple = true;
            goto arr_val_begin;
        }
    }

    Py_UNREACHABLE();

arr_val_begin:;
    assert(_stack_vars.cur_list_size != 0);

    if (_stack_vars.cur_pos < _stack_vars.cur_list_size) {
        if (likely(!_stack_vars.cur_is_tuple)) {
            _stack_vars.val = PyList_GET_ITEM(_stack_vars.cur_obj, _stack_vars.cur_pos);
        } else {
            _stack_vars.val = PyTuple_GET_ITEM(_stack_vars.cur_obj, _stack_vars.cur_pos);
        }
        _stack_vars.cur_pos++;
        //
        EncodeValJumpFlag jump_flag = ENCODE_PROCESS_VAL(&_unicode_buffer_info, _stack_vars.val, &_stack_vars, false);
        switch ((jump_flag)) {
            case JumpFlag_Default: {
                break;
            }
            case JumpFlag_ArrValBegin: {
                _stack_vars.cur_is_tuple = false;
                goto arr_val_begin;
            }
            case JumpFlag_DictPairBegin: {
                goto dict_pair_begin;
            }
            case JumpFlag_TupleValBegin: {
                _stack_vars.cur_is_tuple = true;
                goto arr_val_begin;
            }
            case JumpFlag_Fail: {
                goto fail;
            }
#if COMPILE_UCS_LEVEL < 1
            case JumpFlag_Elevate1_ArrVal: {
                return PYYJSON_CONCAT3(pyyjson_dumps_obj, COMPILE_INDENT_LEVEL, 1)(_stack_vars, _unicode_buffer_info, CallFlag_ArrVal);
            }
#endif
#if COMPILE_UCS_LEVEL < 2
            case JumpFlag_Elevate2_ArrVal: {
                return PYYJSON_CONCAT3(pyyjson_dumps_obj, COMPILE_INDENT_LEVEL, 2)(_stack_vars, _unicode_buffer_info, CallFlag_ArrVal);
            }
#endif
#if COMPILE_UCS_LEVEL < 4
            case JumpFlag_Elevate4_ArrVal: {
                return PYYJSON_CONCAT3(pyyjson_dumps_obj, COMPILE_INDENT_LEVEL, 4)(_stack_vars, _unicode_buffer_info, CallFlag_ArrVal);
            }
#endif
            default: {
                Py_UNREACHABLE();
            }
        }
        //
        goto arr_val_begin;
    } else {
        // list end
        assert(_stack_vars.cur_nested_depth);
        EncodeCtnWithIndex *last_pos = _stack_vars.ctn_stack + (--_stack_vars.cur_nested_depth);

        GOTO_FAIL_ON_UNLIKELY_ERR(!UNICODE_BUFFER_APPEND_ARR_END(&_unicode_buffer_info, _stack_vars.cur_nested_depth));
        if (unlikely(_stack_vars.cur_nested_depth == 0)) {
            goto success;
        }

        // update cur_obj and cur_pos
        _stack_vars.cur_obj = last_pos->ctn;
        _stack_vars.cur_pos = last_pos->index;

        if (PyDict_CheckExact(_stack_vars.cur_obj)) {
            goto dict_pair_begin;
        } else if (PyList_CheckExact(_stack_vars.cur_obj)) {
            _stack_vars.cur_list_size = PyList_GET_SIZE(_stack_vars.cur_obj);
            _stack_vars.cur_is_tuple = false;
            goto arr_val_begin;
        } else {
            assert(PyTuple_CheckExact(_stack_vars.cur_obj));
            _stack_vars.cur_list_size = PyTuple_GET_SIZE(_stack_vars.cur_obj);
            _stack_vars.cur_is_tuple = true;
            goto arr_val_begin;
        }
    }
    Py_UNREACHABLE();

success:;
    assert(_stack_vars.cur_nested_depth == 0);
    // remove trailing comma
    (_WRITER(&_unicode_buffer_info))--;

#if COMPILE_UCS_LEVEL == 4
    ucs2_elevate4(&_unicode_buffer_info, &_stack_vars.unicode_info);
    ucs1_elevate4(&_unicode_buffer_info, &_stack_vars.unicode_info);
    ascii_elevate4(&_unicode_buffer_info, &_stack_vars.unicode_info);
#endif
#if COMPILE_UCS_LEVEL == 2
    ucs1_elevate2(&_unicode_buffer_info, &_stack_vars.unicode_info);
    ascii_elevate2(&_unicode_buffer_info, &_stack_vars.unicode_info);
#endif
#if COMPILE_UCS_LEVEL == 1
    ascii_elevate1(&_unicode_buffer_info, &_stack_vars.unicode_info);
#endif
    assert(_stack_vars.unicode_info.cur_ucs_type == COMPILE_UCS_LEVEL);
    Py_ssize_t final_len = GET_UNICODE_BUFFER_FINAL_LEN(&_unicode_buffer_info);
    GOTO_FAIL_ON_UNLIKELY_ERR(!resize_to_fit_pyunicode(&_unicode_buffer_info, final_len, COMPILE_UCS_LEVEL));
    init_pyunicode(_unicode_buffer_info.head, final_len, COMPILE_UCS_LEVEL);
    return (PyObject *)_unicode_buffer_info.head;
fail:;
    if (_unicode_buffer_info.head) {
        PyObject_Free(_unicode_buffer_info.head);
    }
    return NULL;
fail_ctntype:;
    PyErr_SetString(JSONEncodeError, "Unsupported type");
    goto fail;
fail_keytype:;
    PyErr_SetString(JSONEncodeError, "Expected `str` as key");
    goto fail;
}

#include "commondef/iw_out.inl.h"

#undef PYYJSON_DUMPS_OBJ
#undef ENCODE_PROCESS_VAL
#undef GET_UNICODE_BUFFER_FINAL_LEN
#undef UNICODE_BUFFER_APPEND_ARR_END
#undef WRITE_UNICODE_ARR_END
#undef UNICODE_BUFFER_APPEND_OBJ_END
#undef WRITE_UNICODE_OBJ_END
#undef UNICODE_BUFFER_APPEND_OBJ_BEGIN
#undef WRITE_UNICODE_OBJ_BEGIN
#undef UNICODE_BUFFER_APPEND_EMPTY_OBJ
#undef WRITE_UNICODE_EMPTY_OBJ
#undef UNICODE_BUFFER_APPEND_ARR_BEGIN
#undef WRITE_UNICODE_ARR_BEGIN
#undef UNICODE_BUFFER_APPEND_EMPTY_ARR
#undef WRITE_UNICODE_EMPTY_ARR
#undef UNICODE_BUFFER_APPEND_FLOAT
#undef UNICODE_BUFFER_APPEND_NULL
#undef WRITE_UNICODE_NULL
#undef UNICODE_BUFFER_APPEND_TRUE
#undef WRITE_UNICODE_TRUE
#undef UNICODE_BUFFER_APPEND_FALSE
#undef WRITE_UNICODE_FALSE
#undef UNICODE_BUFFER_APPEND_LONG
#undef UNICODE_BUFFER_APPEND_STR
#undef UNICODE_BUFFER_APPEND_KEY
#undef _PREPARE_UNICODE_WRITE
#undef VEC_BACK1
#undef WRITE_INDENT_RETURN_IF_FAIL
#undef COMPILE_WRITE_UCS_LEVEL
#undef COMPILE_READ_UCS_LEVEL
