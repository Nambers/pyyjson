#include "encode_shared.h"
#include "simd/cvt.h"
#include "simd/memcpy.h"
#include "simd/simd_detect.h"
#include "simd/simd_impl.h"
#include "tls.h"
#include "unicode/unicode_buffer.h"

/* Implmentations of some inline functions used in current scope */
#include "encode/indent_writer.h"
#include "unicode/reserve_wrap.h"

#include "encode_cvt.h"
#include "states.h"

typedef enum EncodeValJumpFlag {
    JumpFlag_Default,
    JumpFlag_ArrValBegin,
    JumpFlag_DictPairBegin,
    JumpFlag_TupleValBegin,
    JumpFlag_Elevate1_ArrVal,
    JumpFlag_Elevate1_ObjVal,
    JumpFlag_Elevate1_Key,
    JumpFlag_Elevate2_ArrVal,
    JumpFlag_Elevate2_ObjVal,
    JumpFlag_Elevate2_Key,
    JumpFlag_Elevate4_ArrVal,
    JumpFlag_Elevate4_ObjVal,
    JumpFlag_Elevate4_Key,
    JumpFlag_Fail,
} EncodeValJumpFlag;

typedef enum EncodeCallFlag {
    CallFlag_ObjVal,
    CallFlag_ArrVal,
    CallFlag_Key,
} EncodeCallFlag;

typedef struct EncodeStackVars {
    // cache
    PyObject *key, *val;
    PyObject *cur_obj;           // = in_obj;
    Py_ssize_t cur_pos;          // = 0;
    Py_ssize_t cur_nested_depth; // = 0;
    Py_ssize_t cur_list_size;
    // alias thread local buffer
    EncodeCtnWithIndex *ctn_stack; // = obj_viewer->ctn_stack;
    UnicodeInfo unicode_info;
    bool cur_is_tuple;
} EncodeStackVars;

force_inline bool init_stack_vars(EncodeStackVars *stack_vars, PyObject *in_obj) {
    stack_vars->cur_obj = in_obj;
    stack_vars->cur_pos = 0;
    stack_vars->cur_nested_depth = 0;
    stack_vars->ctn_stack = get_encode_obj_stack_buffer();
    if (unlikely(!stack_vars->ctn_stack)) {
        PyErr_NoMemory();
        return false;
    }
    memset(&stack_vars->unicode_info, 0, sizeof(UnicodeInfo));
    return true;
}

force_inline bool init_unicode_buffer(EncodeUnicodeBufferInfo *unicode_buffer_info) {
    unicode_buffer_info->head = PyObject_Malloc(PYYJSON_ENCODE_DST_BUFFER_INIT_SIZE);
    if (likely(unicode_buffer_info->head)) {
#ifndef NDEBUG
        memset(unicode_buffer_info->head, 0, PYYJSON_ENCODE_DST_BUFFER_INIT_SIZE);
#endif
        unicode_buffer_info->writer.writer_void = PYYJSON_CAST(PyASCIIObject *, unicode_buffer_info->head) + 1;
        unicode_buffer_info->end = PYYJSON_CAST(u8 *, unicode_buffer_info->head) + PYYJSON_ENCODE_DST_BUFFER_INIT_SIZE;
    } else {
        PyErr_NoMemory();
        return false;
    }
    return true;
}


typedef struct {
    u8 *writer;
    void *head;
    void *end;
} EncodeUTF8BufferInfo;

typedef struct {
    // cache
    PyObject *key, *val;
    PyObject *cur_obj;           // = in_obj;
    Py_ssize_t cur_pos;          // = 0;
    Py_ssize_t cur_nested_depth; // = 0;
    Py_ssize_t cur_list_size;
    // alias thread local buffer
    EncodeCtnWithIndex *ctn_stack;
    bool cur_is_tuple;
} EncodeUTF8StackVars;

