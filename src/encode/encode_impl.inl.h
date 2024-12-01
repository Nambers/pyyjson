#ifndef COMPILE_UCS_LEVEL
#error "COMPILE_UCS_LEVEL is not defined"
#endif

#ifndef COMPILE_INDENT_LEVEL
#error "COMPILE_INDENT_LEVEL is not defined"
#endif

#if COMPILE_UCS_LEVEL <= 1
#define COMPILE_READ_UCS_LEVEL 1
#define COMPILE_WRITE_UCS_LEVEL 1
#else
#define COMPILE_READ_UCS_LEVEL COMPILE_UCS_LEVEL
#define COMPILE_WRITE_UCS_LEVEL COMPILE_UCS_LEVEL
#endif

#include "commondef/w_in.inl.h"
#include "unicode/include/indent.h"

#define VEC_WRITE_U64 PYYJSON_CONCAT2(vec_write_u64, COMPILE_WRITE_UCS_LEVEL)
#define VEC_WRITE_F64 PYYJSON_CONCAT2(vec_write_f64, COMPILE_WRITE_UCS_LEVEL)


#define WRITE_INDENT_RETURN_IF_FAIL(vec_addr, cur_nested_depth, _is_in_obj, _additional_reserve_count) \
    do {                                                                                               \
        vec = INDENT_WRITER(vec_addr, cur_nested_depth, _is_in_obj, _additional_reserve_count);        \
        if (unlikely(!vec)) return false;                                                              \
    } while (0)

#define _CTN_STACK(stack_vars) ((void) 0, stack_vars->ctn_stack)

#define VEC_BACK1 PYYJSON_CONCAT2(vec_back1, COMPILE_UCS_LEVEL)
#if COMPILE_INDENT_LEVEL == 0
// avoid compile again
force_inline void VEC_BACK1(UnicodeVector *vec) {
    (_WRITER(vec))--;
}
#endif


#define _PREPARE_UNICODE_WRITE PYYJSON_CONCAT3(prepare_unicode_write, COMPILE_INDENT_LEVEL, COMPILE_UCS_LEVEL)

force_inline void _PREPARE_UNICODE_WRITE(PyObject *obj, UnicodeVector *restrict vec, UnicodeInfo *restrict unicode_info, Py_ssize_t *out_len, int *read_kind, int *write_kind) {
    Py_ssize_t out_len_val = PyUnicode_GET_LENGTH(obj);
    *out_len = out_len_val;
    int read_kind_val = PyUnicode_KIND(obj);
    *read_kind = read_kind_val;

#if COMPILE_UCS_LEVEL == 4
    *write_kind = 4;
#elif COMPILE_UCS_LEVEL == 2
    *write_kind = 2;
    if (unlikely(read_kind_val == 4)) {
        memorize_ucs2_to_ucs4(vec, unicode_info);
        *write_kind = 4;
    }
#elif COMPILE_UCS_LEVEL == 1
    *write_kind = 1;
    if (unlikely(read_kind_val == 2)) {
        memorize_ucs1_to_ucs2(vec, unicode_info);
        *write_kind = 2;
    } else if (unlikely(read_kind_val == 4)) {
        memorize_ucs1_to_ucs4(vec, unicode_info);
        *write_kind = 4;
    }
#elif COMPILE_UCS_LEVEL == 0
    *write_kind = 0;
    if (unlikely(read_kind_val == 2)) {
        memorize_ascii_to_ucs2(vec, unicode_info);
        *write_kind = 2;
    } else if (unlikely(read_kind_val == 4)) {
        memorize_ascii_to_ucs4(vec, unicode_info);
        *write_kind = 4;
    } else if (unlikely(!PyUnicode_IS_ASCII(obj))) {
        memorize_ascii_to_ucs1(vec, unicode_info);
        *write_kind = 1;
    }
#endif
}

#define VECTOR_APPEND_KEY PYYJSON_CONCAT3(vector_append_key, COMPILE_INDENT_LEVEL, COMPILE_UCS_LEVEL)

force_inline bool VECTOR_APPEND_KEY(PyObject *key, UnicodeVector **restrict vec_addr, UnicodeInfo *unicode_info, Py_ssize_t cur_nested_depth) {
    Py_ssize_t len;
    int kind, write_kind;
    bool _c;
    _PREPARE_UNICODE_WRITE(key, *vec_addr, unicode_info, &len, &kind, &write_kind);

    switch (write_kind) {
#if COMPILE_UCS_LEVEL < 1
        case 0:
#endif
#if COMPILE_UCS_LEVEL < 2
        case 1: {
            _c = PYYJSON_CONCAT4(vec_write_key, COMPILE_INDENT_LEVEL, 1, 1)(key, len, vec_addr, cur_nested_depth);
            RETURN_ON_UNLIKELY_ERR(!_c);
            break;
        }
#endif
#if COMPILE_UCS_LEVEL < 4
        case 2: {
            switch (kind) {
                case 1: {
                    _c = PYYJSON_CONCAT4(vec_write_key, COMPILE_INDENT_LEVEL, 1, 2)(key, len, vec_addr, cur_nested_depth);
                    break;
                }
                case 2: {
                    _c = PYYJSON_CONCAT4(vec_write_key, COMPILE_INDENT_LEVEL, 2, 2)(key, len, vec_addr, cur_nested_depth);
                    break;
                }
                default: {
                    assert(false);
                    Py_UNREACHABLE();
                }
            }
            RETURN_ON_UNLIKELY_ERR(!_c);
            break;
        }
#endif
        case 4: {
            switch (kind) {
                case 1: {
                    _c = PYYJSON_CONCAT4(vec_write_key, COMPILE_INDENT_LEVEL, 1, 4)(key, len, vec_addr, cur_nested_depth);
                    break;
                }
                case 2: {
                    _c = PYYJSON_CONCAT4(vec_write_key, COMPILE_INDENT_LEVEL, 2, 4)(key, len, vec_addr, cur_nested_depth);
                    break;
                }
                case 4: {
                    _c = PYYJSON_CONCAT4(vec_write_key, COMPILE_INDENT_LEVEL, 4, 4)(key, len, vec_addr, cur_nested_depth);
                    break;
                }
                default: {
                    assert(false);
                    Py_UNREACHABLE();
                }
            }
            RETURN_ON_UNLIKELY_ERR(!_c);
            break;
        }
        default: {
            assert(false);
            Py_UNREACHABLE();
        }
    }
    return true;
}


