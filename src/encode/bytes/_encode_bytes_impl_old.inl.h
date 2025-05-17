#ifdef PYYJSON_CLANGD_DUMMY
#    include "encode/encode_shared.h"
#    include "pyyjson.h"
#    include "tls.h"
#endif


#define BYTES_INDENT_WRITER PYYJSON_CONCAT2(bytes_indent_writer, COMPILE_INDENT_LEVEL)
#define WRITE_INDENT_RETURN_IF_FAIL(_unicode_buffer_info_, _cur_nested_depth_, _is_in_obj_, _additional_reserve_count_)                       \
    do {                                                                                                                                      \
        if (unlikely(!BYTES_INDENT_WRITER(_unicode_buffer_info_, _cur_nested_depth_, _is_in_obj_, _additional_reserve_count_))) return false; \
    } while (0)

// #define WRITE_BYTES_FALSE PYYJSON_CONCAT3(write_unicode_false, COMPILE_INDENT_LEVEL, 1)
// #define WRITE_BYTES_TRUE PYYJSON_CONCAT3(write_unicode_true, COMPILE_INDENT_LEVEL, 1)
// #define WRITE_BYTES_NULL PYYJSON_CONCAT3(write_unicode_null, COMPILE_INDENT_LEVEL, 1)
// #define WRITE_BYTES_ARR_BEGIN PYYJSON_CONCAT3(write_unicode_arr_begin, COMPILE_INDENT_LEVEL, 1)
// #define WRITE_BYTES_EMPTY_ARR PYYJSON_CONCAT3(write_unicode_empty_arr, COMPILE_INDENT_LEVEL, 1)
// #define WRITE_BYTES_ARR_END PYYJSON_CONCAT3(write_unicode_arr_end, COMPILE_INDENT_LEVEL, 1)
// #define WRITE_BYTES_OBJ_BEGIN PYYJSON_CONCAT3(write_unicode_obj_begin, COMPILE_INDENT_LEVEL, 1)
// #define WRITE_BYTES_EMPTY_OBJ PYYJSON_CONCAT3(write_unicode_empty_obj, COMPILE_INDENT_LEVEL, 1)
// #define WRITE_BYTES_OBJ_END PYYJSON_CONCAT3(write_unicode_obj_end, COMPILE_INDENT_LEVEL, 1)

// #define BYTES_BUFFER_APPEND_KEY PYYJSON_CONCAT2(bytes_buffer_append_key, COMPILE_INDENT_LEVEL)
// #define BYTES_BUFFER_APPEND_STR PYYJSON_CONCAT2(bytes_buffer_append_str, COMPILE_INDENT_LEVEL)
// #define BYTES_BUFFER_APPEND_LONG PYYJSON_CONCAT2(bytes_buffer_append_long, COMPILE_INDENT_LEVEL)
// #define BYTES_BUFFER_APPEND_FLOAT PYYJSON_CONCAT2(bytes_buffer_append_float, COMPILE_INDENT_LEVEL)

// #define BYTES_BUFFER_APPEND_FALSE PYYJSON_CONCAT2(bytes_buffer_append_false, COMPILE_INDENT_LEVEL)
// #define BYTES_BUFFER_APPEND_TRUE PYYJSON_CONCAT2(bytes_buffer_append_true, COMPILE_INDENT_LEVEL)
// #define BYTES_BUFFER_APPEND_NULL PYYJSON_CONCAT2(bytes_buffer_append_null, COMPILE_INDENT_LEVEL)

// #define BYTES_BUFFER_APPEND_ARR_BEGIN PYYJSON_CONCAT2(bytes_buffer_append_arr_begin, COMPILE_INDENT_LEVEL)
// #define BYTES_BUFFER_APPEND_EMPTY_ARR PYYJSON_CONCAT2(bytes_buffer_append_empty_arr, COMPILE_INDENT_LEVEL)
// #define BYTES_BUFFER_APPEND_ARR_END PYYJSON_CONCAT2(bytes_buffer_append_arr_end, COMPILE_INDENT_LEVEL)
// #define BYTES_BUFFER_APPEND_OBJ_BEGIN PYYJSON_CONCAT2(bytes_buffer_append_obj_begin, COMPILE_INDENT_LEVEL)
// #define BYTES_BUFFER_APPEND_EMPTY_OBJ PYYJSON_CONCAT2(bytes_buffer_append_empty_obj, COMPILE_INDENT_LEVEL)
// #define BYTES_BUFFER_APPEND_OBJ_END PYYJSON_CONCAT2(bytes_buffer_append_obj_end, COMPILE_INDENT_LEVEL)

#define ENCODE_PROCESS_BYTES_VAL PYYJSON_CONCAT2(encode_process_bytes_val, COMPILE_INDENT_LEVEL)

