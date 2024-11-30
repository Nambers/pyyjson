#include "encode_shared.h"
#include "simd/cvt.h"
#include "simd/simd_impl.h"
#include "simd_detect.h"
#include "unicode/uvector.h"
#include <threads.h>


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


typedef enum PyFastTypes {
    T_Unicode,
    T_Long,
    T_False,
    T_True,
    T_None,
    T_Float,
    T_List,
    T_Dict,
    T_Tuple,
    Unknown,
} PyFastTypes;


typedef struct UnicodeInfo {
    Py_ssize_t ascii_size;
    Py_ssize_t u8_size;
    Py_ssize_t u16_size;
    Py_ssize_t u32_size;
    int cur_ucs_type;
} UnicodeInfo;

typedef struct CtnType {
    PyObject *ctn;
    Py_ssize_t index;
} CtnType;


typedef struct StackVars {
    // cache
    UnicodeVector *vec;
    PyObject *key, *val;
    PyObject *cur_obj;           // = in_obj;
    Py_ssize_t cur_pos;          // = 0;
    Py_ssize_t cur_nested_depth; //= 0;
    Py_ssize_t cur_list_size;
    // alias thread local buffer
    CtnType *ctn_stack; //= obj_viewer->ctn_stack;
    UnicodeInfo unicode_info;
} StackVars;


#define GET_VEC(stack_vars) ((stack_vars)->vec)


force_inline void memorize_ascii_to_ucs4(UnicodeVector *vec, UnicodeInfo *unicode_info) {
    Py_ssize_t len = vec->head.write_u8 - (u8 *) GET_VEC_ASCII_START(vec);
    unicode_info->ascii_size = len;
    u32 *new_write_ptr = ((u32 *) GET_VEC_COMPACT_START(vec)) + len;
    vec->head.write_u32 = new_write_ptr;
    assert(unicode_info->cur_ucs_type == 0);
    unicode_info->cur_ucs_type = 4;
}

force_inline void memorize_ascii_to_ucs2(UnicodeVector *vec, UnicodeInfo *unicode_info) {
    Py_ssize_t len = vec->head.write_u8 - (u8 *) GET_VEC_ASCII_START(vec);
    unicode_info->ascii_size = len;
    u16 *new_write_ptr = ((u16 *) GET_VEC_COMPACT_START(vec)) + len;
    vec->head.write_u16 = new_write_ptr;
    assert(unicode_info->cur_ucs_type == 0);
    unicode_info->cur_ucs_type = 2;
}

force_inline void memorize_ascii_to_ucs1(UnicodeVector *vec, UnicodeInfo *unicode_info) {
    Py_ssize_t len = vec->head.write_u8 - (u8 *) GET_VEC_ASCII_START(vec);
    unicode_info->ascii_size = len;
    u8 *new_write_ptr = ((u8 *) GET_VEC_COMPACT_START(vec)) + len;
    vec->head.write_u8 = new_write_ptr;
    assert(unicode_info->cur_ucs_type == 0);
    unicode_info->cur_ucs_type = 1;
}

force_inline void memorize_ucs1_to_ucs2(UnicodeVector *vec, UnicodeInfo *unicode_info) {
    Py_ssize_t diff = vec->head.write_u8 - (u8 *) GET_VEC_COMPACT_START(vec);
    Py_ssize_t len = diff - unicode_info->ascii_size;
    assert(len >= 0);
    unicode_info->u8_size = len;
    u16 *new_write_ptr = ((u16 *) GET_VEC_COMPACT_START(vec)) + diff;
    vec->head.write_u16 = new_write_ptr;
    assert(unicode_info->cur_ucs_type == 1);
    unicode_info->cur_ucs_type = 2;
}

force_inline void memorize_ucs1_to_ucs4(UnicodeVector *vec, UnicodeInfo *unicode_info) {
    Py_ssize_t diff = vec->head.write_u8 - (u8 *) GET_VEC_COMPACT_START(vec);
    Py_ssize_t len = diff - unicode_info->ascii_size;
    assert(len >= 0);
    unicode_info->u8_size = len;
    u32 *new_write_ptr = ((u32 *) GET_VEC_COMPACT_START(vec)) + diff;
    vec->head.write_u32 = new_write_ptr;
    assert(unicode_info->cur_ucs_type == 1);
    unicode_info->cur_ucs_type = 4;
}