force_inline bool init_utf8_buffer(EncodeUTF8BufferInfo *utf8_buffer_info) {
    utf8_buffer_info->head = PyObject_Malloc(PYYJSON_ENCODE_DST_BUFFER_INIT_SIZE);
    if (likely(utf8_buffer_info->head)) {
#ifndef NDEBUG
        memset(utf8_buffer_info->head, 0, PYYJSON_ENCODE_DST_BUFFER_INIT_SIZE);
#endif
        const usize offset = PYBYTES_START_OFFSET;
        utf8_buffer_info->writer = PYYJSON_CAST(u8 *, utf8_buffer_info->head) + offset;
        utf8_buffer_info->end = PYYJSON_CAST(u8 *, utf8_buffer_info->head) + PYYJSON_ENCODE_DST_BUFFER_INIT_SIZE;
    } else {
        PyErr_NoMemory();
        return false;
    }
    return true;
}

force_inline bool init_utf8_stack_vars(EncodeUTF8StackVars *stack_vars, PyObject *in_obj) {
    stack_vars->cur_obj = in_obj;
    stack_vars->cur_pos = 0;
    stack_vars->cur_nested_depth = 0;
    stack_vars->ctn_stack = get_encode_obj_stack_buffer();
    if (unlikely(!stack_vars->ctn_stack)) {
        PyErr_NoMemory();
        return false;
    }
    return true;
}

force_inline void init_pybytes(PyObject *in_new_bytes, usize final_len) {
    PyBytesObject *new_bytes = PYYJSON_CAST(PyBytesObject *, in_new_bytes);
    PyObject_Init(in_new_bytes, &PyBytes_Type);
#if PY_MINOR_VERSION < 11
    new_bytes->ob_shash = -1;
#endif
    new_bytes->ob_sval[final_len] = 0;
}

force_inline bool bytes_buffer_reserve(EncodeUTF8BufferInfo *utf8_buffer_info, Py_ssize_t target_size) {
    return unicode_buffer_reserve_u8(PYYJSON_CAST(EncodeUnicodeBufferInfo *, utf8_buffer_info), target_size);
}

/* 
 * Some utility functions only related to *write*, like unicode buffer reserve, writing number
 * need macro: COMPILE_WRITE_UCS_LEVEL, value: 1, 2, or 4.
 */
#include "encode_utils_impl_wrap.h"

/* 
 * Some utility functions related to SIMD, like getting escape mask,
 * elevating ucs level, read/write simd vars.
 * need macro:
 *      COMPILE_READ_UCS_LEVEL, value: 1, 2, or 4.
 *      COMPILE_WRITE_UCS_LEVEL, value: 1, 2, or 4.
 */
// #include "encode_simd_utils_wrap.h"

/* 
 * Some functions for writing the unicode buffer, like writing key, writing value str.
 * need macro:
 *      COMPILE_READ_UCS_LEVEL, value: 1, 2, or 4.
 *      COMPILE_WRITE_UCS_LEVEL, value: 1, 2, or 4.
 */
#include "encode_unicode_impl_wrap.h"

/* 
 * Top-level encode functions for encoding container types: dict, list and tuple.
 * need macro:
 *      COMPILE_UCS_LEVEL, value: 0, 1, 2, or 4. COMPILE_UCS_LEVEL is the current writing level.
 *          This differs from COMPILE_WRITE_UCS_LEVEL: `0` stands for ascii. Since we always start from
 *          writing ascii, `0` also defines the entrance of encoding containers. See `PYYJSON_DUMPS_OBJ`
 *          for more details.
 *      COMPILE_INDENT_LEVEL, value: 0, 2, or 4.
 */
#include "encode_impl_wrap.h"

#include "bytes/encode_utf8.h"

/* 
 * Top-level encode functions for encoding container types tp bytes.
 * need macro:
 *      COMPILE_INDENT_LEVEL, value: 0, 2, or 4.
 */
// #include "bytes/encode_bytes_impl_wrap.h"