force_inline bool BYTES_INDENT_WRITER(EncodeUTF8BufferInfo *utf8_buffer_info, Py_ssize_t cur_nested_depth, bool is_in_obj, usize additional_reserve_count) {
    if (!is_in_obj && COMPILE_INDENT_LEVEL != 0) {
        RETURN_ON_UNLIKELY_ERR(!bytes_buffer_reserve(utf8_buffer_info, get_indent_char_count(cur_nested_depth, COMPILE_INDENT_LEVEL) + additional_reserve_count));
        PYYJSON_CONCAT3(write_unicode_indent, __INDENT_NAME, u8)(&utf8_buffer_info->writer, cur_nested_depth);
    } else {
        RETURN_ON_UNLIKELY_ERR(!bytes_buffer_reserve(utf8_buffer_info, additional_reserve_count));
    }
    return true;
}

// force_inline bool BYTES_BUFFER_APPEND_LONG(EncodeUTF8BufferInfo *utf8_buffer_info, Py_ssize_t cur_nested_depth, PyObject *val, bool is_in_obj) {
//     assert(PyLong_CheckExact(val));
//     // 32 < TAIL_PADDING == 64 so this is enough
//     WRITE_INDENT_RETURN_IF_FAIL(utf8_buffer_info, cur_nested_depth, is_in_obj, TAIL_PADDING);

//     if (pylong_is_zero(val)) {
//         // _dst_t *writer = utf8_buffer_info->writer;
//         *utf8_buffer_info->writer++ = '0';
//         *utf8_buffer_info->writer++ = ',';
//         utf8_buffer_info->writer += 2;
//     } else {
//         u64 v;
//         usize sign;
//         if (pylong_is_unsigned(val)) {
//             if (unlikely(!pylong_value_unsigned(val, &v))) {
//                 PyErr_SetString(JSONEncodeError, "convert value to unsigned long long failed");
//                 return false;
//             }
//             sign = 0;
//         } else {
//             i64 v2;
//             if (unlikely(!pylong_value_signed(val, &v2))) {
//                 PyErr_SetString(JSONEncodeError, "convert value to long long failed");
//                 return false;
//             }
//             assert(v2 <= 0);
//             v = -v2;
//             sign = 1;
//         }
//         write_unicode_u64_1(&utf8_buffer_info->writer, v, sign);
//         *utf8_buffer_info->writer++ = ',';
//     }
//     // assert(check_unicode_writer_valid(unicode_buffer_info));
//     return true;
// }

// force_inline bool BYTES_BUFFER_APPEND_KEY(PyObject *val, EncodeUTF8BufferInfo *restrict utf8_buffer_info, Py_ssize_t cur_nested_depth) {
//     PyASCIIObject *ascii_obj = PYYJSON_CAST(PyASCIIObject *, val);
//     int kind;
//     if (ascii_obj->state.ascii) {
//         kind = 0;
//     } else {
//         kind = ascii_obj->state.kind;
//     }
//     usize len = ascii_obj->length;
//     RETURN_ON_UNLIKELY_ERR(!bytes_buffer_reserve(utf8_buffer_info, get_indent_char_count(cur_nested_depth, COMPILE_INDENT_LEVEL) + 5 + 6 * len + TAIL_PADDING));
//     *utf8_buffer_info->writer++ = '"';
//     switch (kind) {
//         case 0: {
//             bytes_write_ascii(&utf8_buffer_info->writer, PYYJSON_CAST(u8 *, PYYJSON_CAST(PyASCIIObject *, val) + 1), len);
//             break;
//         }
//         case 1: {
//             bytes_write_ucs1(&utf8_buffer_info->writer, PYYJSON_CAST(u8 *, PYYJSON_CAST(PyCompactUnicodeObject *, val) + 1), len);
//             break;
//         }
//         case 2: {
//             bytes_write_ucs2(&utf8_buffer_info->writer, PYYJSON_CAST(u16 *, PYYJSON_CAST(PyCompactUnicodeObject *, val) + 1), len);
//             break;
//         }
//         case 4: {
//             bytes_write_ucs4(&utf8_buffer_info->writer, PYYJSON_CAST(u32 *, PYYJSON_CAST(PyCompactUnicodeObject *, val) + 1), len);
//             break;
//         }
//         default:
//             PYYJSON_UNREACHABLE();
//     }
//     *utf8_buffer_info->writer++ = '"';
//     *utf8_buffer_info->writer++ = ':';
//     if (COMPILE_INDENT_LEVEL > 0) {
//         *utf8_buffer_info->writer++ = ' ';
//     }
// #if SIZEOF_VOID_P == 8
//     *utf8_buffer_info->writer = 0;
// #endif
//     return true;
//     //     RETURN_ON_UNLIKELY_ERR(!unicode_buffer_reserve(unicode_buffer_info, get_indent_char_count(cur_nested_depth, COMPILE_INDENT_LEVEL) + 5 + 6 * len + TAIL_PADDING));
//     //     write_unicode_indent(&_WRITER(unicode_buffer_info), cur_nested_depth);
//     //     *_WRITER(unicode_buffer_info)++ = '"';
//     //     WRITE_UNICODE_IMPL(unicode_buffer_info, (_src_t *)get_unicode_data(key), len);
//     //     _dst_t *writer = _WRITER(unicode_buffer_info);
//     //     *writer++ = '"';
//     //     *writer++ = ':';
//     // #if COMPILE_INDENT_LEVEL > 0
//     //     *writer++ = ' ';
//     // #    if SIZEOF_VOID_P == 8 || COMPILE_WRITE_UCS_LEVEL != 4
//     //     *writer = 0;
//     // #    endif // SIZEOF_VOID_P == 8 || COMPILE_WRITE_UCS_LEVEL != 4
//     // #endif     // COMPILE_INDENT_LEVEL > 0
//     //     _WRITER(unicode_buffer_info) += (COMPILE_INDENT_LEVEL > 0) ? 3 : 2;
//     //     assert(check_unicode_writer_valid(unicode_buffer_info));
//     //     return true;
//     // return false;
// }