force_inline void memorize_ucs2_to_ucs4(UnicodeVector *vec, UnicodeInfo *unicode_info) {
    Py_ssize_t diff = vec->head.write_u16 - (u16 *) GET_VEC_COMPACT_START(vec);
    Py_ssize_t len = diff - unicode_info->ascii_size - unicode_info->u8_size;
    assert(len >= 0);
    unicode_info->u16_size = len;
    u32 *new_write_ptr = ((u32 *) GET_VEC_COMPACT_START(vec)) + diff;
    vec->head.write_u32 = new_write_ptr;
    assert(unicode_info->cur_ucs_type == 2);
    unicode_info->cur_ucs_type = 4;
}


force_inline void ascii_elevate2(UnicodeVector *vec, UnicodeInfo *unicode_info) {
    u8 *start = ((u8 *) GET_VEC_ASCII_START(vec));
    u16 *write_start = ((u16 *) GET_VEC_COMPACT_START(vec));
    long_back_elevate_1_2(write_start, start, unicode_info->ascii_size);
}

force_inline void ascii_elevate4(UnicodeVector *vec, UnicodeInfo *unicode_info) {
    u8 *start = ((u8 *) GET_VEC_ASCII_START(vec));
    u32 *write_start = ((u32 *) GET_VEC_COMPACT_START(vec));
    long_back_elevate_1_4(write_start, start, unicode_info->ascii_size);
}

force_inline void ucs1_elevate2(UnicodeVector *vec, UnicodeInfo *unicode_info) {
    Py_ssize_t offset = unicode_info->ascii_size;
    u8 *start = ((u8 *) GET_VEC_COMPACT_START(vec)) + offset;
    u16 *write_start = ((u16 *) GET_VEC_COMPACT_START(vec)) + offset;
    long_back_elevate_1_2(write_start, start, unicode_info->u8_size);
}

force_inline void ucs1_elevate4(UnicodeVector *vec, UnicodeInfo *unicode_info) {
    Py_ssize_t offset = unicode_info->ascii_size;
    u8 *start = ((u8 *) GET_VEC_COMPACT_START(vec)) + offset;
    u32 *write_start = ((u32 *) GET_VEC_COMPACT_START(vec)) + offset;
    long_back_elevate_1_4(write_start, start, unicode_info->u8_size);
}

force_inline void ucs2_elevate4(UnicodeVector *vec, UnicodeInfo *unicode_info) {
    Py_ssize_t offset = unicode_info->ascii_size + unicode_info->u8_size;
    u16 *start = ((u16 *) GET_VEC_COMPACT_START(vec)) + offset;
    u32 *write_start = ((u32 *) GET_VEC_COMPACT_START(vec)) + offset;
    long_back_elevate_2_4(write_start, start, unicode_info->u16_size);
}

force_inline void ascii_elevate1(UnicodeVector *vec, UnicodeInfo *unicode_info) {
    memmove(GET_VEC_COMPACT_START(vec), GET_VEC_ASCII_START(vec), unicode_info->ascii_size);
}

thread_local CtnType __tls_ctn_stack[PYYJSON_ENCODE_MAX_RECURSION];

force_inline bool init_stack_vars(StackVars *stack_vars, PyObject *in_obj) {
    stack_vars->cur_obj = in_obj;
    stack_vars->cur_pos = 0;
    stack_vars->cur_nested_depth = 0;
    stack_vars->ctn_stack = __tls_ctn_stack;
    if (unlikely(!stack_vars->ctn_stack)) {
        stack_vars->vec = NULL; // avoid freeing a wild pointer
        PyErr_NoMemory();
        return false;
    }
    memset(&stack_vars->unicode_info, 0, sizeof(UnicodeInfo));
    GET_VEC(stack_vars) = PyObject_Malloc(PYYJSON_ENCODE_DST_BUFFER_INIT_SIZE);
#ifndef NDEBUG
    memset(GET_VEC(stack_vars), 0, PYYJSON_ENCODE_DST_BUFFER_INIT_SIZE);
#endif
    if (likely(GET_VEC(stack_vars))) {
        GET_VEC(stack_vars)->head.write_u8 = (u8 *) (((PyASCIIObject *) GET_VEC(stack_vars)) + 1);
        GET_VEC(stack_vars)->head.write_end = (void *) ((u8 *) (GET_VEC(stack_vars)) + PYYJSON_ENCODE_DST_BUFFER_INIT_SIZE);
        return true;
    }
    PyErr_NoMemory();
    return false;
}

