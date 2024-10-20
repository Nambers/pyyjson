#include "cpy_utils.inl"
#include "encode_shared.h"
typedef struct UnicodeVecHead {
    union {
        u8 *write_u8;
        u16 *write_u16;
        u32 *write_u32;
    };
    void *write_end;
} UnicodeVecHead;

static_assert(sizeof(UnicodeVecHead) <= sizeof(PyASCIIObject), "sizeof(UnicodeVecHead) <= sizeof(PyASCIIObject)");


typedef struct VecASCIIRep {
    union {
        PyASCIIObject ascii_obj;
    };
    u8 ascii_data[1];
} VecASCIIRep;


typedef struct CompactUnicodeRep {
    union {
        VecASCIIRep ascii_rep;
        PyCompactUnicodeObject compact_obj;
    };
    union {
        u8 latin1_data[1];
        u16 u16_data[1];
        u32 u32_data[1];
    };
} CompactUnicodeRep;

typedef struct UnicodeVector {
    union {
        UnicodeVecHead head;
        CompactUnicodeRep unicode_rep;
    };
} UnicodeVector;

#define GET_VEC_ASCII_START(vec) ((void *) &(vec)->unicode_rep.ascii_rep.ascii_data[0])
#define GET_VEC_COMPACT_START(vec) ((void *) &(vec)->unicode_rep.latin1_data[0])

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

force_inline int get_cur_ucs_type(StackVars *stack_vars) {
    return stack_vars->unicode_info.cur_ucs_type;
}

force_inline void memorize_ascii_to_ucs4(StackVars *stack_vars) {
    UnicodeVector *vec = GET_VEC(stack_vars);
    Py_ssize_t len = vec->head.write_u8 - (u8 *) GET_VEC_ASCII_START(vec);
    stack_vars->unicode_info.ascii_size = len;
    u32 *new_write_ptr = ((u32 *) GET_VEC_COMPACT_START(vec)) + len;
    vec->head.write_u32 = new_write_ptr;
    assert(get_cur_ucs_type(stack_vars) == 0);
    stack_vars->unicode_info.cur_ucs_type = 4;
}

force_inline void memorize_ascii_to_ucs2(StackVars *stack_vars) {
    UnicodeVector *vec = GET_VEC(stack_vars);
    Py_ssize_t len = vec->head.write_u8 - (u8 *) GET_VEC_ASCII_START(vec);
    stack_vars->unicode_info.ascii_size = len;
    u16 *new_write_ptr = ((u16 *) GET_VEC_COMPACT_START(vec)) + len;
    vec->head.write_u16 = new_write_ptr;
    assert(get_cur_ucs_type(stack_vars) == 0);
    stack_vars->unicode_info.cur_ucs_type = 2;
}

force_inline void memorize_ascii_to_ucs1(StackVars *stack_vars) {
    UnicodeVector *vec = GET_VEC(stack_vars);
    Py_ssize_t len = vec->head.write_u8 - (u8 *) GET_VEC_ASCII_START(vec);
    stack_vars->unicode_info.ascii_size = len;
    u8 *new_write_ptr = ((u8 *) GET_VEC_COMPACT_START(vec)) + len;
    vec->head.write_u8 = new_write_ptr;
    assert(get_cur_ucs_type(stack_vars) == 0);
    stack_vars->unicode_info.cur_ucs_type = 1;
}

force_inline void memorize_ucs1_to_ucs2(StackVars *stack_vars) {
    UnicodeVector *vec = GET_VEC(stack_vars);
    Py_ssize_t diff = vec->head.write_u8 - (u8 *) GET_VEC_COMPACT_START(vec);
    Py_ssize_t len = diff - stack_vars->unicode_info.ascii_size;
    assert(len >= 0);
    stack_vars->unicode_info.u8_size = len;
    u16 *new_write_ptr = ((u16 *) GET_VEC_COMPACT_START(vec)) + diff;
    vec->head.write_u16 = new_write_ptr;
    assert(get_cur_ucs_type(stack_vars) == 1);
    stack_vars->unicode_info.cur_ucs_type = 2;
}