/* Encodes non-container types. */
force_inline PyObject *pyyjson_dumps_single_unicode(PyObject *unicode) {
    EncodeUnicodeBufferInfo _unicode_buffer_info; //, new_unicode_buffer_info;
    _unicode_buffer_info.head = PyObject_Malloc(PYYJSON_ENCODE_DST_BUFFER_INIT_SIZE);
    RETURN_ON_UNLIKELY_ERR(!_unicode_buffer_info.head);
    //
    Py_ssize_t len = PyUnicode_GET_LENGTH(unicode);
    int unicode_kind = PyUnicode_KIND(unicode);
    bool is_ascii = PyUnicode_IS_ASCII(unicode);
    //
    Py_ssize_t offset;
    if (is_ascii) {
        offset = sizeof(PyASCIIObject);
    } else {
        offset = sizeof(PyCompactUnicodeObject);
    }
    U8_WRITER(&_unicode_buffer_info) = PYYJSON_CAST(u8 *, _unicode_buffer_info.head) + offset;
    _unicode_buffer_info.end = PYYJSON_CAST(u8 *, _unicode_buffer_info.head) + PYYJSON_ENCODE_DST_BUFFER_INIT_SIZE;
    //
    bool success;
    switch (unicode_kind) {
        // pass `is_in_obj = true` to avoid unwanted indent check
        case 1: {
            success = unicode_buffer_append_str_internal_0_1_1(unicode, len, &_unicode_buffer_info, true, 0);
            _unicode_buffer_info.writer.writer_u8--;
            break;
        }
        case 2: {
            success = unicode_buffer_append_str_internal_0_2_2(unicode, len, &_unicode_buffer_info, true, 0);
            _unicode_buffer_info.writer.writer_u16--;
            break;
        }
        case 4: {
            success = unicode_buffer_append_str_internal_0_4_4(unicode, len, &_unicode_buffer_info, true, 0);
            _unicode_buffer_info.writer.writer_u32--;
            break;
        }
        default: {
            Py_UNREACHABLE();
            assert(false);
        }
    }
    if (unlikely(!success)) {
        // realloc failed when encoding, the original buffer is still valid
        PyObject_Free(_unicode_buffer_info.head);
        return NULL;
    }
    Py_ssize_t written_len = (uintptr_t)_unicode_buffer_info.writer.writer_u8 - (uintptr_t)_unicode_buffer_info.head - offset;
    written_len /= unicode_kind;
    assert(written_len >= 2);
    if (unlikely(!resize_to_fit_pyunicode(&_unicode_buffer_info, written_len, is_ascii ? 0 : unicode_kind))) {
        PyObject_Free(_unicode_buffer_info.head);
        return NULL;
    }
    init_pyunicode(_unicode_buffer_info.head, written_len, is_ascii ? 0 : unicode_kind);
    return (PyObject *)_unicode_buffer_info.head;
}

force_inline PyObject *pyyjson_dumps_single_long(PyObject *val) {
    PyObject *ret;
    if (pylong_is_zero(val)) {
        ret = PyUnicode_New(1, 127);
        RETURN_ON_UNLIKELY_ERR(!ret);
        u8 *writer = (u8 *)(((PyASCIIObject *)ret) + 1);
        writer[0] = '0';
        writer[1] = 0;
    } else {
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
        u8 buffer[64];
        if (sign) *buffer = '-';
        u8 *buffer_end = write_u64(v, buffer + sign);
        ret = PyUnicode_New(buffer_end - buffer, 127);
        RETURN_ON_UNLIKELY_ERR(!ret);
        u8 *writer = (u8 *)(((PyASCIIObject *)ret) + 1);
        pyyjson_memcpy(writer, buffer, buffer_end - buffer);
        writer[buffer_end - buffer] = 0;
    }
    return ret;
}

force_inline PyObject *pyyjson_dumps_single_float(PyObject *val) {
    u8 buffer[32];
    double v = PyFloat_AS_DOUBLE(val);
    u64 *raw = (u64 *)&v;
    size_t size = d2s_buffered_n(f64_from_raw(*raw), (char *)buffer);
    u8 *buffer_end = buffer + size;
    PyObject *unicode = PyUnicode_New(size, 127);
    if (unlikely(!unicode)) return NULL;
    // assert(unicode);
    char *write_pos = (char *)(((PyASCIIObject *)unicode) + 1);
    pyyjson_memcpy((void *)write_pos, buffer, size);
    write_pos[size] = 0;
    return unicode;
}