force_inline bool vector_resize_to_fit(UnicodeVector **restrict vec_addr, Py_ssize_t len, int ucs_type) {
    Py_ssize_t char_size = ucs_type ? ucs_type : 1;
    Py_ssize_t struct_size = ucs_type ? sizeof(PyCompactUnicodeObject) : sizeof(PyASCIIObject);
    assert(len <= ((PY_SSIZE_T_MAX - struct_size) / char_size - 1));
    // Resizes to a smaller size. It *should* always success
    UnicodeVector *new_vec = (UnicodeVector *) PyObject_Realloc(*vec_addr, struct_size + (len + 1) * char_size);
    if (likely(new_vec)) {
        *vec_addr = new_vec;
        return true;
    }
    return false;
}

// _PyUnicode_CheckConsistency is hidden in Python 3.13
#if PY_MINOR_VERSION >= 13
extern int _PyUnicode_CheckConsistency(PyObject *op, int check_content);
#endif

force_inline void init_py_unicode(UnicodeVector **restrict vec_addr, Py_ssize_t size, int kind) {
    UnicodeVector *vec = *vec_addr;
    PyCompactUnicodeObject *unicode = &vec->unicode_rep.compact_obj;
    PyASCIIObject *ascii = &vec->unicode_rep.ascii_rep.ascii_obj;
    PyObject_Init((PyObject *) unicode, &PyUnicode_Type);
    void *data = kind ? GET_VEC_COMPACT_START(vec) : GET_VEC_ASCII_START(vec);
    //
    ascii->length = size;
    ascii->hash = -1;
    ascii->state.interned = 0;
    ascii->state.kind = kind ? kind : 1;
    ascii->state.compact = 1;
    ascii->state.ascii = kind ? 0 : 1;

#if PY_MINOR_VERSION >= 12
    // statically_allocated appears in 3.12
    ascii->state.statically_allocated = 0;
#else
    bool is_sharing = false;
    // ready is dropped in 3.12
    ascii->state.ready = 1;
#endif

    if (kind <= 1) {
        ((u8 *) data)[size] = 0;
    } else if (kind == 2) {
        ((u16 *) data)[size] = 0;
#if PY_MINOR_VERSION < 12
        is_sharing = sizeof(wchar_t) == 2;
#endif
    } else if (kind == 4) {
        ((u32 *) data)[size] = 0;
#if PY_MINOR_VERSION < 12
        is_sharing = sizeof(wchar_t) == 4;
#endif
    } else {
        assert(false);
        Py_UNREACHABLE();
    }
    if (kind) {
        unicode->utf8 = NULL;
        unicode->utf8_length = 0;
    }
#if PY_MINOR_VERSION < 12
    if (kind > 1) {
        if (is_sharing) {
            unicode->wstr_length = size;
            ascii->wstr = (wchar_t *) data;
        } else {
            unicode->wstr_length = 0;
            ascii->wstr = NULL;
        }
    } else {
        ascii->wstr = NULL;
        if (kind) unicode->wstr_length = 0;
    }
#endif
    assert(_PyUnicode_CheckConsistency((PyObject *) unicode, 0));
    assert(ascii->ob_base.ob_refcnt == 1);
}

#define _CTN_STACK(stack_vars) ((void) 0, stack_vars->ctn_stack)

#if PY_MINOR_VERSION >= 13
// _PyNone_Type is hidden in Python 3.13
static PyTypeObject *PyNone_Type = NULL;
void _init_PyNone_Type(PyTypeObject *none_type) {
    PyNone_Type = none_type;
}
#else
#define PyNone_Type &_PyNone_Type
#endif