// force_inline bool BYTES_BUFFER_APPEND_STR(PyObject *val, EncodeUTF8BufferInfo *utf8_buffer_info, Py_ssize_t cur_nested_depth, bool is_in_obj) {
//     // TODO
//     return false;
// }

// force_inline bool BYTES_BUFFER_APPEND_FALSE(EncodeUTF8BufferInfo *utf8_buffer_info, Py_ssize_t cur_nested_depth, bool is_in_obj) {
//     WRITE_INDENT_RETURN_IF_FAIL(utf8_buffer_info, cur_nested_depth, is_in_obj, TAIL_PADDING);
//     WRITE_BYTES_FALSE(&utf8_buffer_info->writer);
//     return true;
// }

// force_inline bool BYTES_BUFFER_APPEND_TRUE(EncodeUTF8BufferInfo *utf8_buffer_info, Py_ssize_t cur_nested_depth, bool is_in_obj) {
//     WRITE_INDENT_RETURN_IF_FAIL(utf8_buffer_info, cur_nested_depth, is_in_obj, TAIL_PADDING);
//     WRITE_BYTES_TRUE(&utf8_buffer_info->writer);
//     return true;
// }

// force_inline bool BYTES_BUFFER_APPEND_NULL(EncodeUTF8BufferInfo *utf8_buffer_info, Py_ssize_t cur_nested_depth, bool is_in_obj) {
//     WRITE_INDENT_RETURN_IF_FAIL(utf8_buffer_info, cur_nested_depth, is_in_obj, TAIL_PADDING);
//     WRITE_BYTES_NULL(&utf8_buffer_info->writer);
//     return true;
// }

// force_inline bool BYTES_BUFFER_APPEND_FLOAT(EncodeUTF8BufferInfo *utf8_buffer_info, Py_ssize_t cur_nested_depth, PyObject *val, bool is_in_obj) {
//     WRITE_INDENT_RETURN_IF_FAIL(utf8_buffer_info, cur_nested_depth, is_in_obj, TAIL_PADDING);
//     double v = PyFloat_AS_DOUBLE(val);
//     u64 raw = *PYYJSON_CAST(u64 *, &v); //(u64 *)&v;
//     write_unicode_f64_1(&utf8_buffer_info->writer, raw);
//     *utf8_buffer_info->writer++ = ',';
//     return true;
// }

// force_inline bool BYTES_BUFFER_APPEND_ARR_BEGIN(EncodeUTF8BufferInfo *utf8_buffer_info, Py_ssize_t cur_nested_depth, bool is_in_obj) {
//     WRITE_INDENT_RETURN_IF_FAIL(utf8_buffer_info, cur_nested_depth, is_in_obj, TAIL_PADDING);
//     WRITE_BYTES_ARR_BEGIN(&utf8_buffer_info->writer);
//     return true;
// }

// force_inline bool BYTES_BUFFER_APPEND_EMPTY_ARR(EncodeUTF8BufferInfo *utf8_buffer_info, Py_ssize_t cur_nested_depth, bool is_in_obj) {
//     WRITE_INDENT_RETURN_IF_FAIL(utf8_buffer_info, cur_nested_depth, is_in_obj, TAIL_PADDING);
//     WRITE_BYTES_EMPTY_ARR(&utf8_buffer_info->writer);
//     return true;
// }