force_inline void memorize_ucs1_to_ucs4(StackVars *stack_vars) {
    UnicodeVector *vec = GET_VEC(stack_vars);
    Py_ssize_t diff = vec->head.write_u8 - (u8 *) GET_VEC_COMPACT_START(vec);
    Py_ssize_t len = diff - stack_vars->unicode_info.ascii_size;
    assert(len >= 0);
    stack_vars->unicode_info.u8_size = len;
    u32 *new_write_ptr = ((u32 *) GET_VEC_COMPACT_START(vec)) + diff;
    vec->head.write_u32 = new_write_ptr;
    assert(get_cur_ucs_type(stack_vars) == 1);
    stack_vars->unicode_info.cur_ucs_type = 4;
}

force_inline void memorize_ucs2_to_ucs4(StackVars *stack_vars) {
    UnicodeVector *vec = GET_VEC(stack_vars);
    Py_ssize_t diff = vec->head.write_u16 - (u16 *) GET_VEC_COMPACT_START(vec);
    Py_ssize_t len = diff - stack_vars->unicode_info.ascii_size - stack_vars->unicode_info.u8_size;
    assert(len >= 0);
    stack_vars->unicode_info.u16_size = len;
    u32 *new_write_ptr = ((u32 *) GET_VEC_COMPACT_START(vec)) + diff;
    vec->head.write_u32 = new_write_ptr;
    assert(get_cur_ucs_type(stack_vars) == 2);
    stack_vars->unicode_info.cur_ucs_type = 4;
}

force_inline void ascii_elevate2(StackVars *stack_vars) {
}

force_inline void ascii_elevate4(StackVars *stack_vars) {
}

force_inline void ucs1_elevate2(StackVars *stack_vars) {
}

force_inline void ucs1_elevate4(StackVars *stack_vars) {
}

force_inline void ucs2_elevate4(StackVars *stack_vars) {
}

force_inline void ascii_elevate1(StackVars *stack_vars) {
    memmove(GET_VEC_COMPACT_START(stack_vars->vec), GET_VEC_ASCII_START(stack_vars->vec), stack_vars->unicode_info.ascii_size);
}

force_inline bool init_stack_vars(StackVars *stack_vars, PyObject *in_obj) {
    stack_vars->cur_obj = in_obj;
    stack_vars->cur_pos = 0;
    stack_vars->cur_nested_depth = 0;
    stack_vars->ctn_stack = malloc(sizeof(CtnType) * 1024);
    if (unlikely(!stack_vars->ctn_stack)) {
        stack_vars->vec = NULL; // avoid freeing a wild pointer
        PyErr_NoMemory();
        return false;
    }
    memset(&stack_vars->unicode_info, 0, sizeof(UnicodeInfo));
    GET_VEC(stack_vars) = PyObject_Malloc(65535);
    if (likely(GET_VEC(stack_vars))) {
        GET_VEC(stack_vars)->head.write_u8 = (u8 *) (((PyASCIIObject *) GET_VEC(stack_vars)) + 1);
        GET_VEC(stack_vars)->head.write_end = (void *) ((u8 *) (GET_VEC(stack_vars)) + 65535);
        return true;
    }
    PyErr_NoMemory();
    return false;
}

force_inline bool vector_resize_to_fit(StackVars *stack_vars, Py_ssize_t len, int ucs_type) {
    assert(get_cur_ucs_type(stack_vars) == ucs_type);
    Py_ssize_t char_size = ucs_type ? ucs_type : 1;
    Py_ssize_t struct_size = ucs_type ? sizeof(PyCompactUnicodeObject) : sizeof(PyASCIIObject);
    assert(len <= ((PY_SSIZE_T_MAX - struct_size) / char_size - 1));
    stack_vars->vec = (UnicodeVector *) PyObject_Realloc(stack_vars->vec, struct_size + (len + 1) * char_size);
    return NULL != stack_vars->vec;
}