#define VECTOR_APPEND_STR PYYJSON_CONCAT3(vector_append_str, COMPILE_INDENT_LEVEL, COMPILE_UCS_LEVEL)

force_inline bool VECTOR_APPEND_STR(PyObject *val, UnicodeVector **restrict vec_addr, UnicodeInfo *unicode_info, Py_ssize_t cur_nested_depth, bool is_in_obj) {
    Py_ssize_t len;
    int kind, write_kind;
    bool _c;
    _PREPARE_UNICODE_WRITE(val, *vec_addr, unicode_info, &len, &kind, &write_kind);
    switch (write_kind) {
#if COMPILE_UCS_LEVEL < 1
        case 0:
#endif
#if COMPILE_UCS_LEVEL < 2
        case 1: {
            _c = PYYJSON_CONCAT4(vec_write_str, COMPILE_INDENT_LEVEL, 1, 1)(val, len, vec_addr, cur_nested_depth, is_in_obj);
            RETURN_ON_UNLIKELY_ERR(!_c);
            break;
        }
#endif
#if COMPILE_UCS_LEVEL < 4
        case 2: {
            switch (kind) {
                case 1: {
                    _c = PYYJSON_CONCAT4(vec_write_str, COMPILE_INDENT_LEVEL, 1, 2)(val, len, vec_addr, cur_nested_depth, is_in_obj);
                    break;
                }
                case 2: {
                    _c = PYYJSON_CONCAT4(vec_write_str, COMPILE_INDENT_LEVEL, 2, 2)(val, len, vec_addr, cur_nested_depth, is_in_obj);
                    break;
                }
                default: {
                    assert(false);
                    Py_UNREACHABLE();
                }
            }
            RETURN_ON_UNLIKELY_ERR(!_c);
            break;
        }
#endif
        case 4: {
            switch (kind) {
                case 1: {
                    _c = PYYJSON_CONCAT4(vec_write_str, COMPILE_INDENT_LEVEL, 1, 4)(val, len, vec_addr, cur_nested_depth, is_in_obj);
                    break;
                }
                case 2: {
                    _c = PYYJSON_CONCAT4(vec_write_str, COMPILE_INDENT_LEVEL, 2, 4)(val, len, vec_addr, cur_nested_depth, is_in_obj);
                    break;
                }
                case 4: {
                    _c = PYYJSON_CONCAT4(vec_write_str, COMPILE_INDENT_LEVEL, 4, 4)(val, len, vec_addr, cur_nested_depth, is_in_obj);
                    break;
                }
                default: {
                    assert(false);
                    Py_UNREACHABLE();
                }
            }
            RETURN_ON_UNLIKELY_ERR(!_c);
            break;
        }
        default: {
            assert(false);
            Py_UNREACHABLE();
        }
    }
    return true;
}

#define VECTOR_APPEND_LONG PYYJSON_CONCAT3(vector_append_long, COMPILE_INDENT_LEVEL, COMPILE_UCS_LEVEL)

force_inline bool VECTOR_APPEND_LONG(UnicodeVector **restrict vec_addr, Py_ssize_t cur_nested_depth, PyObject *val, StackVars *stack_vars, bool is_in_obj) {
    assert(PyLong_CheckExact(val));
    // 32 < TAIL_PADDING == 64 so this is enough
    UnicodeVector *vec = *vec_addr;
    WRITE_INDENT_RETURN_IF_FAIL(vec_addr, cur_nested_depth, is_in_obj, TAIL_PADDING);

    if (pylong_is_zero(val)) {
        _TARGET_TYPE *writer = _WRITER(vec);
        *writer++ = '0';
        *writer++ = ',';
        _WRITER(vec) += 2;
    } else {
        // u8 *const buffer = (u8 *) reserve_view_data_buffer<u8[32]>(viewer);
        u64 v;
        usize sign;
        if (pylong_is_unsigned(val)) {
            bool _c = pylong_value_unsigned(val, &v);
            RETURN_ON_UNLIKELY_ERR(!_c);
            sign = 0;
        } else {
            i64 v2;
            bool _c = pylong_value_signed(val, &v2);
            RETURN_ON_UNLIKELY_ERR(!_c);
            assert(v2 <= 0);
            v = -v2;
            sign = 1;
        }
        VEC_WRITE_U64(vec, v, sign);
        *_WRITER(vec)++ = ',';
    }
    assert(vec_in_boundary(vec));
    return true;
}

#define VECTOR_APPEND_FALSE PYYJSON_CONCAT3(vector_append_false, COMPILE_INDENT_LEVEL, COMPILE_UCS_LEVEL)

