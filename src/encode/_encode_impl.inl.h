#ifdef PYYJSON_CLANGD_DUMMY
#    include "encode_shared.h"
#    include "encode_unicode_impl_wrap.h"
#    include "pyyjson.h"
#    include "states.h"
#    include "unicode/unicode_buffer.h"
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
//
#include "compile_context/sirw_in.inl.h"


#define WRITE_INDENT_RETURN_IF_FAIL(_writer_addr_, _unicode_buffer_info_, _cur_nested_depth_, _is_in_obj_, _additional_reserve_count_)                         \
    do {                                                                                                                                                       \
        if (unlikely(!unicode_indent_writer(_writer_addr_, _unicode_buffer_info_, _cur_nested_depth_, _is_in_obj_, _additional_reserve_count_))) return false; \
    } while (0)

force_inline void prepare_unicode_write(PyObject *obj, EncodeUnicodeWriter *writer_addr, EncodeUnicodeBufferInfo *unicode_buffer_info, UnicodeInfo *restrict unicode_info, Py_ssize_t *out_len, int *read_kind, int *write_kind) {
    Py_ssize_t out_len_val = PyUnicode_GET_LENGTH(obj);
    *out_len = out_len_val;
    int read_kind_val = PyUnicode_KIND(obj);
    *read_kind = read_kind_val;

#if COMPILE_UCS_LEVEL == 4
    *write_kind = 4;
#elif COMPILE_UCS_LEVEL == 2
    *write_kind = 2;
    if (unlikely(read_kind_val == 4)) {
        memorize_ucs2_to_ucs4(writer_addr, unicode_buffer_info, unicode_info);
        *write_kind = 4;
    }
#elif COMPILE_UCS_LEVEL == 1
    *write_kind = 1;
    if (unlikely(read_kind_val == 2)) {
        memorize_ucs1_to_ucs2(writer_addr, unicode_buffer_info, unicode_info);
        *write_kind = 2;
    } else if (unlikely(read_kind_val == 4)) {
        memorize_ucs1_to_ucs4(writer_addr, unicode_buffer_info, unicode_info);
        *write_kind = 4;
    }
#elif COMPILE_UCS_LEVEL == 0
    *write_kind = 0;
    if (unlikely(read_kind_val == 2)) {
        memorize_ascii_to_ucs2(writer_addr, unicode_buffer_info, unicode_info);
        *write_kind = 2;
    } else if (unlikely(read_kind_val == 4)) {
        memorize_ascii_to_ucs4(writer_addr, unicode_buffer_info, unicode_info);
        *write_kind = 4;
    } else if (unlikely(!PyUnicode_IS_ASCII(obj))) {
        memorize_ascii_to_ucs1(writer_addr, unicode_buffer_info, unicode_info);
        *write_kind = 1;
    }
#endif
}

force_inline bool unicode_buffer_append_key(PyObject *key, EncodeUnicodeWriter *writer_addr, EncodeUnicodeBufferInfo *unicode_buffer_info, UnicodeInfo *unicode_info, Py_ssize_t cur_nested_depth) {
    Py_ssize_t len;
    int kind, write_kind;
    prepare_unicode_write(key, writer_addr, unicode_buffer_info, unicode_info, &len, &kind, &write_kind);

    switch (write_kind) {
#if COMPILE_UCS_LEVEL < 1
        case 0:
#endif
#if COMPILE_UCS_LEVEL < 2
        case 1: {
            RETURN_ON_UNLIKELY_ERR(!KEY_WRITER_IMPL(u8, u8)(key, len, &writer_addr->writer_u8, unicode_buffer_info, cur_nested_depth));
            break;
        }
#endif
#if COMPILE_UCS_LEVEL < 4
        case 2: {
            switch (kind) {
                case 1: {
                    RETURN_ON_UNLIKELY_ERR(!KEY_WRITER_IMPL(u8, u16)(key, len, &writer_addr->writer_u16, unicode_buffer_info, cur_nested_depth));
                    break;
                }
                case 2: {
                    RETURN_ON_UNLIKELY_ERR(!KEY_WRITER_IMPL(u16, u16)(key, len, &writer_addr->writer_u16, unicode_buffer_info, cur_nested_depth));
                    break;
                }
                default: {
                    PYYJSON_UNREACHABLE();
                }
            }
            break;
        }
#endif
        case 4: {
            switch (kind) {
                case 1: {
                    RETURN_ON_UNLIKELY_ERR(!KEY_WRITER_IMPL(u8, u32)(key, len, &writer_addr->writer_u32, unicode_buffer_info, cur_nested_depth));
                    break;
                }
                case 2: {
                    RETURN_ON_UNLIKELY_ERR(!KEY_WRITER_IMPL(u16, u32)(key, len, &writer_addr->writer_u32, unicode_buffer_info, cur_nested_depth));
                    break;
                }
                case 4: {
                    RETURN_ON_UNLIKELY_ERR(!KEY_WRITER_IMPL(u32, u32)(key, len, &writer_addr->writer_u32, unicode_buffer_info, cur_nested_depth));
                    break;
                }
                default: {
                    PYYJSON_UNREACHABLE();
                }
            }
            break;
        }
        default: {
            PYYJSON_UNREACHABLE();
        }
    }
    return true;
}