// force_inline bool BYTES_BUFFER_APPEND_ARR_END(EncodeUTF8BufferInfo *utf8_buffer_info, Py_ssize_t cur_nested_depth) {
//     // remove last comma
//     (utf8_buffer_info->writer)--;
//     // this is not a *value*, the indent is always needed. i.e. `is_in_obj` should always pass false
//     WRITE_INDENT_RETURN_IF_FAIL(utf8_buffer_info, cur_nested_depth, false, TAIL_PADDING);
//     WRITE_BYTES_ARR_END(&utf8_buffer_info->writer);
//     return true;
// }

// force_inline bool BYTES_BUFFER_APPEND_OBJ_BEGIN(EncodeUTF8BufferInfo *utf8_buffer_info, Py_ssize_t cur_nested_depth, bool is_in_obj) {
//     WRITE_INDENT_RETURN_IF_FAIL(utf8_buffer_info, cur_nested_depth, is_in_obj, TAIL_PADDING);
//     WRITE_BYTES_OBJ_BEGIN(&utf8_buffer_info->writer);
//     return true;
// }

// force_inline bool BYTES_BUFFER_APPEND_EMPTY_OBJ(EncodeUTF8BufferInfo *utf8_buffer_info, Py_ssize_t cur_nested_depth, bool is_in_obj) {
//     WRITE_INDENT_RETURN_IF_FAIL(utf8_buffer_info, cur_nested_depth, is_in_obj, TAIL_PADDING);
//     WRITE_BYTES_EMPTY_OBJ(&utf8_buffer_info->writer);
//     return true;
// }

// force_inline bool BYTES_BUFFER_APPEND_OBJ_END(EncodeUTF8BufferInfo *utf8_buffer_info, Py_ssize_t cur_nested_depth) {
//     // remove last comma
//     (utf8_buffer_info->writer)--;
//     // this is not a *value*, the indent is always needed. i.e. `is_in_obj` should always pass false
//     WRITE_INDENT_RETURN_IF_FAIL(utf8_buffer_info, cur_nested_depth, false, TAIL_PADDING);
//     WRITE_BYTES_OBJ_END(&utf8_buffer_info->writer);
//     return true;
// }

typedef enum EncodeBytesValJumpFlag {
    BytesJumpFlag_Default,
    BytesJumpFlag_ArrValBegin,
    BytesJumpFlag_DictPairBegin,
    BytesJumpFlag_TupleValBegin,
    BytesJumpFlag_Fail,
} EncodeBytesValJumpFlag;