force_inline PyObject *pyyjson_dumps_single_constant(PyFastTypes py_type) {
    PyObject *ret;
    switch (py_type) {
        case T_True: {
            ret = PyUnicode_New(4, 127);
            RETURN_ON_UNLIKELY_ERR(!ret);
            u8 *writer = (u8 *)(((PyASCIIObject *)ret) + 1);
            strcpy((char *)writer, "true");
            break;
        }
        case T_False: {
            ret = PyUnicode_New(5, 127);
            RETURN_ON_UNLIKELY_ERR(!ret);
            u8 *writer = (u8 *)(((PyASCIIObject *)ret) + 1);
            strcpy((char *)writer, "false");
            break;
        }
        case T_None: {
            ret = PyUnicode_New(4, 127);
            RETURN_ON_UNLIKELY_ERR(!ret);
            u8 *writer = (u8 *)(((PyASCIIObject *)ret) + 1);
            strcpy((char *)writer, "null");
            break;
        }
        default: {
            ret = NULL;
            break;
        }
    }
    return ret;
}

static int invalid_arg_checked = 0;

/* Entrance for python code. */
PyObject *SIMD_NAME_MODIFIER(pyyjson_Encode)(PyObject *self, PyObject *args, PyObject *kwargs) {
    PyObject *obj;
    PyObject *ret;
    //
    PyObject *indent = NULL, *skipkeys = NULL, *ensure_ascii = NULL, *check_circular = NULL, *allow_nan = NULL, *cls = NULL, *separators = NULL, *default_ = NULL, *sort_keys = NULL;
    static const char *kwlist[] = {"obj", "indent", "skipkeys", "ensure_ascii", "check_circular", "allow_nan", "cls", "separators", "default", "sort_keys", NULL};
    //
    int indent_int = 0;
    //
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O|OOOOOOOOO", (char **)kwlist, &obj, &indent, &skipkeys, &ensure_ascii, &check_circular, &allow_nan, &cls, &separators, &default_, &sort_keys)) {
        goto fail;
    }

    if (!invalid_arg_checked && (skipkeys || ensure_ascii || check_circular || allow_nan || cls || separators || default_ || sort_keys)) {
        fprintf(stderr, "Warning: some options are not supported in this version of pyyjson\n");
        invalid_arg_checked = 1;
    }

    if (indent) {
        if (indent != Py_None && !PyLong_Check(indent)) {
            PyErr_SetString(PyExc_TypeError, "indent must be an integer");
            goto fail;
        }
        if (indent != Py_None) {
            int _indent = PyLong_AsLong(indent);
            if (_indent < 0 || _indent > 4 || (_indent / 2) * 2 != _indent) {
                PyErr_SetString(PyExc_ValueError, "indent must be 0, 2, or 4");
                goto fail;
            }
            indent_int = _indent;
        }
    }

    assert(obj);

    PyFastTypes fast_type = fast_type_check(obj);

    switch (fast_type) {
        case T_List:
        case T_Dict:
        case T_Tuple: {
            goto dumps_container;
        }
        case T_Unicode: {
            goto dumps_unicode;
        }
        case T_Long: {
            goto dumps_long;
        }
        case T_False:
        case T_True:
        case T_None: {
            goto dumps_constant;
        }
        case T_Float: {
            goto dumps_float;
        }
        default: {
            PyErr_SetString(JSONEncodeError, "Unsupported type to encode");
            goto fail;
        }
    }

dumps_container:;

    switch (indent_int) {
        case 0: {
            ret = pyyjson_dumps_obj_0_0(obj);
            break;
        }
        case 2: {
            ret = pyyjson_dumps_obj_2_0(obj);
            break;
        }
        case 4: {
            ret = pyyjson_dumps_obj_4_0(obj);
            break;
        }
        default: {
            Py_UNREACHABLE();
            assert(false);
        }
    }

    if (unlikely(!ret)) {
        if (!PyErr_Occurred()) {
            PyErr_SetString(JSONEncodeError, "Failed to decode JSON: unknown error");
        }
    }

    assert(!ret || ret->ob_refcnt == 1);

    goto success;

dumps_unicode:;
    return pyyjson_dumps_single_unicode(obj);
dumps_long:;
    return pyyjson_dumps_single_long(obj);
dumps_constant:;
    return pyyjson_dumps_single_constant(fast_type);
dumps_float:;
    return pyyjson_dumps_single_float(obj);
success:;
    return ret;
fail:;
    return NULL;
}