force_inline bool unicode_buffer_append_str(PyObject *val, EncodeUnicodeWriter *writer_addr, EncodeUnicodeBufferInfo *unicode_buffer_info, UnicodeInfo *unicode_info, Py_ssize_t cur_nested_depth, bool is_in_obj) {
    Py_ssize_t len;
    int kind, write_kind;
    prepare_unicode_write(val, writer_addr, unicode_buffer_info, unicode_info, &len, &kind, &write_kind);
    switch (write_kind) {
#if COMPILE_UCS_LEVEL < 1
        case 0:
#endif
#if COMPILE_UCS_LEVEL < 2
        case 1: {
            RETURN_ON_UNLIKELY_ERR(!STR_WRITER_IMPL(u8, u8)(val, len, &writer_addr->writer_u8, unicode_buffer_info, cur_nested_depth, is_in_obj));
            break;
        }
#endif
#if COMPILE_UCS_LEVEL < 4
        case 2: {
            switch (kind) {
                case 1: {
                    RETURN_ON_UNLIKELY_ERR(!STR_WRITER_IMPL(u8, u16)(val, len, &writer_addr->writer_u16, unicode_buffer_info, cur_nested_depth, is_in_obj));
                    break;
                }
                case 2: {
                    RETURN_ON_UNLIKELY_ERR(!STR_WRITER_IMPL(u16, u16)(val, len, &writer_addr->writer_u16, unicode_buffer_info, cur_nested_depth, is_in_obj));
                    break;
                }
                default: {
                    PYYJSON_UNREACHABLE();
                }
            }
            break;
        }
#endif
        case 4: {
            switch (kind) {
                case 1: {
                    RETURN_ON_UNLIKELY_ERR(!STR_WRITER_IMPL(u8, u32)(val, len, &writer_addr->writer_u32, unicode_buffer_info, cur_nested_depth, is_in_obj));
                    break;
                }
                case 2: {
                    RETURN_ON_UNLIKELY_ERR(!STR_WRITER_IMPL(u16, u32)(val, len, &writer_addr->writer_u32, unicode_buffer_info, cur_nested_depth, is_in_obj));
                    break;
                }
                case 4: {
                    RETURN_ON_UNLIKELY_ERR(!STR_WRITER_IMPL(u32, u32)(val, len, &writer_addr->writer_u32, unicode_buffer_info, cur_nested_depth, is_in_obj));
                    break;
                }
                default: {
                    PYYJSON_UNREACHABLE();
                }
            }
            break;
        }
        default: {
            PYYJSON_UNREACHABLE();
        }
    }
    return true;
}

force_inline bool unicode_buffer_append_long(_dst_t **writer_addr, EncodeUnicodeBufferInfo *unicode_buffer_info, Py_ssize_t cur_nested_depth, PyObject *val, bool is_in_obj) {
    assert(PyLong_CheckExact(val));
    // 32 < TAIL_PADDING == 64 so this is enough
    WRITE_INDENT_RETURN_IF_FAIL(writer_addr, unicode_buffer_info, cur_nested_depth, is_in_obj, TAIL_PADDING);
    _dst_t *writer = *writer_addr;

    if (pylong_is_zero(val)) {
        *writer++ = '0';
        *writer++ = ',';
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
        u64_to_unicode(&writer, v, sign);
        *writer++ = ',';
    }
    assert(check_unicode_writer_valid(writer, unicode_buffer_info));
    *writer_addr = writer;
    return true;
}