force_inline EncodeBytesValJumpFlag ENCODE_PROCESS_BYTES_VAL(
        EncodeUTF8BufferInfo *utf8_buffer_info, PyObject *val,
        EncodeUTF8StackVars *stack_vars, bool is_in_obj) {
#define CTN_SIZE_GROW()                                                               \
    do {                                                                              \
        if (unlikely(stack_vars->cur_nested_depth == PYYJSON_ENCODE_MAX_RECURSION)) { \
            PyErr_SetString(JSONEncodeError, "Too many nested structures");           \
            return BytesJumpFlag_Fail;                                                \
        }                                                                             \
    } while (0)
#define RETURN_JUMP_FAIL_ON_UNLIKELY_ERR(_cond_) \
    do {                                         \
        if (unlikely(_cond_)) {                  \
            return BytesJumpFlag_Fail;           \
        }                                        \
    } while (0)

    PyFastTypes fast_type = fast_type_check(val);

    switch (fast_type) {
        case T_Unicode: {
            RETURN_JUMP_FAIL_ON_UNLIKELY_ERR(!BYTES_BUFFER_APPEND_STR(val, utf8_buffer_info, stack_vars->cur_nested_depth, is_in_obj));
            // #if COMPILE_UCS_LEVEL < 1
            //             if (unlikely(stack_vars->unicode_info.cur_ucs_type == 1)) {
            //                 return is_in_obj ? JumpFlag_Elevate1_ObjVal : JumpFlag_Elevate1_ArrVal;
            //             }
            // #endif
            // #if COMPILE_UCS_LEVEL < 2
            //             if (unlikely(stack_vars->unicode_info.cur_ucs_type == 2)) {
            //                 return is_in_obj ? JumpFlag_Elevate2_ObjVal : JumpFlag_Elevate2_ArrVal;
            //             }
            // #endif
            // #if COMPILE_UCS_LEVEL < 4
            //             if (unlikely(stack_vars->unicode_info.cur_ucs_type == 4)) {
            //                 return is_in_obj ? JumpFlag_Elevate4_ObjVal : JumpFlag_Elevate4_ArrVal;
            //             }
            // #endif
            break;
        }
        case T_Long: {
            RETURN_JUMP_FAIL_ON_UNLIKELY_ERR(!BYTES_BUFFER_APPEND_LONG(utf8_buffer_info, stack_vars->cur_nested_depth, val, is_in_obj));
            break;
        }
        case T_False: {
            RETURN_JUMP_FAIL_ON_UNLIKELY_ERR(!BYTES_BUFFER_APPEND_FALSE(utf8_buffer_info, stack_vars->cur_nested_depth, is_in_obj));
            break;
        }
        case T_True: {
            RETURN_JUMP_FAIL_ON_UNLIKELY_ERR(!BYTES_BUFFER_APPEND_TRUE(utf8_buffer_info, stack_vars->cur_nested_depth, is_in_obj));
            break;
        }
        case T_None: {
            RETURN_JUMP_FAIL_ON_UNLIKELY_ERR(!BYTES_BUFFER_APPEND_NULL(utf8_buffer_info, stack_vars->cur_nested_depth, is_in_obj));
            break;
        }
        case T_Float: {
            RETURN_JUMP_FAIL_ON_UNLIKELY_ERR(!BYTES_BUFFER_APPEND_FLOAT(utf8_buffer_info, stack_vars->cur_nested_depth, val, is_in_obj));
            break;
        }
        case T_List: {
            Py_ssize_t this_list_size = PyList_GET_SIZE(val);
            if (unlikely(this_list_size == 0)) {
                RETURN_JUMP_FAIL_ON_UNLIKELY_ERR(!BYTES_BUFFER_APPEND_EMPTY_ARR(utf8_buffer_info, stack_vars->cur_nested_depth, is_in_obj));
            } else {
                RETURN_JUMP_FAIL_ON_UNLIKELY_ERR(!BYTES_BUFFER_APPEND_ARR_BEGIN(utf8_buffer_info, stack_vars->cur_nested_depth, is_in_obj));
                CTN_SIZE_GROW();
                EncodeCtnWithIndex *cur_write_ctn = stack_vars->ctn_stack + (stack_vars->cur_nested_depth++);
                cur_write_ctn->ctn = stack_vars->cur_obj;
                cur_write_ctn->index = stack_vars->cur_pos;
                stack_vars->cur_obj = val;
                stack_vars->cur_pos = 0;
                stack_vars->cur_list_size = this_list_size;
                return BytesJumpFlag_ArrValBegin;
            }
            break;
        }
        case T_Dict: {
            if (unlikely(PyDict_GET_SIZE(val) == 0)) {
                RETURN_JUMP_FAIL_ON_UNLIKELY_ERR(!BYTES_BUFFER_APPEND_EMPTY_OBJ(utf8_buffer_info, stack_vars->cur_nested_depth, is_in_obj));
            } else {
                RETURN_JUMP_FAIL_ON_UNLIKELY_ERR(!BYTES_BUFFER_APPEND_OBJ_BEGIN(utf8_buffer_info, stack_vars->cur_nested_depth, is_in_obj));
                CTN_SIZE_GROW();
                EncodeCtnWithIndex *cur_write_ctn = stack_vars->ctn_stack + (stack_vars->cur_nested_depth++);
                cur_write_ctn->ctn = stack_vars->cur_obj;
                cur_write_ctn->index = stack_vars->cur_pos;
                stack_vars->cur_obj = val;
                stack_vars->cur_pos = 0;
                return BytesJumpFlag_DictPairBegin;
            }
            break;
        }
        case T_Tuple: {
            Py_ssize_t this_list_size = PyTuple_Size(val);
            if (unlikely(this_list_size == 0)) {
                RETURN_JUMP_FAIL_ON_UNLIKELY_ERR(!BYTES_BUFFER_APPEND_EMPTY_ARR(utf8_buffer_info, stack_vars->cur_nested_depth, is_in_obj));
            } else {
                RETURN_JUMP_FAIL_ON_UNLIKELY_ERR(!BYTES_BUFFER_APPEND_ARR_BEGIN(utf8_buffer_info, stack_vars->cur_nested_depth, is_in_obj));
                CTN_SIZE_GROW();
                EncodeCtnWithIndex *cur_write_ctn = stack_vars->ctn_stack + (stack_vars->cur_nested_depth++);
                cur_write_ctn->ctn = stack_vars->cur_obj;
                cur_write_ctn->index = stack_vars->cur_pos;
                stack_vars->cur_obj = val;
                stack_vars->cur_pos = 0;
                stack_vars->cur_list_size = this_list_size;
                return BytesJumpFlag_TupleValBegin;
            }
            break;
        }
        default: {
            PyErr_SetString(JSONEncodeError, "Unsupported type");
            return BytesJumpFlag_Fail;
        }
    }

    return BytesJumpFlag_Default;
#undef RETURN_JUMP_FAIL_ON_UNLIKELY_ERR
#undef CTN_SIZE_GROW
}