force_inline PyFastTypes fast_type_check(PyObject *val) {
    PyTypeObject *type = Py_TYPE(val);
    if (type == &PyUnicode_Type) {
        return T_Unicode;
    } else if (type == &PyLong_Type) {
        return T_Long;
    } else if (type == &PyBool_Type) {
        if (val == Py_False) {
            return T_False;
        } else {
            assert(val == Py_True);
            return T_True;
        }
    } else if (type == PyNone_Type) {
        return T_None;
    } else if (type == &PyFloat_Type) {
        return T_Float;
    } else if (type == &PyList_Type) {
        return T_List;
    } else if (type == &PyDict_Type) {
        return T_Dict;
    } else if (type == &PyTuple_Type) {
        return T_Tuple;
    } else {
        return Unknown;
    }
}

#define RETURN_ON_UNLIKELY_ERR(x) \
    do {                          \
        if (unlikely((x))) {      \
            return false;         \
        }                         \
    } while (0)

#define TAIL_PADDING (512 / 8)

force_inline Py_ssize_t get_indent_char_count(Py_ssize_t cur_nested_depth, Py_ssize_t indent_level) {
    return indent_level ? (indent_level * cur_nested_depth + 1) : 0;
}


// /*
//  * Some unicode vector utility functions only related to write
//  * need macro: COMPILE_WRITE_UCS_LEVEL, value: 1, 2, or 4.
//  */
// #include "unicode/_include_helper/w_wrap.inl.h"

// /*
//  * Some unicode vector utility functions related to write and indent
//  * need macro: COMPILE_WRITE_UCS_LEVEL, value: 1, 2, or 4.
//  * need macro: COMPILE_INDENT_LEVEL, value: 0, 2, or 4.
//  */
// #include "unicode/_include_helper/w_i_wrap.inl.h"
#include "unicode/_include_helper/indent_wrap.h"
#include "unicode/_include_helper/reserve_wrap.h"

/* 
 * Some utility functions only related to *write*, like vector reserve, writing number
 * need macro: COMPILE_WRITE_UCS_LEVEL, value: 1, 2, or 4.
 */
#include "encode_utils_impl_wrap.inl.h"

/* 
 * Some utility functions related to SIMD, like getting escape mask,
 * elevating ucs level, read/write simd vars.
 * need macro:
 *      COMPILE_READ_UCS_LEVEL, value: 1, 2, or 4.
 *      COMPILE_WRITE_UCS_LEVEL, value: 1, 2, or 4.
 */
#include "encode_simd_utils_wrap.inl.h"

/* 
 * Some functions for writing the unicode vector, like writing key, writing value str.
 * need macro:
 *      COMPILE_READ_UCS_LEVEL, value: 1, 2, or 4.
 *      COMPILE_WRITE_UCS_LEVEL, value: 1, 2, or 4.
 */
#include "encode_unicode_impl_wrap.inl.h"

/* 
 * Top-level encode functions, like encode list, encode dict, encode single unicode.
 * need macro:
 *      COMPILE_UCS_LEVEL, value: 0, 1, 2, or 4. COMPILE_UCS_LEVEL is the current writing level.
 *          This differs from COMPILE_WRITE_UCS_LEVEL: `0` stands for ascii. Since we always start from
 *          writing ascii, `0` also defines the entrance of encoding. See `PYYJSON_DUMPS_OBJ` for more
 *          details.
 *      COMPILE_INDENT_LEVEL, value: 0, 2, or 4.
 */
#include "encode_impl_wrap.inl.h"