force_inline void write_unicode_false(_dst_t **writer_addr) {
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

force_inline bool unicode_buffer_append_false(_dst_t **writer_addr, EncodeUnicodeBufferInfo *unicode_buffer_info, Py_ssize_t cur_nested_depth, bool is_in_obj) {
    WRITE_INDENT_RETURN_IF_FAIL(writer_addr, unicode_buffer_info, cur_nested_depth, is_in_obj, TAIL_PADDING);
    write_unicode_false(writer_addr);
    return true;
}

force_inline void write_unicode_true(_dst_t **writer_addr) {
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

force_inline bool unicode_buffer_append_true(_dst_t **writer_addr, EncodeUnicodeBufferInfo *unicode_buffer_info, Py_ssize_t cur_nested_depth, bool is_in_obj) {
    WRITE_INDENT_RETURN_IF_FAIL(writer_addr, unicode_buffer_info, cur_nested_depth, is_in_obj, TAIL_PADDING);
    write_unicode_true(writer_addr);
    return true;
}

force_inline void write_unicode_null(_dst_t **writer_addr) {
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

force_inline bool unicode_buffer_append_null(_dst_t **writer_addr, EncodeUnicodeBufferInfo *unicode_buffer_info, Py_ssize_t cur_nested_depth, bool is_in_obj) {
    WRITE_INDENT_RETURN_IF_FAIL(writer_addr, unicode_buffer_info, cur_nested_depth, is_in_obj, TAIL_PADDING);
    write_unicode_null(writer_addr);
    return true;
}

force_inline bool unicode_buffer_append_float(_dst_t **writer_addr, EncodeUnicodeBufferInfo *unicode_buffer_info, Py_ssize_t cur_nested_depth, PyObject *val, bool is_in_obj) {
    WRITE_INDENT_RETURN_IF_FAIL(writer_addr, unicode_buffer_info, cur_nested_depth, is_in_obj, TAIL_PADDING);
    double v = PyFloat_AS_DOUBLE(val);
    u64 raw = *PYYJSON_CAST(u64 *, &v); //(u64 *)&v;
    _dst_t *writer = *writer_addr;
    f64_to_unicode(&writer, raw);
    *writer++ = ',';
    *writer_addr = writer;
    return true;
}

force_inline void write_unicode_empty_arr(_dst_t **writer_addr) {
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

force_inline bool unicode_buffer_append_empty_arr(_dst_t **writer_addr, EncodeUnicodeBufferInfo *unicode_buffer_info, Py_ssize_t cur_nested_depth, bool is_in_obj) {
    WRITE_INDENT_RETURN_IF_FAIL(writer_addr, unicode_buffer_info, cur_nested_depth, is_in_obj, TAIL_PADDING);
    write_unicode_empty_arr(writer_addr);
    return true;
}

force_inline void write_unicode_arr_begin(_dst_t **writer_addr) {
    *(*writer_addr)++ = '[';
}

force_inline bool unicode_buffer_append_arr_begin(_dst_t **writer_addr, EncodeUnicodeBufferInfo *unicode_buffer_info, Py_ssize_t cur_nested_depth, bool is_in_obj) {
    WRITE_INDENT_RETURN_IF_FAIL(writer_addr, unicode_buffer_info, cur_nested_depth, is_in_obj, TAIL_PADDING);
    write_unicode_arr_begin(writer_addr);
    return true;
}

force_inline void write_unicode_empty_obj(_dst_t **writer_addr) {
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

force_inline bool unicode_buffer_append_empty_obj(_dst_t **writer_addr, EncodeUnicodeBufferInfo *unicode_buffer_info, Py_ssize_t cur_nested_depth, bool is_in_obj) {
    WRITE_INDENT_RETURN_IF_FAIL(writer_addr, unicode_buffer_info, cur_nested_depth, is_in_obj, TAIL_PADDING);
    write_unicode_empty_obj(writer_addr);
    return true;
}

force_inline void write_unicode_obj_begin(_dst_t **writer_addr) {
    *(*writer_addr)++ = '{';
}

force_inline bool unicode_buffer_append_obj_begin(_dst_t **writer_addr, EncodeUnicodeBufferInfo *unicode_buffer_info, Py_ssize_t cur_nested_depth, bool is_in_obj) {
    WRITE_INDENT_RETURN_IF_FAIL(writer_addr, unicode_buffer_info, cur_nested_depth, is_in_obj, TAIL_PADDING);
    write_unicode_obj_begin(writer_addr);
    return true;
}

force_inline void write_unicode_obj_end(_dst_t **writer_addr) {
    _dst_t *writer = *writer_addr;
    *writer++ = '}';
    *writer++ = ',';
    *writer_addr = writer;
}

force_inline bool unicode_buffer_append_obj_end(_dst_t **writer_addr, EncodeUnicodeBufferInfo *unicode_buffer_info, Py_ssize_t cur_nested_depth) {
    _dst_t *writer = *writer_addr;
    // remove last comma
    writer--;
    // this is not a *value*, the indent is always needed. i.e. `is_in_obj` should always pass false
    WRITE_INDENT_RETURN_IF_FAIL(&writer, unicode_buffer_info, cur_nested_depth, false, TAIL_PADDING);
    write_unicode_obj_end(&writer);
    *writer_addr = writer;
    return true;
}

force_inline void write_unicode_arr_end(_dst_t **writer_addr) {
    _dst_t *writer = *writer_addr;
    *writer++ = ']';
    *writer++ = ',';
    *writer_addr = writer;
}

force_inline bool unicode_buffer_append_arr_end(_dst_t **writer_addr, EncodeUnicodeBufferInfo *unicode_buffer_info, Py_ssize_t cur_nested_depth) {
    _dst_t *writer = *writer_addr;
    // remove last comma
    writer--;
    // this is not a *value*, the indent is always needed. i.e. `is_in_obj` should always pass false
    WRITE_INDENT_RETURN_IF_FAIL(&writer, unicode_buffer_info, cur_nested_depth, false, TAIL_PADDING);
    write_unicode_arr_end(&writer);
    *writer_addr = writer;
    return true;
}

// #define GET_UNICODE_BUFFER_FINAL_LEN PYYJSON_CONCAT2(get_unicode_buffer_final_len, COMPILE_UCS_LEVEL)
// #if COMPILE_INDENT_LEVEL == 0
// // avoid compile again
// force_inline Py_ssize_t GET_UNICODE_BUFFER_FINAL_LEN(EncodeUnicodeBufferInfo *unicode_buffer_info) {
// #    if COMPILE_UCS_LEVEL == 0
//     return unicode_buffer_info->writer.writer_u8 - (u8 *)GET_VEC_ASCII_START(unicode_buffer_info);
// #    elif COMPILE_UCS_LEVEL == 1
//     return unicode_buffer_info->writer.writer_u8 - (u8 *)GET_VEC_COMPACT_START(unicode_buffer_info);
// #    elif COMPILE_UCS_LEVEL == 2
//     return unicode_buffer_info->writer.writer_u16 - (u16 *)GET_VEC_COMPACT_START(unicode_buffer_info);
// #    elif COMPILE_UCS_LEVEL == 4
//     return unicode_buffer_info->writer.writer_u32 - (u32 *)GET_VEC_COMPACT_START(unicode_buffer_info);
// #    endif
// }
// #endif

#define ENCODE_PROCESS_VAL PYYJSON_CONCAT3(encode_process_val, COMPILE_INDENT_LEVEL, COMPILE_UCS_LEVEL)

force_inline EncodeValJumpFlag ENCODE_PROCESS_VAL(
        EncodeUnicodeWriter *writer_addr,
        EncodeUnicodeBufferInfo *unicode_buffer_info, PyObject *val,
        PyObject **cur_obj_addr,
        Py_ssize_t *cur_pos_addr,
        Py_ssize_t *cur_nested_depth_addr,
        Py_ssize_t *cur_list_size_addr,
        EncodeCtnWithIndex *ctn_stack,
        UnicodeInfo *unicode_info_addr,
        bool is_in_obj) {
#define CTN_SIZE_GROW()                                                         \
    do {                                                                        \
        if (unlikely(*cur_nested_depth_addr == PYYJSON_ENCODE_MAX_RECURSION)) { \
            PyErr_SetString(JSONEncodeError, "Too many nested structures");     \
            return JumpFlag_Fail;                                               \
        }                                                                       \
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
            RETURN_JUMP_FAIL_ON_UNLIKELY_ERR(!unicode_buffer_append_str(val, writer_addr, unicode_buffer_info, unicode_info_addr, *cur_nested_depth_addr, is_in_obj));
#if COMPILE_UCS_LEVEL < 1
            if (unlikely(unicode_info_addr->cur_ucs_type == 1)) {
                return is_in_obj ? JumpFlag_Elevate1_ObjVal : JumpFlag_Elevate1_ArrVal;
            }
#endif
#if COMPILE_UCS_LEVEL < 2
            if (unlikely(unicode_info_addr->cur_ucs_type == 2)) {
                return is_in_obj ? JumpFlag_Elevate2_ObjVal : JumpFlag_Elevate2_ArrVal;
            }
#endif
#if COMPILE_UCS_LEVEL < 4
            if (unlikely(unicode_info_addr->cur_ucs_type == 4)) {
                return is_in_obj ? JumpFlag_Elevate4_ObjVal : JumpFlag_Elevate4_ArrVal;
            }
#endif
            break;
        }
        case T_Long: {
            RETURN_JUMP_FAIL_ON_UNLIKELY_ERR(!unicode_buffer_append_long(&_WRITER(writer_addr), unicode_buffer_info, *cur_nested_depth_addr, val, is_in_obj));
            break;
        }
        case T_Bool: {
            if (val == Py_False) {
                RETURN_JUMP_FAIL_ON_UNLIKELY_ERR(!unicode_buffer_append_false(&_WRITER(writer_addr), unicode_buffer_info, *cur_nested_depth_addr, is_in_obj));
            } else {
                assert(val == Py_True);
                RETURN_JUMP_FAIL_ON_UNLIKELY_ERR(!unicode_buffer_append_true(&_WRITER(writer_addr), unicode_buffer_info, *cur_nested_depth_addr, is_in_obj));
            }
            break;
        }
        case T_None: {
            RETURN_JUMP_FAIL_ON_UNLIKELY_ERR(!unicode_buffer_append_null(&_WRITER(writer_addr), unicode_buffer_info, *cur_nested_depth_addr, is_in_obj));
            break;
        }
        case T_Float: {
            RETURN_JUMP_FAIL_ON_UNLIKELY_ERR(!unicode_buffer_append_float(&_WRITER(writer_addr), unicode_buffer_info, *cur_nested_depth_addr, val, is_in_obj));
            break;
        }
        case T_List: {
            Py_ssize_t this_list_size = PyList_GET_SIZE(val);
            if (unlikely(this_list_size == 0)) {
                RETURN_JUMP_FAIL_ON_UNLIKELY_ERR(!unicode_buffer_append_empty_arr(&_WRITER(writer_addr), unicode_buffer_info, *cur_nested_depth_addr, is_in_obj));
            } else {
                RETURN_JUMP_FAIL_ON_UNLIKELY_ERR(!unicode_buffer_append_arr_begin(&_WRITER(writer_addr), unicode_buffer_info, *cur_nested_depth_addr, is_in_obj));
                CTN_SIZE_GROW();
                EncodeCtnWithIndex *cur_write_ctn = ctn_stack + ((*cur_nested_depth_addr)++);
                cur_write_ctn->ctn = *cur_obj_addr;
                cur_write_ctn->index = *cur_pos_addr;
                *cur_obj_addr = val;
                *cur_pos_addr = 0;
                *cur_list_size_addr = this_list_size;
                return JumpFlag_ArrValBegin;
            }
            break;
        }
        case T_Dict: {
            if (unlikely(PyDict_GET_SIZE(val) == 0)) {
                RETURN_JUMP_FAIL_ON_UNLIKELY_ERR(!unicode_buffer_append_empty_obj(&_WRITER(writer_addr), unicode_buffer_info, *cur_nested_depth_addr, is_in_obj));
            } else {
                RETURN_JUMP_FAIL_ON_UNLIKELY_ERR(!unicode_buffer_append_obj_begin(&_WRITER(writer_addr), unicode_buffer_info, *cur_nested_depth_addr, is_in_obj));
                CTN_SIZE_GROW();
                EncodeCtnWithIndex *cur_write_ctn = ctn_stack + ((*cur_nested_depth_addr)++);
                cur_write_ctn->ctn = *cur_obj_addr;
                cur_write_ctn->index = *cur_pos_addr;
                *cur_obj_addr = val;
                *cur_pos_addr = 0;
                return JumpFlag_DictPairBegin;
            }
            break;
        }
        case T_Tuple: {
            Py_ssize_t this_list_size = PyTuple_Size(val);
            if (unlikely(this_list_size == 0)) {
                RETURN_JUMP_FAIL_ON_UNLIKELY_ERR(!unicode_buffer_append_empty_arr(&_WRITER(writer_addr), unicode_buffer_info, *cur_nested_depth_addr, is_in_obj));
            } else {
                RETURN_JUMP_FAIL_ON_UNLIKELY_ERR(!unicode_buffer_append_arr_begin(&_WRITER(writer_addr), unicode_buffer_info, *cur_nested_depth_addr, is_in_obj));
                CTN_SIZE_GROW();
                EncodeCtnWithIndex *cur_write_ctn = ctn_stack + ((*cur_nested_depth_addr)++);
                cur_write_ctn->ctn = *cur_obj_addr;
                cur_write_ctn->index = *cur_pos_addr;
                *cur_obj_addr = val;
                *cur_pos_addr = 0;
                *cur_list_size_addr = this_list_size;
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

#define _DUMPS_PASS_PARAMS writer, key, val, cur_obj, cur_pos, cur_nested_depth, cur_list_size, ctn_stack, unicode_info, cur_is_tuple

static force_noinline PyObject *
PYYJSON_DUMPS_OBJ(
#if COMPILE_UCS_LEVEL > 0
        EncodeUnicodeWriter writer,
        PyObject *key, PyObject *val, PyObject *cur_obj,
        Py_ssize_t cur_pos, Py_ssize_t cur_nested_depth, Py_ssize_t cur_list_size,
        EncodeCtnWithIndex *ctn_stack, UnicodeInfo unicode_info, bool cur_is_tuple,
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
    EncodeUnicodeWriter writer;
    EncodeUnicodeBufferInfo _unicode_buffer_info;
    PyObject *key, *val;
    PyObject *cur_obj = in_obj;
    Py_ssize_t cur_pos = 0;
    Py_ssize_t cur_nested_depth = 0;
    Py_ssize_t cur_list_size;
    // alias thread local buffer
    EncodeCtnWithIndex *ctn_stack;
    UnicodeInfo unicode_info;
    bool cur_is_tuple;
    memset(&unicode_info, 0, sizeof(unicode_info));
    //
    GOTO_FAIL_ON_UNLIKELY_ERR(!init_unicode_buffer(&writer, &_unicode_buffer_info) || !init_encode_ctn_stack(&ctn_stack));

    // this is the starting, we don't need an indent before container.
    // so is_in_obj always pass true
    if (PyDict_CheckExact(cur_obj)) {
        if (unlikely(PyDict_GET_SIZE(cur_obj) == 0)) {
            bool _c = unicode_buffer_append_empty_obj(&_WRITER(&writer), &_unicode_buffer_info, cur_nested_depth, true);
            assert(_c);
            goto success;
        }
        {
            bool _c = unicode_buffer_append_obj_begin(&_WRITER(&writer), &_unicode_buffer_info, cur_nested_depth, true);
            assert(_c);
        }
        assert(!cur_nested_depth);
        cur_nested_depth = 1;
        // NOTE: ctn_stack[0] is always invalid
        goto dict_pair_begin;
    } else if (PyList_CheckExact(cur_obj)) {
        cur_list_size = PyList_GET_SIZE(cur_obj);
        if (unlikely(cur_list_size == 0)) {
            bool _c = unicode_buffer_append_empty_arr(&_WRITER(&writer), &_unicode_buffer_info, cur_nested_depth, true);
            assert(_c);
            goto success;
        }
        {
            bool _c = unicode_buffer_append_arr_begin(&_WRITER(&writer), &_unicode_buffer_info, cur_nested_depth, true);
            assert(_c);
        }
        assert(!cur_nested_depth);
        cur_nested_depth = 1;
        // NOTE: ctn_stack[0] is always invalid
        cur_is_tuple = false;
        goto arr_val_begin;
    } else {
        if (unlikely(!PyTuple_CheckExact(cur_obj))) {
            goto fail_ctntype;
        }
        cur_list_size = PyTuple_GET_SIZE(cur_obj);
        if (unlikely(cur_list_size == 0)) {
            bool _c = unicode_buffer_append_empty_arr(&_WRITER(&writer), &_unicode_buffer_info, cur_nested_depth, true);
            assert(_c);
            goto success;
        }
        {
            bool _c = unicode_buffer_append_arr_begin(&_WRITER(&writer), &_unicode_buffer_info, cur_nested_depth, true);
            assert(_c);
        }
        assert(!cur_nested_depth);
        cur_nested_depth = 1;
        cur_is_tuple = true;
        goto arr_val_begin;
    }

    PYYJSON_UNREACHABLE();
#else
    switch (encode_call_flag) {
        case CallFlag_ArrVal: {
            if (PyList_CheckExact(cur_obj)) {
                cur_is_tuple = false;
                goto arr_val_begin;
            } else {
                cur_is_tuple = true;
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
            PYYJSON_UNREACHABLE();
            break;
        }
    }
    PYYJSON_UNREACHABLE();
#endif

dict_pair_begin:;
    assert(PyDict_GET_SIZE(cur_obj) != 0);
    if (pydict_next(cur_obj, &cur_pos, &key, &val)) {
        if (unlikely(!PyUnicode_CheckExact(key))) {
            goto fail_keytype;
        }
        GOTO_FAIL_ON_UNLIKELY_ERR(!unicode_buffer_append_key(key, &writer, &_unicode_buffer_info, &unicode_info, cur_nested_depth));
        {
#if COMPILE_UCS_LEVEL < 1
            if (unlikely(unicode_info.cur_ucs_type == 1)) {
                return PYYJSON_CONCAT3(pyyjson_dumps_obj, COMPILE_INDENT_LEVEL, 1)(_DUMPS_PASS_PARAMS, _unicode_buffer_info, CallFlag_Key);
            }
#endif
#if COMPILE_UCS_LEVEL < 2
            if (unlikely(unicode_info.cur_ucs_type == 2)) {
                return PYYJSON_CONCAT3(pyyjson_dumps_obj, COMPILE_INDENT_LEVEL, 2)(_DUMPS_PASS_PARAMS, _unicode_buffer_info, CallFlag_Key);
            }
#endif
#if COMPILE_UCS_LEVEL < 4
            if (unlikely(unicode_info.cur_ucs_type == 4)) {
                return PYYJSON_CONCAT3(pyyjson_dumps_obj, COMPILE_INDENT_LEVEL, 4)(_DUMPS_PASS_PARAMS, _unicode_buffer_info, CallFlag_Key);
            }
#endif
        }
    dict_key_done:;
        //
        EncodeValJumpFlag jump_flag = ENCODE_PROCESS_VAL(&writer, &_unicode_buffer_info, val, &cur_obj, &cur_pos, &cur_nested_depth, &cur_list_size, ctn_stack, &unicode_info, true);
        switch ((jump_flag)) {
            case JumpFlag_Default: {
                break;
            }
            case JumpFlag_ArrValBegin: {
                cur_is_tuple = false;
                goto arr_val_begin;
            }
            case JumpFlag_DictPairBegin: {
                goto dict_pair_begin;
            }
            case JumpFlag_TupleValBegin: {
                cur_is_tuple = true;
                goto arr_val_begin;
            }
            case JumpFlag_Fail: {
                goto fail;
            }
#if COMPILE_UCS_LEVEL < 1
            case JumpFlag_Elevate1_ObjVal: {
                return PYYJSON_CONCAT3(pyyjson_dumps_obj, COMPILE_INDENT_LEVEL, 1)(_DUMPS_PASS_PARAMS, _unicode_buffer_info, CallFlag_ObjVal);
            }
#endif
#if COMPILE_UCS_LEVEL < 2
            case JumpFlag_Elevate2_ObjVal: {
                return PYYJSON_CONCAT3(pyyjson_dumps_obj, COMPILE_INDENT_LEVEL, 2)(_DUMPS_PASS_PARAMS, _unicode_buffer_info, CallFlag_ObjVal);
            }
#endif
#if COMPILE_UCS_LEVEL < 4
            case JumpFlag_Elevate4_ObjVal: {
                return PYYJSON_CONCAT3(pyyjson_dumps_obj, COMPILE_INDENT_LEVEL, 4)(_DUMPS_PASS_PARAMS, _unicode_buffer_info, CallFlag_ObjVal);
            }
#endif
            default: {
                PYYJSON_UNREACHABLE();
            }
        }
        goto dict_pair_begin;
    } else {
        // dict end
        assert(cur_nested_depth);
        EncodeCtnWithIndex *last_pos = ctn_stack + (--cur_nested_depth);

        GOTO_FAIL_ON_UNLIKELY_ERR(!unicode_buffer_append_obj_end(&_WRITER(&writer), &_unicode_buffer_info, cur_nested_depth));
        if (unlikely(cur_nested_depth == 0)) {
            goto success;
        }

        // update cur_obj and cur_pos
        cur_obj = last_pos->ctn;
        cur_pos = last_pos->index;

        if (PyDict_CheckExact(cur_obj)) {
            goto dict_pair_begin;
        } else if (PyList_CheckExact(cur_obj)) {
            cur_list_size = PyList_GET_SIZE(cur_obj);
            cur_is_tuple = false;
            goto arr_val_begin;
        } else {
            assert(PyTuple_CheckExact(cur_obj));
            cur_list_size = PyTuple_GET_SIZE(cur_obj);
            cur_is_tuple = true;
            goto arr_val_begin;
        }
    }

    PYYJSON_UNREACHABLE();

arr_val_begin:;
    assert(cur_list_size != 0);

    if (cur_pos < cur_list_size) {
        if (likely(!cur_is_tuple)) {
            val = PyList_GET_ITEM(cur_obj, cur_pos);
        } else {
            val = PyTuple_GET_ITEM(cur_obj, cur_pos);
        }
        cur_pos++;
        //
        EncodeValJumpFlag jump_flag = ENCODE_PROCESS_VAL(&writer, &_unicode_buffer_info, val, &cur_obj, &cur_pos, &cur_nested_depth, &cur_list_size, ctn_stack, &unicode_info, false);
        switch ((jump_flag)) {
            case JumpFlag_Default: {
                break;
            }
            case JumpFlag_ArrValBegin: {
                cur_is_tuple = false;
                goto arr_val_begin;
            }
            case JumpFlag_DictPairBegin: {
                goto dict_pair_begin;
            }
            case JumpFlag_TupleValBegin: {
                cur_is_tuple = true;
                goto arr_val_begin;
            }
            case JumpFlag_Fail: {
                goto fail;
            }
#if COMPILE_UCS_LEVEL < 1
            case JumpFlag_Elevate1_ArrVal: {
                return PYYJSON_CONCAT3(pyyjson_dumps_obj, COMPILE_INDENT_LEVEL, 1)(_DUMPS_PASS_PARAMS, _unicode_buffer_info, CallFlag_ArrVal);
            }
#endif
#if COMPILE_UCS_LEVEL < 2
            case JumpFlag_Elevate2_ArrVal: {
                return PYYJSON_CONCAT3(pyyjson_dumps_obj, COMPILE_INDENT_LEVEL, 2)(_DUMPS_PASS_PARAMS, _unicode_buffer_info, CallFlag_ArrVal);
            }
#endif
#if COMPILE_UCS_LEVEL < 4
            case JumpFlag_Elevate4_ArrVal: {
                return PYYJSON_CONCAT3(pyyjson_dumps_obj, COMPILE_INDENT_LEVEL, 4)(_DUMPS_PASS_PARAMS, _unicode_buffer_info, CallFlag_ArrVal);
            }
#endif
            default: {
                PYYJSON_UNREACHABLE();
            }
        }
        //
        goto arr_val_begin;
    } else {
        // list end
        assert(cur_nested_depth);
        EncodeCtnWithIndex *last_pos = ctn_stack + (--cur_nested_depth);

        GOTO_FAIL_ON_UNLIKELY_ERR(!unicode_buffer_append_arr_end(&_WRITER(&writer), &_unicode_buffer_info, cur_nested_depth));
        if (unlikely(cur_nested_depth == 0)) {
            goto success;
        }

        // update cur_obj and cur_pos
        cur_obj = last_pos->ctn;
        cur_pos = last_pos->index;

        if (PyDict_CheckExact(cur_obj)) {
            goto dict_pair_begin;
        } else if (PyList_CheckExact(cur_obj)) {
            cur_list_size = PyList_GET_SIZE(cur_obj);
            cur_is_tuple = false;
            goto arr_val_begin;
        } else {
            assert(PyTuple_CheckExact(cur_obj));
            cur_list_size = PyTuple_GET_SIZE(cur_obj);
            cur_is_tuple = true;
            goto arr_val_begin;
        }
    }
    PYYJSON_UNREACHABLE();

success:;
    assert(cur_nested_depth == 0);
    // remove trailing comma
    (_WRITER(&writer))--;

#if COMPILE_UCS_LEVEL == 4
    ucs2_elevate4(&_unicode_buffer_info, &unicode_info);
    ucs1_elevate4(&_unicode_buffer_info, &unicode_info);
    ascii_elevate4(&_unicode_buffer_info, &unicode_info);
#endif
#if COMPILE_UCS_LEVEL == 2
    ucs1_elevate2(&_unicode_buffer_info, &unicode_info);
    ascii_elevate2(&_unicode_buffer_info, &unicode_info);
#endif
#if COMPILE_UCS_LEVEL == 1
    ascii_elevate1(&_unicode_buffer_info, &unicode_info);
#endif
    assert(unicode_info.cur_ucs_type == COMPILE_UCS_LEVEL);
    Py_ssize_t final_len = get_unicode_buffer_final_len(writer, &_unicode_buffer_info);
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

#undef _DUMPS_PASS_PARAMS


#include "compile_context/sirw_out.inl.h"

#undef PYYJSON_DUMPS_OBJ
#undef ENCODE_PROCESS_VAL
#undef VEC_BACK1
#undef WRITE_INDENT_RETURN_IF_FAIL
#undef COMPILE_WRITE_UCS_LEVEL
#undef COMPILE_READ_UCS_LEVEL