#define PYYJSON_DUMPS_OBJ_BYTES PYYJSON_CONCAT2(pyyjson_dumps_obj_bytes, COMPILE_INDENT_LEVEL)

static force_noinline PyObject *
PYYJSON_DUMPS_OBJ_BYTES(
        PyObject *in_obj) {
#define GOTO_FAIL_ON_UNLIKELY_ERR(_condition) \
    do {                                      \
        if (unlikely(_condition)) goto fail;  \
    } while (0)

    // #if COMPILE_UCS_LEVEL == 0
    EncodeUTF8BufferInfo _utf8_buffer_info;
    EncodeUTF8StackVars _stack_vars;
    GOTO_FAIL_ON_UNLIKELY_ERR(!init_utf8_buffer(&_utf8_buffer_info) || !init_utf8_stack_vars(&_stack_vars, in_obj));

    // this is the starting, we don't need an indent before container.
    // so is_in_obj always pass true
    if (PyDict_CheckExact(_stack_vars.cur_obj)) {
        if (unlikely(PyDict_GET_SIZE(_stack_vars.cur_obj) == 0)) {
            bool _c = BYTES_BUFFER_APPEND_EMPTY_OBJ(&_utf8_buffer_info, _stack_vars.cur_nested_depth, true);
            assert(_c);
            goto success;
        }
        {
            bool _c = BYTES_BUFFER_APPEND_OBJ_BEGIN(&_utf8_buffer_info, _stack_vars.cur_nested_depth, true);
            assert(_c);
        }
        assert(!_stack_vars.cur_nested_depth);
        _stack_vars.cur_nested_depth = 1;
        // NOTE: ctn_stack[0] is always invalid
        goto dict_pair_begin;
    } else if (PyList_CheckExact(_stack_vars.cur_obj)) {
        _stack_vars.cur_list_size = PyList_GET_SIZE(_stack_vars.cur_obj);
        if (unlikely(_stack_vars.cur_list_size == 0)) {
            bool _c = BYTES_BUFFER_APPEND_EMPTY_ARR(&_utf8_buffer_info, _stack_vars.cur_nested_depth, true);
            assert(_c);
            goto success;
        }
        {
            bool _c = BYTES_BUFFER_APPEND_ARR_BEGIN(&_utf8_buffer_info, _stack_vars.cur_nested_depth, true);
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
            bool _c = BYTES_BUFFER_APPEND_EMPTY_ARR(&_utf8_buffer_info, _stack_vars.cur_nested_depth, true);
            assert(_c);
            goto success;
        }
        {
            bool _c = BYTES_BUFFER_APPEND_ARR_BEGIN(&_utf8_buffer_info, _stack_vars.cur_nested_depth, true);
            assert(_c);
        }
        assert(!_stack_vars.cur_nested_depth);
        _stack_vars.cur_nested_depth = 1;
        _stack_vars.cur_is_tuple = true;
        goto arr_val_begin;
    }

    PYYJSON_UNREACHABLE();
    // #else
    //     switch (encode_call_flag) {
    //         case CallFlag_ArrVal: {
    //             if (PyList_CheckExact(_stack_vars.cur_obj)) {
    //                 _stack_vars.cur_is_tuple = false;
    //                 goto arr_val_begin;
    //             } else {
    //                 _stack_vars.cur_is_tuple = true;
    //                 goto arr_val_begin;
    //             }
    //             break;
    //         }
    //         case CallFlag_ObjVal: {
    //             goto dict_pair_begin;
    //             break;
    //         }
    //         case CallFlag_Key: {
    //             goto dict_key_done;
    //             break;
    //         }
    //         default: {
    //             PYYJSON_UNREACHABLE();
    //             break;
    //         }
    //     }
    //     PYYJSON_UNREACHABLE();
    // #endif

dict_pair_begin:;
    assert(PyDict_GET_SIZE(_stack_vars.cur_obj) != 0);
    if (pydict_next(_stack_vars.cur_obj, &_stack_vars.cur_pos, &_stack_vars.key, &_stack_vars.val)) {
        if (unlikely(!PyUnicode_CheckExact(_stack_vars.key))) {
            goto fail_keytype;
        }
        GOTO_FAIL_ON_UNLIKELY_ERR(!BYTES_BUFFER_APPEND_KEY(_stack_vars.key, &_utf8_buffer_info, _stack_vars.cur_nested_depth));
        //         {
        // #if COMPILE_UCS_LEVEL < 1
        //             if (unlikely(_stack_vars.unicode_info.cur_ucs_type == 1)) {
        //                 return PYYJSON_CONCAT3(pyyjson_dumps_obj_bytes, COMPILE_INDENT_LEVEL, 1)(_stack_vars, _utf8_buffer_info, CallFlag_Key);
        //             }
        // #endif
        // #if COMPILE_UCS_LEVEL < 2
        //             if (unlikely(_stack_vars.unicode_info.cur_ucs_type == 2)) {
        //                 return PYYJSON_CONCAT3(pyyjson_dumps_obj_bytes, COMPILE_INDENT_LEVEL, 2)(_stack_vars, _utf8_buffer_info, CallFlag_Key);
        //             }
        // #endif
        // #if COMPILE_UCS_LEVEL < 4
        //             if (unlikely(_stack_vars.unicode_info.cur_ucs_type == 4)) {
        //                 return PYYJSON_CONCAT3(pyyjson_dumps_obj_bytes, COMPILE_INDENT_LEVEL, 4)(_stack_vars, _utf8_buffer_info, CallFlag_Key);
        //             }
        // #endif
        //         }
    dict_key_done:;
        //
        EncodeBytesValJumpFlag jump_flag = ENCODE_PROCESS_BYTES_VAL(&_utf8_buffer_info, _stack_vars.val, &_stack_vars, true);
        switch ((jump_flag)) {
            case BytesJumpFlag_Default: {
                break;
            }
            case BytesJumpFlag_ArrValBegin: {
                _stack_vars.cur_is_tuple = false;
                goto arr_val_begin;
            }
            case BytesJumpFlag_DictPairBegin: {
                goto dict_pair_begin;
            }
            case BytesJumpFlag_TupleValBegin: {
                _stack_vars.cur_is_tuple = true;
                goto arr_val_begin;
            }
            case BytesJumpFlag_Fail: {
                goto fail;
            }
                // #if COMPILE_UCS_LEVEL < 1
                //             case JumpFlag_Elevate1_ObjVal: {
                //                 return PYYJSON_CONCAT3(pyyjson_dumps_obj_bytes, COMPILE_INDENT_LEVEL, 1)(_stack_vars, _utf8_buffer_info, CallFlag_ObjVal);
                //             }
                // #endif
                // #if COMPILE_UCS_LEVEL < 2
                //             case JumpFlag_Elevate2_ObjVal: {
                //                 return PYYJSON_CONCAT3(pyyjson_dumps_obj_bytes, COMPILE_INDENT_LEVEL, 2)(_stack_vars, _utf8_buffer_info, CallFlag_ObjVal);
                //             }
                // #endif
                // #if COMPILE_UCS_LEVEL < 4
                //             case JumpFlag_Elevate4_ObjVal: {
                //                 return PYYJSON_CONCAT3(pyyjson_dumps_obj_bytes, COMPILE_INDENT_LEVEL, 4)(_stack_vars, _utf8_buffer_info, CallFlag_ObjVal);
                //             }
                // #endif
            default: {
                PYYJSON_UNREACHABLE();
            }
        }
        goto dict_pair_begin;
    } else {
        // dict end
        assert(_stack_vars.cur_nested_depth);
        EncodeCtnWithIndex *last_pos = _stack_vars.ctn_stack + (--_stack_vars.cur_nested_depth);

        GOTO_FAIL_ON_UNLIKELY_ERR(!BYTES_BUFFER_APPEND_OBJ_END(&_utf8_buffer_info, _stack_vars.cur_nested_depth));
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

    PYYJSON_UNREACHABLE();

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
        EncodeBytesValJumpFlag jump_flag = ENCODE_PROCESS_BYTES_VAL(&_utf8_buffer_info, _stack_vars.val, &_stack_vars, false);
        switch ((jump_flag)) {
            case BytesJumpFlag_Default: {
                break;
            }
            case BytesJumpFlag_ArrValBegin: {
                _stack_vars.cur_is_tuple = false;
                goto arr_val_begin;
            }
            case BytesJumpFlag_DictPairBegin: {
                goto dict_pair_begin;
            }
            case BytesJumpFlag_TupleValBegin: {
                _stack_vars.cur_is_tuple = true;
                goto arr_val_begin;
            }
            case BytesJumpFlag_Fail: {
                goto fail;
            }
                // #if COMPILE_UCS_LEVEL < 1
                //             case JumpFlag_Elevate1_ArrVal: {
                //                 return PYYJSON_CONCAT3(pyyjson_dumps_obj_bytes, COMPILE_INDENT_LEVEL, 1)(_stack_vars, _utf8_buffer_info, CallFlag_ArrVal);
                //             }
                // #endif
                // #if COMPILE_UCS_LEVEL < 2
                //             case JumpFlag_Elevate2_ArrVal: {
                //                 return PYYJSON_CONCAT3(pyyjson_dumps_obj_bytes, COMPILE_INDENT_LEVEL, 2)(_stack_vars, _utf8_buffer_info, CallFlag_ArrVal);
                //             }
                // #endif
                // #if COMPILE_UCS_LEVEL < 4
                //             case JumpFlag_Elevate4_ArrVal: {
                //                 return PYYJSON_CONCAT3(pyyjson_dumps_obj_bytes, COMPILE_INDENT_LEVEL, 4)(_stack_vars, _utf8_buffer_info, CallFlag_ArrVal);
                //             }
                // #endif
            default: {
                PYYJSON_UNREACHABLE();
            }
        }
        //
        goto arr_val_begin;
    } else {
        // list end
        assert(_stack_vars.cur_nested_depth);
        EncodeCtnWithIndex *last_pos = _stack_vars.ctn_stack + (--_stack_vars.cur_nested_depth);

        GOTO_FAIL_ON_UNLIKELY_ERR(!BYTES_BUFFER_APPEND_ARR_END(&_utf8_buffer_info, _stack_vars.cur_nested_depth));
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
    PYYJSON_UNREACHABLE();

success:;
    assert(_stack_vars.cur_nested_depth == 0);
    // remove trailing comma
    (_utf8_buffer_info.writer)--;

    // #if COMPILE_UCS_LEVEL == 4
    //     ucs2_elevate4(&_utf8_buffer_info, &_stack_vars.unicode_info);
    //     ucs1_elevate4(&_utf8_buffer_info, &_stack_vars.unicode_info);
    //     ascii_elevate4(&_utf8_buffer_info, &_stack_vars.unicode_info);
    // #endif
    // #if COMPILE_UCS_LEVEL == 2
    //     ucs1_elevate2(&_utf8_buffer_info, &_stack_vars.unicode_info);
    //     ascii_elevate2(&_utf8_buffer_info, &_stack_vars.unicode_info);
    // #endif
    // #if COMPILE_UCS_LEVEL == 1
    //     ascii_elevate1(&_utf8_buffer_info, &_stack_vars.unicode_info);
    // #endif
    Py_ssize_t final_len = _utf8_buffer_info.writer - PYYJSON_CAST(u8 *, _utf8_buffer_info.head) + PYBYTES_START_OFFSET;
    // Py_ssize_t final_len = GET_UNICODE_BUFFER_FINAL_LEN(&_utf8_buffer_info);
    {
        int err = _PyBytes_Resize(PYYJSON_CAST(PyObject **, &_utf8_buffer_info.head), final_len);
        GOTO_FAIL_ON_UNLIKELY_ERR(err);
    }

    init_pybytes(_utf8_buffer_info.head, final_len);
    return (PyObject *)_utf8_buffer_info.head;
fail:;
    if (_utf8_buffer_info.head) {
        PyObject_Free(_utf8_buffer_info.head);
    }
    return NULL;
fail_ctntype:;
    PyErr_SetString(JSONEncodeError, "Unsupported type");
    goto fail;
fail_keytype:;
    PyErr_SetString(JSONEncodeError, "Expected `str` as key");
    goto fail;
}

#undef ENCODE_PROCESS_BYTES_VAL

#undef BYTES_BUFFER_APPEND_OBJ_END
#undef BYTES_BUFFER_APPEND_EMPTY_OBJ
#undef BYTES_BUFFER_APPEND_OBJ_BEGIN
#undef BYTES_BUFFER_APPEND_ARR_END
#undef BYTES_BUFFER_APPEND_EMPTY_ARR
#undef BYTES_BUFFER_APPEND_ARR_BEGIN

#undef BYTES_BUFFER_APPEND_NULL
#undef BYTES_BUFFER_APPEND_TRUE
#undef BYTES_BUFFER_APPEND_FALSE

#undef BYTES_BUFFER_APPEND_FLOAT
#undef BYTES_BUFFER_APPEND_LONG
#undef BYTES_BUFFER_APPEND_STR
#undef BYTES_BUFFER_APPEND_KEY

#undef WRITE_BYTES_OBJ_END
#undef WRITE_BYTES_EMPTY_OBJ
#undef WRITE_BYTES_OBJ_BEGIN
#undef WRITE_BYTES_ARR_END
#undef WRITE_BYTES_EMPTY_ARR
#undef WRITE_BYTES_ARR_BEGIN
#undef WRITE_BYTES_NULL
#undef WRITE_BYTES_TRUE
#undef WRITE_BYTES_FALSE
#undef WRITE_INDENT_RETURN_IF_FAIL
#undef BYTES_INDENT_WRITER