force_inline bool VECTOR_APPEND_FALSE(UnicodeVector **restrict vec_addr, Py_ssize_t cur_nested_depth, bool is_in_obj) {
    UnicodeVector *vec = *vec_addr;
    WRITE_INDENT_RETURN_IF_FAIL(vec_addr, cur_nested_depth, is_in_obj, TAIL_PADDING);
    _TARGET_TYPE *writer = _WRITER(vec);
    //   6,12,24
    //-> 8,16,24/32 (64)
    //-> 8,12,24 (32)
    *writer++ = 'f';
    *writer++ = 'a';
    *writer++ = 'l';
    *writer++ = 's';
    *writer++ = 'e';
    *writer++ = ',';
#if COMPILE_UCS_LEVEL == 1
    *writer++ = 0;
    *writer++ = 0;
#elif COMPILE_UCS_LEVEL == 2
#if SIZEOF_VOID_P == 8
    *writer++ = 0;
    *writer++ = 0;
#endif // SIZEOF_VOID_P
#else  // COMPILE_UCS_LEVEL == 4
#if __AVX__
    *writer++ = 0;
    *writer++ = 0;
#endif // __AVX__
#endif // COMPILE_UCS_LEVEL
    _WRITER(vec) += 6;
    return true;
}

#define VECTOR_APPEND_TRUE PYYJSON_CONCAT3(vector_append_true, COMPILE_INDENT_LEVEL, COMPILE_UCS_LEVEL)

force_inline bool VECTOR_APPEND_TRUE(UnicodeVector **restrict vec_addr, Py_ssize_t cur_nested_depth, bool is_in_obj) {
    UnicodeVector *vec = *vec_addr;
    WRITE_INDENT_RETURN_IF_FAIL(vec_addr, cur_nested_depth, is_in_obj, TAIL_PADDING);
    _TARGET_TYPE *writer = _WRITER(vec);
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
#if SIZEOF_VOID_P == 8
    *writer++ = 0;
    *writer++ = 0;
#endif // SIZEOF_VOID_P == 8
#else  // COMPILE_UCS_LEVEL == 4
#if SIZEOF_VOID_P == 8
    *writer++ = 0;
#if __AVX__
    *writer++ = 0;
    *writer++ = 0;
#endif // __AVX__
#endif
#endif // COMPILE_UCS_LEVEL
    _WRITER(vec) += 5;
    return true;
}

#define VECTOR_APPEND_NULL PYYJSON_CONCAT3(vector_append_null, COMPILE_INDENT_LEVEL, COMPILE_UCS_LEVEL)

force_inline bool VECTOR_APPEND_NULL(UnicodeVector **restrict vec_addr, Py_ssize_t cur_nested_depth, bool is_in_obj) {
    UnicodeVector *vec = *vec_addr;
    WRITE_INDENT_RETURN_IF_FAIL(vec_addr, cur_nested_depth, is_in_obj, TAIL_PADDING);
    _TARGET_TYPE *writer = _WRITER(vec);
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
#if SIZEOF_VOID_P == 8
    *writer++ = 0;
    *writer++ = 0;
#endif // SIZEOF_VOID_P == 8
#else  // COMPILE_UCS_LEVEL == 4
#if SIZEOF_VOID_P == 8
    *writer++ = 0;
#if __AVX__
    *writer++ = 0;
    *writer++ = 0;
#endif // __AVX__
#endif
#endif // COMPILE_UCS_LEVEL
    _WRITER(vec) += 5;
    return true;
}

#define VECTOR_APPEND_FLOAT PYYJSON_CONCAT3(vector_append_float, COMPILE_INDENT_LEVEL, COMPILE_UCS_LEVEL)

force_inline bool VECTOR_APPEND_FLOAT(UnicodeVector **restrict vec_addr, Py_ssize_t cur_nested_depth, PyObject *val, bool is_in_obj) {
    UnicodeVector *vec = *vec_addr;
    WRITE_INDENT_RETURN_IF_FAIL(vec_addr, cur_nested_depth, is_in_obj, TAIL_PADDING);
    double v = PyFloat_AS_DOUBLE(val);
    // u8 *const buffer = (u8 *) reserve_view_data_buffer<u8[32]>(viewer);
    u64 *raw = (u64 *) &v;
    VEC_WRITE_F64(vec, *raw);
    *_WRITER(vec)++ = ',';
    return true;
}

#define VECTOR_APPEND_EMPTY_ARR PYYJSON_CONCAT3(vector_append_empty_arr, COMPILE_INDENT_LEVEL, COMPILE_UCS_LEVEL)

force_inline bool VECTOR_APPEND_EMPTY_ARR(UnicodeVector **restrict vec_addr, Py_ssize_t cur_nested_depth, bool is_in_obj) {
    UnicodeVector *vec = *vec_addr;
    WRITE_INDENT_RETURN_IF_FAIL(vec_addr, cur_nested_depth, is_in_obj, TAIL_PADDING);
    _TARGET_TYPE *writer = _WRITER(vec);
    //   3,6,12
    //-> 4,8,16 (64)
    //-> 4,8,12 (32)
    *writer++ = '[';
    *writer++ = ']';
    *writer++ = ',';
#if SIZEOF_VOID_P == 8 || COMPILE_UCS_LEVEL != 4
    *writer = 0;
#endif
    _WRITER(vec) += 3;
    return true;
}

#define VECTOR_APPEND_ARR_BEGIN PYYJSON_CONCAT3(vector_append_arr_begin, COMPILE_INDENT_LEVEL, COMPILE_UCS_LEVEL)

force_inline bool VECTOR_APPEND_ARR_BEGIN(UnicodeVector **restrict vec_addr, Py_ssize_t cur_nested_depth, bool is_in_obj) {
    UnicodeVector *vec = *vec_addr;
    WRITE_INDENT_RETURN_IF_FAIL(vec_addr, cur_nested_depth, is_in_obj, TAIL_PADDING);
    *_WRITER(vec)++ = '[';
    return true;
}