// _PyUnicode_CheckConsistency is hidden in Python 3.13
#if PY_MINOR_VERSION >= 13
extern int _PyUnicode_CheckConsistency(PyObject *op, int check_content);
#endif

force_inline void init_py_unicode(StackVars *stack_vars, Py_ssize_t size, int kind) {
    assert(kind == get_cur_ucs_type(stack_vars));
    UnicodeVector *vec = GET_VEC(stack_vars);
    PyCompactUnicodeObject *unicode = &vec->unicode_rep.compact_obj;
    PyASCIIObject *ascii = &vec->unicode_rep.ascii_rep.ascii_obj;
    PyObject_Init((PyObject *) unicode, &PyUnicode_Type);
    void *data = kind ? GET_VEC_COMPACT_START(stack_vars->vec) : GET_VEC_ASCII_START(stack_vars->vec);
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

#define U32_WRITER(vec) (vec->head.write_u32)
#define U16_WRITER(vec) (vec->head.write_u16)
#define U8_WRITER(vec) (vec->head.write_u8)

#define PYYJSON_CONCAT2_EX(a, b) a##_##b
#define PYYJSON_CONCAT2(a, b) PYYJSON_CONCAT2_EX(a, b)

#define PYYJSON_CONCAT3_EX(a, b, c) a##_##b##_##c
#define PYYJSON_CONCAT3(a, b, c) PYYJSON_CONCAT3_EX(a, b, c)

#define PYYJSON_CONCAT4_EX(a, b, c, d) a##_##b##_##c##_##d
#define PYYJSON_CONCAT4(a, b, c, d) PYYJSON_CONCAT4_EX(a, b, c, d)


#define VEC_END(vec) (vec->head.write_end)
#define VEC_MEM_U8_DIFF(vec, ptr) (Py_ssize_t) ptr - (Py_ssize_t) vec

#define TAIL_PADDING (512 / 8)

force_inline Py_ssize_t get_indent_char_count(StackVars *stack_vars, Py_ssize_t indent_level) {
    return indent_level ? (indent_level * stack_vars->cur_nested_depth + 1) : 0;
}

force_inline void vec_set_rwptr_and_size(UnicodeVector *vec, Py_ssize_t rw_diff, Py_ssize_t target_size) {
    vec->head.write_u8 = (u8 *) (((Py_ssize_t) vec) + rw_diff);
    vec->head.write_end = (void *) (((Py_ssize_t) vec) + target_size);
}

force_inline bool vec_in_boundary(UnicodeVector *vec) {
    return vec->head.write_u8 <= (u8 *) vec->head.write_end && vec->head.write_u8 >= (u8 *) GET_VEC_ASCII_START(vec);
}

#define COMPILE_WRITE_UCS_LEVEL 1
#include "encode_utils_impl.inl"
#undef COMPILE_WRITE_UCS_LEVEL

#define COMPILE_WRITE_UCS_LEVEL 2
#include "encode_utils_impl.inl"
#undef COMPILE_WRITE_UCS_LEVEL

#define COMPILE_WRITE_UCS_LEVEL 4
#include "encode_utils_impl.inl"
#undef COMPILE_WRITE_UCS_LEVEL

#define COMPILE_INDENT_LEVEL 0

#define COMPILE_READ_UCS_LEVEL 1
#define COMPILE_WRITE_UCS_LEVEL 1
#include "encode_unicode_impl.inl"
#undef COMPILE_WRITE_UCS_LEVEL
#undef COMPILE_READ_UCS_LEVEL

#define COMPILE_READ_UCS_LEVEL 1
#define COMPILE_WRITE_UCS_LEVEL 2
#include "encode_unicode_impl.inl"
#undef COMPILE_WRITE_UCS_LEVEL
#undef COMPILE_READ_UCS_LEVEL

#define COMPILE_READ_UCS_LEVEL 1
#define COMPILE_WRITE_UCS_LEVEL 4
#include "encode_unicode_impl.inl"
#undef COMPILE_WRITE_UCS_LEVEL
#undef COMPILE_READ_UCS_LEVEL

#define COMPILE_READ_UCS_LEVEL 2
#define COMPILE_WRITE_UCS_LEVEL 2
#include "encode_unicode_impl.inl"
#undef COMPILE_WRITE_UCS_LEVEL
#undef COMPILE_READ_UCS_LEVEL

#define COMPILE_READ_UCS_LEVEL 2
#define COMPILE_WRITE_UCS_LEVEL 4
#include "encode_unicode_impl.inl"
#undef COMPILE_WRITE_UCS_LEVEL
#undef COMPILE_READ_UCS_LEVEL

#define COMPILE_READ_UCS_LEVEL 4
#define COMPILE_WRITE_UCS_LEVEL 4
#include "encode_unicode_impl.inl"
#undef COMPILE_WRITE_UCS_LEVEL
#undef COMPILE_READ_UCS_LEVEL

#undef COMPILE_INDENT_LEVEL


#define COMPILE_INDENT_LEVEL 2

#define COMPILE_READ_UCS_LEVEL 1
#define COMPILE_WRITE_UCS_LEVEL 1
#include "encode_unicode_impl.inl"
#undef COMPILE_WRITE_UCS_LEVEL
#undef COMPILE_READ_UCS_LEVEL

#define COMPILE_READ_UCS_LEVEL 1
#define COMPILE_WRITE_UCS_LEVEL 2
#include "encode_unicode_impl.inl"
#undef COMPILE_WRITE_UCS_LEVEL
#undef COMPILE_READ_UCS_LEVEL

#define COMPILE_READ_UCS_LEVEL 1
#define COMPILE_WRITE_UCS_LEVEL 4
#include "encode_unicode_impl.inl"
#undef COMPILE_WRITE_UCS_LEVEL
#undef COMPILE_READ_UCS_LEVEL

#define COMPILE_READ_UCS_LEVEL 2
#define COMPILE_WRITE_UCS_LEVEL 2
#include "encode_unicode_impl.inl"
#undef COMPILE_WRITE_UCS_LEVEL
#undef COMPILE_READ_UCS_LEVEL

#define COMPILE_READ_UCS_LEVEL 2
#define COMPILE_WRITE_UCS_LEVEL 4
#include "encode_unicode_impl.inl"
#undef COMPILE_WRITE_UCS_LEVEL
#undef COMPILE_READ_UCS_LEVEL

#define COMPILE_READ_UCS_LEVEL 4
#define COMPILE_WRITE_UCS_LEVEL 4
#include "encode_unicode_impl.inl"
#undef COMPILE_WRITE_UCS_LEVEL
#undef COMPILE_READ_UCS_LEVEL

#undef COMPILE_INDENT_LEVEL

#define COMPILE_INDENT_LEVEL 4

#define COMPILE_READ_UCS_LEVEL 1
#define COMPILE_WRITE_UCS_LEVEL 1
#include "encode_unicode_impl.inl"
#undef COMPILE_WRITE_UCS_LEVEL
#undef COMPILE_READ_UCS_LEVEL

#define COMPILE_READ_UCS_LEVEL 1
#define COMPILE_WRITE_UCS_LEVEL 2
#include "encode_unicode_impl.inl"
#undef COMPILE_WRITE_UCS_LEVEL
#undef COMPILE_READ_UCS_LEVEL

#define COMPILE_READ_UCS_LEVEL 1
#define COMPILE_WRITE_UCS_LEVEL 4
#include "encode_unicode_impl.inl"
#undef COMPILE_WRITE_UCS_LEVEL
#undef COMPILE_READ_UCS_LEVEL

#define COMPILE_READ_UCS_LEVEL 2
#define COMPILE_WRITE_UCS_LEVEL 2
#include "encode_unicode_impl.inl"
#undef COMPILE_WRITE_UCS_LEVEL
#undef COMPILE_READ_UCS_LEVEL

#define COMPILE_READ_UCS_LEVEL 2
#define COMPILE_WRITE_UCS_LEVEL 4
#include "encode_unicode_impl.inl"
#undef COMPILE_WRITE_UCS_LEVEL
#undef COMPILE_READ_UCS_LEVEL

#define COMPILE_READ_UCS_LEVEL 4
#define COMPILE_WRITE_UCS_LEVEL 4
#include "encode_unicode_impl.inl"
#undef COMPILE_WRITE_UCS_LEVEL
#undef COMPILE_READ_UCS_LEVEL

#undef COMPILE_INDENT_LEVEL

/* -------- */
#define COMPILE_INDENT_LEVEL 0

#define COMPILE_UCS_LEVEL 4
#include "encode5_impl.inl"
#undef COMPILE_UCS_LEVEL

#define COMPILE_UCS_LEVEL 2
#include "encode5_impl.inl"
#undef COMPILE_UCS_LEVEL

#define COMPILE_UCS_LEVEL 1
#include "encode5_impl.inl"
#undef COMPILE_UCS_LEVEL

#define COMPILE_UCS_LEVEL 0
#include "encode5_impl.inl"
#undef COMPILE_UCS_LEVEL

#undef COMPILE_INDENT_LEVEL

#define COMPILE_INDENT_LEVEL 2

#define COMPILE_UCS_LEVEL 4
#include "encode5_impl.inl"
#undef COMPILE_UCS_LEVEL

#define COMPILE_UCS_LEVEL 2
#include "encode5_impl.inl"
#undef COMPILE_UCS_LEVEL

#define COMPILE_UCS_LEVEL 1
#include "encode5_impl.inl"
#undef COMPILE_UCS_LEVEL

#define COMPILE_UCS_LEVEL 0
#include "encode5_impl.inl"
#undef COMPILE_UCS_LEVEL

#undef COMPILE_INDENT_LEVEL

#define COMPILE_INDENT_LEVEL 4

#define COMPILE_UCS_LEVEL 4
#include "encode5_impl.inl"
#undef COMPILE_UCS_LEVEL

#define COMPILE_UCS_LEVEL 2
#include "encode5_impl.inl"
#undef COMPILE_UCS_LEVEL

#define COMPILE_UCS_LEVEL 1
#include "encode5_impl.inl"
#undef COMPILE_UCS_LEVEL

#define COMPILE_UCS_LEVEL 0
#include "encode5_impl.inl"
#undef COMPILE_UCS_LEVEL

#undef COMPILE_INDENT_LEVEL


force_noinline PyObject *pyyjson_Encode(PyObject *self, PyObject *args, PyObject *kwargs) {
    PyObject *obj;
    int option_digit = 0;
    static const char *kwlist[] = {"obj", "options", NULL};
    // DumpOption options = {IndentLevel::NONE};
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O|i", (char **) kwlist, &obj, &option_digit)) {
        PyErr_SetString(PyExc_TypeError, "Invalid argument");
        return NULL;
    }
    if (option_digit & 1) {
        if (option_digit & 2) {
            PyErr_SetString(PyExc_ValueError, "Cannot mix indent options");
            return NULL;
        }
        // options.indent_level = IndentLevel::INDENT_2;
    } else if (option_digit & 2) {
        // options.indent_level = IndentLevel::INDENT_4;
    }

    PyObject *ret = pyyjson_dumps_obj_4_0(obj);

    if (unlikely(!ret)) {
        if (!PyErr_Occurred()) {
            PyErr_SetString(JSONEncodeError, "Failed to decode JSON: unknown error");
        }
    }

    return ret;
    // return pyyjson_dumps_obj_4_0(obj);
}