/* Encodes non-container types. */
force_inline PyObject *pyyjson_dumps_single_unicode(PyObject *unicode) {
    UnicodeVector *vec = PyObject_Malloc(PYYJSON_ENCODE_DST_BUFFER_INIT_SIZE);
    RETURN_ON_UNLIKELY_ERR(!vec);
    Py_ssize_t len = PyUnicode_GET_LENGTH(unicode);
    int unicode_kind = PyUnicode_KIND(unicode);
    bool is_ascii = PyUnicode_IS_ASCII(unicode);
    if (is_ascii) {
        U8_WRITER(vec) = (u8 *) (((PyASCIIObject *) vec) + 1);
    } else {
        U8_WRITER(vec) = (u8 *) (((PyCompactUnicodeObject *) vec) + 1);
    }
    void *write_start = (void *) U8_WRITER(vec);
    vec->head.write_end = (void *) (((u8 *) vec) + PYYJSON_ENCODE_DST_BUFFER_INIT_SIZE);
    bool success;
    switch (unicode_kind) {
        // pass `is_in_obj = true` to avoid unwanted indent check
        case 1: {
            success = vec_write_str_0_1_1(unicode, len, &vec, true, 0);
            if (success) vec_back1_1(vec);
            break;
        }
        case 2: {
            success = vec_write_str_0_2_2(unicode, len, &vec, true, 0);
            if (success) vec_back1_2(vec);
            break;
        }
        case 4: {
            success = vec_write_str_0_4_4(unicode, len, &vec, true, 0);
            if (success) vec_back1_4(vec);
            break;
        }
        default: {
            Py_UNREACHABLE();
            assert(false);
        }
    }
    if (unlikely(!success)) {
        PyObject_Free(vec);
        return NULL;
    }
    Py_ssize_t written_len = (Py_ssize_t) U8_WRITER(vec) - (Py_ssize_t) write_start;
    written_len /= unicode_kind;
    assert(written_len >= 2);
    success = vector_resize_to_fit(&vec, written_len, is_ascii ? 0 : unicode_kind);
    if (unlikely(!success)) {
        PyObject_Free(vec);
        return NULL;
    }
    init_py_unicode(&vec, written_len, is_ascii ? 0 : unicode_kind);
    return (PyObject *) vec;
}

force_inline PyObject *pyyjson_dumps_single_long(PyObject *val) {
    PyObject *ret;
    if (pylong_is_zero(val)) {
        ret = PyUnicode_New(1, 127);
        RETURN_ON_UNLIKELY_ERR(!ret);
        u8 *writer = (u8 *) (((PyASCIIObject *) ret) + 1);
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
        u8 *writer = (u8 *) (((PyASCIIObject *) ret) + 1);
        memcpy(writer, buffer, buffer_end - buffer);
        writer[buffer_end - buffer] = 0;
    }
    return ret;
}

force_inline PyObject *pyyjson_dumps_single_float(PyObject *val) {
    u8 buffer[32];
    double v = PyFloat_AS_DOUBLE(val);
    u64 *raw = (u64 *) &v;
    u8 *buffer_end = write_f64_raw(buffer, *raw);
    size_t size = buffer_end - buffer;
    PyObject *unicode = PyUnicode_New(size, 127);
    if (unlikely(!unicode)) return NULL;
    // assert(unicode);
    char *write_pos = (char *) (((PyASCIIObject *) unicode) + 1);
    memcpy((void *) write_pos, buffer, size);
    write_pos[size] = 0;
    return unicode;
}

force_inline PyObject *pyyjson_dumps_single_constant(PyFastTypes py_type) {
    PyObject *ret;
    switch (py_type) {
        case T_True: {
            ret = PyUnicode_New(4, 127);
            RETURN_ON_UNLIKELY_ERR(!ret);
            u8 *writer = (u8 *) (((PyASCIIObject *) ret) + 1);
            strcpy((char *) writer, "true");
            break;
        }
        case T_False: {
            ret = PyUnicode_New(5, 127);
            RETURN_ON_UNLIKELY_ERR(!ret);
            u8 *writer = (u8 *) (((PyASCIIObject *) ret) + 1);
            strcpy((char *) writer, "false");
            break;
        }
        case T_None: {
            ret = PyUnicode_New(4, 127);
            RETURN_ON_UNLIKELY_ERR(!ret);
            u8 *writer = (u8 *) (((PyASCIIObject *) ret) + 1);
            strcpy((char *) writer, "null");
            break;
        }
        default: {
            ret = NULL;
            break;
        }
    }
    return ret;
}

/* Entrance for python code. */
force_noinline PyObject *pyyjson_Encode(PyObject *self, PyObject *args, PyObject *kwargs) {
    PyObject *obj;
    int option_digit = 0;
    usize indent = 0;
    PyObject *ret;
    static const char *kwlist[] = {"obj", "options", NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O|i", (char **) kwlist, &obj, &option_digit)) {
        PyErr_SetString(PyExc_TypeError, "Invalid argument");
        goto fail;
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
    if (option_digit & 1) {
        if (option_digit & 2) {
            PyErr_SetString(PyExc_ValueError, "Cannot mix indent options");
            goto fail;
        }
        indent = 2;
    } else if (option_digit & 2) {
        indent = 4;
    }

    switch (indent) {
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