#define VECTOR_APPEND_EMPTY_OBJ PYYJSON_CONCAT3(vector_append_empty_obj, COMPILE_INDENT_LEVEL, COMPILE_UCS_LEVEL)

force_inline bool VECTOR_APPEND_EMPTY_OBJ(UnicodeVector **restrict vec_addr, Py_ssize_t cur_nested_depth, bool is_in_obj) {
    UnicodeVector *vec = *vec_addr;
    WRITE_INDENT_RETURN_IF_FAIL(vec_addr, cur_nested_depth, is_in_obj, TAIL_PADDING);
    _TARGET_TYPE *writer = _WRITER(vec);
    //   3,6,12
    //-> 4,8,16 (64)
    //-> 4,8,12 (32)
    *writer++ = '{';
    *writer++ = '}';
    *writer++ = ',';
#if SIZEOF_VOID_P == 8 || COMPILE_UCS_LEVEL != 4
    *writer = 0;
#endif
    _WRITER(vec) += 3;
    return true;
}

#define VECTOR_APPEND_OBJ_BEGIN PYYJSON_CONCAT3(vector_append_obj_begin, COMPILE_INDENT_LEVEL, COMPILE_UCS_LEVEL)

force_inline bool VECTOR_APPEND_OBJ_BEGIN(UnicodeVector **restrict vec_addr, Py_ssize_t cur_nested_depth, bool is_in_obj) {
    UnicodeVector *vec = *vec_addr;
    WRITE_INDENT_RETURN_IF_FAIL(vec_addr, cur_nested_depth, is_in_obj, TAIL_PADDING);
    *_WRITER(vec)++ = '{';
    return true;
}

#define VECTOR_APPEND_OBJ_END PYYJSON_CONCAT3(vector_append_obj_end, COMPILE_INDENT_LEVEL, COMPILE_UCS_LEVEL)

force_inline bool VECTOR_APPEND_OBJ_END(UnicodeVector **restrict vec_addr, Py_ssize_t cur_nested_depth) {
    UnicodeVector *vec = *vec_addr;
    // remove last comma
    VEC_BACK1(vec);
    // this is not a *value*, the indent is always needed. i.e. `is_in_obj` should always pass false
    WRITE_INDENT_RETURN_IF_FAIL(vec_addr, cur_nested_depth, false, TAIL_PADDING);
    _TARGET_TYPE *writer = _WRITER(vec);
    *writer++ = '}';
    *writer++ = ',';
    _WRITER(vec) += 2;
    return true;
}

#define VECTOR_APPEND_ARR_END PYYJSON_CONCAT3(vector_append_arr_end, COMPILE_INDENT_LEVEL, COMPILE_UCS_LEVEL)

force_inline bool VECTOR_APPEND_ARR_END(UnicodeVector **restrict vec_addr, Py_ssize_t cur_nested_depth) {
    UnicodeVector *vec = *vec_addr;
    // remove last comma
    VEC_BACK1(vec);
    // this is not a *value*, the indent is always needed. i.e. `is_in_obj` should always pass false
    WRITE_INDENT_RETURN_IF_FAIL(vec_addr, cur_nested_depth, false, TAIL_PADDING);
    _TARGET_TYPE *writer = _WRITER(vec);
    *writer++ = ']';
    *writer++ = ',';
    _WRITER(vec) += 2;
    return true;
}

#define GET_VECTOR_FINAL_LEN PYYJSON_CONCAT2(get_vector_final_len, COMPILE_UCS_LEVEL)
#if COMPILE_INDENT_LEVEL == 0
// avoid compile again
force_inline Py_ssize_t GET_VECTOR_FINAL_LEN(UnicodeVector *vec) {
#if COMPILE_UCS_LEVEL == 0
    return vec->head.write_u8 - (u8 *) GET_VEC_ASCII_START(vec);
#elif COMPILE_UCS_LEVEL == 1
    return vec->head.write_u8 - (u8 *) GET_VEC_COMPACT_START(vec);
#elif COMPILE_UCS_LEVEL == 2
    return vec->head.write_u16 - (u16 *) GET_VEC_COMPACT_START(vec);
#elif COMPILE_UCS_LEVEL == 4
    return vec->head.write_u32 - (u32 *) GET_VEC_COMPACT_START(vec);
#endif
}
#endif

#define ENCODE_PROCESS_VAL PYYJSON_CONCAT3(encode_process_val, COMPILE_INDENT_LEVEL, COMPILE_UCS_LEVEL)

force_inline EncodeValJumpFlag ENCODE_PROCESS_VAL(
        PyObject *val,
        StackVars *restrict stack_vars, bool is_in_obj) {
#define CTN_SIZE_GROW()                                                               \
    do {                                                                              \
        if (unlikely(stack_vars->cur_nested_depth == PYYJSON_ENCODE_MAX_RECURSION)) { \
            PyErr_SetString(PyExc_ValueError, "Too many nested structures");          \
            return JumpFlag_Fail;                                                     \
        }                                                                             \
    } while (0)
#define RETURN_JUMP_FAIL_ON_UNLIKELY_ERR(_condition)    \
    do {                                                \
        if (unlikely(_condition)) return JumpFlag_Fail; \
    } while (0)

    PyFastTypes fast_type = fast_type_check(val);
    bool _c;
    UnicodeVector *vec = GET_VEC(stack_vars);

    switch (fast_type) {
        case T_Unicode: {
            _c = VECTOR_APPEND_STR(val, &GET_VEC(stack_vars), &stack_vars->unicode_info, stack_vars->cur_nested_depth, is_in_obj);
            RETURN_JUMP_FAIL_ON_UNLIKELY_ERR(!_c);
#if COMPILE_UCS_LEVEL < 1
            if (unlikely(stack_vars->unicode_info.cur_ucs_type == 1)) return is_in_obj ? JumpFlag_Elevate1_ObjVal : JumpFlag_Elevate1_ArrVal;
#endif
#if COMPILE_UCS_LEVEL < 2
            if (unlikely(stack_vars->unicode_info.cur_ucs_type == 2)) return is_in_obj ? JumpFlag_Elevate2_ObjVal : JumpFlag_Elevate2_ArrVal;
#endif
#if COMPILE_UCS_LEVEL < 4
            if (unlikely(stack_vars->unicode_info.cur_ucs_type == 4)) return is_in_obj ? JumpFlag_Elevate4_ObjVal : JumpFlag_Elevate4_ArrVal;
#endif
            break;
        }
        case T_Long: {
            _c = VECTOR_APPEND_LONG(&GET_VEC(stack_vars), stack_vars->cur_nested_depth, val, stack_vars, is_in_obj);
            RETURN_JUMP_FAIL_ON_UNLIKELY_ERR(!_c);
            break;
        }
        case T_False: {
            _c = VECTOR_APPEND_FALSE(&GET_VEC(stack_vars), stack_vars->cur_nested_depth, is_in_obj);
            RETURN_JUMP_FAIL_ON_UNLIKELY_ERR(!_c);
            break;
        }
        case T_True: {
            _c = VECTOR_APPEND_TRUE(&GET_VEC(stack_vars), stack_vars->cur_nested_depth, is_in_obj);
            RETURN_JUMP_FAIL_ON_UNLIKELY_ERR(!_c);
            break;
        }
        case T_None: {
            _c = VECTOR_APPEND_NULL(&GET_VEC(stack_vars), stack_vars->cur_nested_depth, is_in_obj);
            RETURN_JUMP_FAIL_ON_UNLIKELY_ERR(!_c);
            break;
        }
        case T_Float: {
            _c = VECTOR_APPEND_FLOAT(&GET_VEC(stack_vars), stack_vars->cur_nested_depth, val, is_in_obj);
            RETURN_JUMP_FAIL_ON_UNLIKELY_ERR(!_c);
            break;
        }
        case T_List: {
            Py_ssize_t this_list_size = PyList_GET_SIZE(val);
            if (unlikely(this_list_size == 0)) {
                _c = VECTOR_APPEND_EMPTY_ARR(&GET_VEC(stack_vars), stack_vars->cur_nested_depth, is_in_obj);
                RETURN_JUMP_FAIL_ON_UNLIKELY_ERR(!_c);
            } else {
                _c = VECTOR_APPEND_ARR_BEGIN(&GET_VEC(stack_vars), stack_vars->cur_nested_depth, is_in_obj);
                RETURN_JUMP_FAIL_ON_UNLIKELY_ERR(!_c);
                CTN_SIZE_GROW();
                CtnType *cur_write_ctn = _CTN_STACK(stack_vars) + (stack_vars->cur_nested_depth++);
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
                _c = VECTOR_APPEND_EMPTY_OBJ(&GET_VEC(stack_vars), stack_vars->cur_nested_depth, is_in_obj);
                RETURN_JUMP_FAIL_ON_UNLIKELY_ERR(!_c);
            } else {
                _c = VECTOR_APPEND_OBJ_BEGIN(&GET_VEC(stack_vars), stack_vars->cur_nested_depth, is_in_obj);
                RETURN_JUMP_FAIL_ON_UNLIKELY_ERR(!_c);
                CTN_SIZE_GROW();
                CtnType *cur_write_ctn = _CTN_STACK(stack_vars) + (stack_vars->cur_nested_depth++);
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
                bool _c = VECTOR_APPEND_EMPTY_ARR(&GET_VEC(stack_vars), stack_vars->cur_nested_depth, is_in_obj);
                RETURN_JUMP_FAIL_ON_UNLIKELY_ERR(!_c);
            } else {
                bool _c = VECTOR_APPEND_ARR_BEGIN(&GET_VEC(stack_vars), stack_vars->cur_nested_depth, is_in_obj);
                RETURN_JUMP_FAIL_ON_UNLIKELY_ERR(!_c);
                CTN_SIZE_GROW();
                CtnType *cur_write_ctn = _CTN_STACK(stack_vars) + (stack_vars->cur_nested_depth++);
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

force_noinline PyObject *
PYYJSON_DUMPS_OBJ(
#if COMPILE_UCS_LEVEL > 0
        EncodeValJumpFlag jump_flag,
        StackVars *restrict stack_vars
#else
        PyObject *in_obj
#endif
) {
#define GOTO_FAIL_ON_UNLIKELY_ERR(_condition) \
    do {                                      \
        if (unlikely(_condition)) goto fail;  \
    } while (0)
#if COMPILE_UCS_LEVEL == 0
    StackVars _stack_vars;
    StackVars *restrict stack_vars = &_stack_vars;
    if (unlikely(!init_stack_vars(stack_vars, in_obj))) {
        goto fail;
    }
    UnicodeVector *vec = GET_VEC(stack_vars);

    // this is the starting, we don't need an indent before container.
    // so is_in_obj always pass true
    if (PyDict_CheckExact(stack_vars->cur_obj)) {
        if (unlikely(PyDict_GET_SIZE(stack_vars->cur_obj) == 0)) {
            bool _c = VECTOR_APPEND_EMPTY_OBJ(&GET_VEC(stack_vars), stack_vars->cur_nested_depth, true);
            assert(_c);
            goto success;
        }
        bool _c = VECTOR_APPEND_OBJ_BEGIN(&GET_VEC(stack_vars), stack_vars->cur_nested_depth, true);
        assert(_c);
        assert(!stack_vars->cur_nested_depth);
        stack_vars->cur_nested_depth = 1;
        // NOTE: ctn_stack[0] is always invalid
        goto dict_pair_begin;
    } else if (PyList_CheckExact(stack_vars->cur_obj)) {
        stack_vars->cur_list_size = PyList_GET_SIZE(stack_vars->cur_obj);
        if (unlikely(stack_vars->cur_list_size == 0)) {
            bool _c = VECTOR_APPEND_EMPTY_ARR(&GET_VEC(stack_vars), stack_vars->cur_nested_depth, true);
            assert(_c);
            goto success;
        }
        bool _c = VECTOR_APPEND_ARR_BEGIN(&GET_VEC(stack_vars), stack_vars->cur_nested_depth, true);
        assert(_c);
        assert(!stack_vars->cur_nested_depth);
        stack_vars->cur_nested_depth = 1;
        // NOTE: ctn_stack[0] is always invalid
        goto arr_val_begin;
    } else {
        if (unlikely(!PyTuple_CheckExact(stack_vars->cur_obj))) {
            goto fail_ctntype;
        }
        stack_vars->cur_list_size = PyTuple_GET_SIZE(stack_vars->cur_obj);
        if (unlikely(stack_vars->cur_list_size == 0)) {
            bool _c = VECTOR_APPEND_EMPTY_ARR(&GET_VEC(stack_vars), stack_vars->cur_nested_depth, true);
            assert(_c);
            goto success;
        }
        bool _c = VECTOR_APPEND_ARR_BEGIN(&GET_VEC(stack_vars), stack_vars->cur_nested_depth, true);
        assert(_c);
        assert(!stack_vars->cur_nested_depth);
        stack_vars->cur_nested_depth = 1;
        goto tuple_val_begin;
    }

    Py_UNREACHABLE();
#else
    switch (jump_flag) {
#if COMPILE_UCS_LEVEL == 1
        case JumpFlag_Elevate1_ArrVal: {
            goto arr_val_begin;
            break;
        }
        case JumpFlag_Elevate1_ObjVal: {
            goto dict_pair_begin;
            break;
        }
        case JumpFlag_Elevate1_Key: {
            goto dict_key_done;
            break;
        }
#elif COMPILE_UCS_LEVEL == 2
        case JumpFlag_Elevate2_ArrVal: {
            goto arr_val_begin;
            break;
        }
        case JumpFlag_Elevate2_ObjVal: {
            goto dict_pair_begin;
            break;
        }
        case JumpFlag_Elevate2_Key: {
            goto dict_key_done;
            break;
        }
#elif COMPILE_UCS_LEVEL == 4
        case JumpFlag_Elevate4_ArrVal: {
            goto arr_val_begin;
            break;
        }
        case JumpFlag_Elevate4_ObjVal: {
            goto dict_pair_begin;
            break;
        }
        case JumpFlag_Elevate4_Key: {
            goto dict_key_done;
            break;
        }
#endif
        default: {
            Py_UNREACHABLE();
            break;
        }
    }
    Py_UNREACHABLE();
#endif

dict_pair_begin:;
    assert(PyDict_GET_SIZE(stack_vars->cur_obj) != 0);
    if (pydict_next(stack_vars->cur_obj, &stack_vars->cur_pos, &stack_vars->key, &stack_vars->val)) {
        if (unlikely(!PyUnicode_CheckExact(stack_vars->key))) {
            goto fail_keytype;
        }
        // view_update_str_info(obj_viewer, key);
        bool _c = VECTOR_APPEND_KEY(stack_vars->key, &GET_VEC(stack_vars), &stack_vars->unicode_info, stack_vars->cur_nested_depth);
        GOTO_FAIL_ON_UNLIKELY_ERR(!_c);
        {
#if COMPILE_UCS_LEVEL < 1
            if (unlikely(stack_vars->unicode_info.cur_ucs_type == 1)) {
                return PYYJSON_CONCAT3(pyyjson_dumps_obj, COMPILE_INDENT_LEVEL, 1)(JumpFlag_Elevate1_Key, stack_vars);
            }
#endif
#if COMPILE_UCS_LEVEL < 2
            if (unlikely(stack_vars->unicode_info.cur_ucs_type == 2)) {
                return PYYJSON_CONCAT3(pyyjson_dumps_obj, COMPILE_INDENT_LEVEL, 2)(JumpFlag_Elevate2_Key, stack_vars);
            }
#endif
#if COMPILE_UCS_LEVEL < 4
            if (unlikely(stack_vars->unicode_info.cur_ucs_type == 4)) {
                return PYYJSON_CONCAT3(pyyjson_dumps_obj, COMPILE_INDENT_LEVEL, 4)(JumpFlag_Elevate4_Key, stack_vars);
            }
#endif
        }
    dict_key_done:;
        //
        EncodeValJumpFlag jump_flag = ENCODE_PROCESS_VAL(stack_vars->val, stack_vars, true);
        switch ((jump_flag)) {
            case JumpFlag_Default: {
                break;
            }
            case JumpFlag_ArrValBegin: {
                goto arr_val_begin;
            }
            case JumpFlag_DictPairBegin: {
                goto dict_pair_begin;
            }
            case JumpFlag_TupleValBegin: {
                goto tuple_val_begin;
            }
            case JumpFlag_Fail: {
                goto fail;
            }
#if COMPILE_UCS_LEVEL < 1
            case JumpFlag_Elevate1_ObjVal: {
                return PYYJSON_CONCAT3(pyyjson_dumps_obj, COMPILE_INDENT_LEVEL, 1)(JumpFlag_Elevate1_ObjVal, stack_vars);
            }
#endif
#if COMPILE_UCS_LEVEL < 2
            case JumpFlag_Elevate2_ObjVal: {
                return PYYJSON_CONCAT3(pyyjson_dumps_obj, COMPILE_INDENT_LEVEL, 2)(JumpFlag_Elevate2_ObjVal, stack_vars);
            }
#endif
#if COMPILE_UCS_LEVEL < 4
            case JumpFlag_Elevate4_ObjVal: {
                return PYYJSON_CONCAT3(pyyjson_dumps_obj, COMPILE_INDENT_LEVEL, 4)(JumpFlag_Elevate4_ObjVal, stack_vars);
            }
#endif
            default: {
                Py_UNREACHABLE();
            }
        }
        goto dict_pair_begin;
    } else {
        // dict end
        assert(stack_vars->cur_nested_depth);
        CtnType *last_pos = _CTN_STACK(stack_vars) + (--stack_vars->cur_nested_depth);

        bool _c = VECTOR_APPEND_OBJ_END(&GET_VEC(stack_vars), stack_vars->cur_nested_depth);
        GOTO_FAIL_ON_UNLIKELY_ERR(!_c);
        if (unlikely(stack_vars->cur_nested_depth == 0)) {
            goto success;
        }

        // update cur_obj and cur_pos
        stack_vars->cur_obj = last_pos->ctn;
        stack_vars->cur_pos = last_pos->index;

        if (PyDict_CheckExact(stack_vars->cur_obj)) {
            goto dict_pair_begin;
        } else if (PyList_CheckExact(stack_vars->cur_obj)) {
            stack_vars->cur_list_size = PyList_GET_SIZE(stack_vars->cur_obj);
            goto arr_val_begin;
        } else {
            assert(PyTuple_CheckExact(stack_vars->cur_obj));
            stack_vars->cur_list_size = PyTuple_GET_SIZE(stack_vars->cur_obj);
            goto tuple_val_begin;
        }
    }

    Py_UNREACHABLE();

arr_val_begin:;
    assert(stack_vars->cur_list_size != 0);

    if (stack_vars->cur_pos < stack_vars->cur_list_size) {
        stack_vars->val = PyList_GET_ITEM(stack_vars->cur_obj, stack_vars->cur_pos);
        stack_vars->cur_pos++;
        //
        EncodeValJumpFlag jump_flag = ENCODE_PROCESS_VAL(stack_vars->val, stack_vars, false);
        switch ((jump_flag)) {
            case JumpFlag_Default: {
                break;
            }
            case JumpFlag_ArrValBegin: {
                goto arr_val_begin;
            }
            case JumpFlag_DictPairBegin: {
                goto dict_pair_begin;
            }
            case JumpFlag_TupleValBegin: {
                goto tuple_val_begin;
            }
            case JumpFlag_Fail: {
                goto fail;
            }
#if COMPILE_UCS_LEVEL < 1
            case JumpFlag_Elevate1_ArrVal: {
                return PYYJSON_CONCAT3(pyyjson_dumps_obj, COMPILE_INDENT_LEVEL, 1)(JumpFlag_Elevate1_ArrVal, stack_vars);
            }
#endif
#if COMPILE_UCS_LEVEL < 2
            case JumpFlag_Elevate2_ArrVal: {
                return PYYJSON_CONCAT3(pyyjson_dumps_obj, COMPILE_INDENT_LEVEL, 2)(JumpFlag_Elevate2_ArrVal, stack_vars);
            }
#endif
#if COMPILE_UCS_LEVEL < 4
            case JumpFlag_Elevate4_ArrVal: {
                return PYYJSON_CONCAT3(pyyjson_dumps_obj, COMPILE_INDENT_LEVEL, 4)(JumpFlag_Elevate4_ArrVal, stack_vars);
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
        assert(stack_vars->cur_nested_depth);
        CtnType *last_pos = _CTN_STACK(stack_vars) + (--stack_vars->cur_nested_depth);

        bool _c = VECTOR_APPEND_ARR_END(&GET_VEC(stack_vars), stack_vars->cur_nested_depth);
        GOTO_FAIL_ON_UNLIKELY_ERR(!_c);
        if (unlikely(stack_vars->cur_nested_depth == 0)) {
            goto success;
        }

        // update cur_obj and cur_pos
        stack_vars->cur_obj = last_pos->ctn;
        stack_vars->cur_pos = last_pos->index;

        if (PyDict_CheckExact(stack_vars->cur_obj)) {
            goto dict_pair_begin;
        } else if (PyList_CheckExact(stack_vars->cur_obj)) {
            stack_vars->cur_list_size = PyList_GET_SIZE(stack_vars->cur_obj);
            goto arr_val_begin;
        } else {
            assert(PyTuple_CheckExact(stack_vars->cur_obj));
            stack_vars->cur_list_size = PyTuple_GET_SIZE(stack_vars->cur_obj);
            goto tuple_val_begin;
        }
    }
    Py_UNREACHABLE();

tuple_val_begin:;
    assert(stack_vars->cur_list_size != 0);

    if (stack_vars->cur_pos < stack_vars->cur_list_size) {
        stack_vars->val = PyTuple_GET_ITEM(stack_vars->cur_obj, stack_vars->cur_pos);
        stack_vars->cur_pos++;
        //
        EncodeValJumpFlag jump_flag = ENCODE_PROCESS_VAL(stack_vars->val, stack_vars, false);
        switch ((jump_flag)) {
            case JumpFlag_Default: {
                break;
            }
            case JumpFlag_ArrValBegin: {
                goto arr_val_begin;
            }
            case JumpFlag_DictPairBegin: {
                goto dict_pair_begin;
            }
            case JumpFlag_TupleValBegin: {
                goto tuple_val_begin;
            }
            case JumpFlag_Fail: {
                goto fail;
            }
#if COMPILE_UCS_LEVEL < 1
            case JumpFlag_Elevate1_ArrVal: {
                return PYYJSON_CONCAT3(pyyjson_dumps_obj, COMPILE_INDENT_LEVEL, 1)(JumpFlag_Elevate1_ArrVal, stack_vars);
            }
#endif
#if COMPILE_UCS_LEVEL < 2
            case JumpFlag_Elevate2_ArrVal: {
                return PYYJSON_CONCAT3(pyyjson_dumps_obj, COMPILE_INDENT_LEVEL, 2)(JumpFlag_Elevate2_ArrVal, stack_vars);
            }
#endif
#if COMPILE_UCS_LEVEL < 4
            case JumpFlag_Elevate4_ArrVal: {
                return PYYJSON_CONCAT3(pyyjson_dumps_obj, COMPILE_INDENT_LEVEL, 4)(JumpFlag_Elevate4_ArrVal, stack_vars);
            }
#endif
            default: {
                Py_UNREACHABLE();
            }
        }
        //
        goto tuple_val_begin;
    } else {
        // list end
        assert(stack_vars->cur_nested_depth);
        CtnType *last_pos = _CTN_STACK(stack_vars) + (--stack_vars->cur_nested_depth);

        bool _c = VECTOR_APPEND_ARR_END(&GET_VEC(stack_vars), stack_vars->cur_nested_depth);
        GOTO_FAIL_ON_UNLIKELY_ERR(!_c);
        if (unlikely(stack_vars->cur_nested_depth == 0)) {
            goto success;
        }

        // update cur_obj and cur_pos
        stack_vars->cur_obj = last_pos->ctn;
        stack_vars->cur_pos = last_pos->index;

        if (PyDict_CheckExact(stack_vars->cur_obj)) {
            goto dict_pair_begin;
        } else if (PyList_CheckExact(stack_vars->cur_obj)) {
            stack_vars->cur_list_size = PyList_GET_SIZE(stack_vars->cur_obj);
            goto arr_val_begin;
        } else {
            assert(PyTuple_CheckExact(stack_vars->cur_obj));
            stack_vars->cur_list_size = PyTuple_GET_SIZE(stack_vars->cur_obj);
            goto tuple_val_begin;
        }
    }
    Py_UNREACHABLE();

success:;
    assert(stack_vars->cur_nested_depth == 0);
    // remove trailing comma
    VEC_BACK1(stack_vars->vec);

#if COMPILE_UCS_LEVEL == 4
    ucs2_elevate4(GET_VEC(stack_vars), &stack_vars->unicode_info);
    ucs1_elevate4(GET_VEC(stack_vars), &stack_vars->unicode_info);
    ascii_elevate4(GET_VEC(stack_vars), &stack_vars->unicode_info);
#endif
#if COMPILE_UCS_LEVEL == 2
    ucs1_elevate2(GET_VEC(stack_vars), &stack_vars->unicode_info);
    ascii_elevate2(GET_VEC(stack_vars), &stack_vars->unicode_info);
#endif
#if COMPILE_UCS_LEVEL == 1
    ascii_elevate1(GET_VEC(stack_vars), &stack_vars->unicode_info);
#endif
    assert(stack_vars->unicode_info.cur_ucs_type == COMPILE_UCS_LEVEL);
    Py_ssize_t final_len = GET_VECTOR_FINAL_LEN(GET_VEC(stack_vars));
    bool _c = vector_resize_to_fit(&GET_VEC(stack_vars), final_len, COMPILE_UCS_LEVEL);
    GOTO_FAIL_ON_UNLIKELY_ERR(!_c);
    init_py_unicode(GET_VEC(stack_vars), final_len, COMPILE_UCS_LEVEL);
    return (PyObject *) GET_VEC(stack_vars);
fail:;
    if (GET_VEC(stack_vars)) {
        PyObject_Free(GET_VEC(stack_vars));
    }
    return NULL;
fail_ctntype:;
    PyErr_SetString(JSONEncodeError, "Unsupported type");
    goto fail;
fail_keytype:;
    PyErr_SetString(JSONEncodeError, "Expected `str` as key");
    goto fail;
}


#include "commondef/w_out.inl.h"

#undef PYYJSON_DUMPS_OBJ
#undef ENCODE_PROCESS_VAL
#undef GET_VECTOR_FINAL_LEN
#undef VECTOR_APPEND_ARR_END
#undef VECTOR_APPEND_OBJ_END
#undef VECTOR_APPEND_OBJ_BEGIN
#undef VECTOR_APPEND_EMPTY_OBJ
#undef VECTOR_APPEND_ARR_BEGIN
#undef VECTOR_APPEND_EMPTY_ARR
#undef VECTOR_APPEND_FLOAT
#undef VECTOR_APPEND_NULL
#undef VECTOR_APPEND_TRUE
#undef VECTOR_APPEND_FALSE
#undef VECTOR_APPEND_LONG
#undef VECTOR_APPEND_STR
#undef VECTOR_APPEND_KEY
#undef _PREPARE_UNICODE_WRITE
#undef VEC_BACK1
#undef _CTN_STACK
#undef WRITE_INDENT_RETURN_IF_FAIL
#undef VEC_WRITE_F64
#undef VEC_WRITE_U64
#undef COMPILE_WRITE_UCS_LEVEL
#undef COMPILE_READ_UCS_LEVEL
