#define XXH_INLINE_ALL
#include "decode.h"
#include "pyyjson.h"
#include "xxhash.h"
#include "decode_float.inl.h"
#include <assert.h>
#include <math.h>
#include <stdbool.h>
#include <stddef.h>
#include <threads.h>
#include "tls.h"

thread_local u8 pyyjson_string_buffer[PYYJSON_STRING_BUFFER_SIZE];


force_inline bool decode_ctn_is_arr(DecodeCtnWithSize*ctn){
    return ctn->raw < 0;
}

force_inline Py_ssize_t get_decode_ctn_len(DecodeCtnWithSize*ctn){
    return ctn->raw & PY_SSIZE_T_MAX;
}

force_inline void set_decode_ctn(DecodeCtnWithSize*ctn, Py_ssize_t len, bool is_arr){
    assert(len >= 0);
    ctn->raw = len | (is_arr ? PY_SSIZE_T_MIN : 0);
}

force_inline void incr_decode_ctn_size(DecodeCtnWithSize*ctn){
    assert(ctn->raw != PY_SSIZE_T_MAX);
    ctn->raw++;
}

force_inline bool ctn_grow_check(DecodeCtnStackInfo *decode_ctn_info) {
    return ++decode_ctn_info->ctn < decode_ctn_info->ctn_end;
}

#if PY_MINOR_VERSION >= 13
// these are hidden in Python 3.13
#if PY_MINOR_VERSION == 13
PyAPI_FUNC(Py_hash_t) _Py_HashBytes(const void *, Py_ssize_t);
#endif // PY_MINOR_VERSION == 13
PyAPI_FUNC(int) _PyDict_SetItem_KnownHash(PyObject *mp, PyObject *key, PyObject *item, Py_hash_t hash);
#endif // PY_MINOR_VERSION >= 13

#if PY_MINOR_VERSION >= 12
#define PYYJSON_PY_DECREF_DEBUG() (_Py_DECREF_STAT_INC())
#define PYYJSON_PY_INCREF_DEBUG() (_Py_INCREF_STAT_INC())
#else
#ifdef Py_REF_DEBUG
#define PYYJSON_PY_DECREF_DEBUG() (_Py_RefTotal--)
#define PYYJSON_PY_INCREF_DEBUG() (_Py_RefTotal++)
#else
#define PYYJSON_PY_DECREF_DEBUG()
#define PYYJSON_PY_INCREF_DEBUG()
#endif
#endif
#define REHASHER(_x) (((size_t) (_x)) % (PYYJSON_KEY_CACHE_SIZE))

typedef XXH64_hash_t pyyjson_hash_t;


// thread_local PyObject *pyobject_stack_buffer[PYYJSON_DECODE_OBJSTACK_BUFFER_SIZE];


#if PYYJSON_ENABLE_TRACE
Py_ssize_t max_str_len = 0;
int __count_trace[PYYJSON_OP_BITCOUNT_MAX] = {0};
int __hash_trace[PYYJSON_KEY_CACHE_SIZE] = {0};
size_t __hash_hit_counter = 0;
size_t __hash_add_key_call_count = 0;

#define PYYJSON_TRACE_STR_LEN(_len) max_str_len = max_str_len > _len ? max_str_len : _len
#define PYYJSON_TRACE_HASH(_hash) \
    __hash_add_key_call_count++;  \
    __hash_trace[_hash & (PYYJSON_KEY_CACHE_SIZE - 1)]++
#define PYYJSON_TRACE_CACHE_HIT() __hash_hit_counter++
#define PYYJSON_TRACE_HASH_CONFLICT(_hash) printf("hash conflict: %lld, index=%lld\n", (long long int) _hash, (long long int) (_hash & (PYYJSON_KEY_CACHE_SIZE - 1)))
#else // PYYJSON_ENABLE_TRACE
#define PYYJSON_TRACE_STR_LEN(_len) (void) (0)
#define PYYJSON_TRACE_HASH(_hash) (void) (0)
#define PYYJSON_TRACE_CACHE_HIT() (void) (0)
#define PYYJSON_TRACE_HASH_CONFLICT(_hash) (void) (0)
#endif // PYYJSON_ENABLE_TRACE

force_inline void Py_DecRef_NoCheck(PyObject *op) {
    // Non-limited C API and limited C API for Python 3.9 and older access
    // directly PyObject.ob_refcnt.
#if PY_MINOR_VERSION >= 12
    if (_Py_IsImmortal(op)) {
        return;
    }
#endif
    PYYJSON_PY_DECREF_DEBUG();
    assert(op->ob_refcnt > 1);
    --op->ob_refcnt;
}

force_inline void Py_Immortal_IncRef(PyObject *op) {
    // Non-limited C API and limited C API for Python 3.9 and older access
    // directly PyObject.ob_refcnt.
#if PY_MINOR_VERSION >= 12
#if SIZEOF_VOID_P > 4
    // Portable saturated add, branching on the carry flag and set low bits
#ifndef NDEBUG
    assert(0 > (int32_t) op->ob_refcnt_split[PY_BIG_ENDIAN]);
#endif // NDEBUG
#else  // SIZEOF_VOID_P > 4
    // Explicitly check immortality against the immortal value
    assert(_Py_IsImmortal(op));
#endif // SIZEOF_VOID_P > 4
#else  // PY_MINOR_VERSION >= 12
    op->ob_refcnt++;
#endif // PY_MINOR_VERSION >= 12
    PYYJSON_PY_INCREF_DEBUG();
}

pyyjson_cache_type AssociativeKeyCache[PYYJSON_KEY_CACHE_SIZE];

force_inline void add_key_cache(pyyjson_hash_t hash, PyObject *obj) {
    assert(PyUnicode_GET_LENGTH(obj) * PyUnicode_KIND(obj) <= 64);
    size_t index = REHASHER(hash);
    PYYJSON_TRACE_HASH(index);
    pyyjson_cache_type old = AssociativeKeyCache[index];
    if (old) {
        PYYJSON_TRACE_HASH_CONFLICT(hash);
        Py_DECREF(old);
    }
    Py_INCREF(obj);
    AssociativeKeyCache[index] = obj;
}

force_inline PyObject *get_key_cache(const u8 *utf8_str, pyyjson_hash_t hash, size_t real_len) {
    assert(real_len <= 64);
    pyyjson_cache_type cache = AssociativeKeyCache[REHASHER(hash)];
    if (!cache) return NULL;
    if (likely(((real_len == PyUnicode_GET_LENGTH(cache) * PyUnicode_KIND(cache))) && (memcmp(PyUnicode_DATA(cache), utf8_str, real_len) == 0))) {
        PYYJSON_TRACE_CACHE_HIT();
        return cache;
    }
    return NULL;
}

force_inline PyObject *make_string(const u8 *utf8_str, Py_ssize_t len, int type_flag, bool is_key) {
    PYYJSON_TRACE_STR_LEN(len);
    PyObject *obj;
    Py_UCS4 max_char;
    size_t real_len;
    XXH64_hash_t hash;

    switch (type_flag) {
        case PYYJSON_STRING_TYPE_ASCII: {
            max_char = 0x7f;
            real_len = len;
            break;
        }
        case PYYJSON_STRING_TYPE_LATIN1: {
            max_char = 0xff;
            real_len = len;
            break;
        }
        case PYYJSON_STRING_TYPE_UCS2: {
            max_char = 0xffff;
            real_len = len * 2;
            break;
        }
        case PYYJSON_STRING_TYPE_UCS4: {
            max_char = 0x10ffff;
            real_len = len * 4;
            break;
        }
        default:
            assert(false);
            Py_UNREACHABLE();
    }

    bool should_cache = (is_key && likely(real_len <= 64));

    if (should_cache) {
        hash = XXH3_64bits(utf8_str, real_len);
        obj = get_key_cache(utf8_str, hash, real_len);
        if (obj) {
            Py_INCREF(obj);
            return obj;
        }
    }

    obj = PyUnicode_New(len, max_char);
    if (obj == NULL) return NULL;
    memcpy(PyUnicode_DATA(obj), utf8_str, real_len);
    if (should_cache) {
        add_key_cache(hash, obj);
    }
success:
    if (is_key) {
        assert(((PyASCIIObject *) obj)->hash == -1);
#if PY_MINOR_VERSION >= 14
        ((PyASCIIObject *) obj)->hash = PyUnicode_Type.tp_hash(obj);
#else
        ((PyASCIIObject *) obj)->hash = _Py_HashBytes(utf8_str, real_len);
#endif
    }
    return obj;
}

force_inline bool init_decode_obj_stack_info(DecodeObjStackInfo *restrict decode_obj_stack_info) {
    PyObject **new_buffer = get_decode_obj_stack_buffer();
    if (unlikely(!new_buffer)) {
        PyErr_NoMemory();
        return false;
    }
    decode_obj_stack_info->result_stack = new_buffer;
    decode_obj_stack_info->cur_write_result_addr = new_buffer;
    decode_obj_stack_info->result_stack_end = new_buffer + PYYJSON_DECODE_OBJ_BUFFER_INIT_SIZE;
    // decode_obj_stack_info->op = op_head;
    // decode_obj_stack_info->result_stack = pyobject_stack_buffer;
    //     if (unlikely(obj_stack_maxsize > PYYJSON_DECODE_OBJSTACK_BUFFER_SIZE)) {
    //         decode_obj_stack_info->result_stack = (PyObject **) malloc(obj_stack_maxsize * sizeof(PyObject *));
    //         if (unlikely(decode_obj_stack_info->result_stack == NULL)) {
    //             PyErr_NoMemory();
    //             return false;
    //         }
    //     }
    //     decode_obj_stack_info->cur_write_result_addr = decode_obj_stack_info->result_stack;
    // // #ifndef NDEBUG
    //     decode_obj_stack_info->result_stack_end = decode_obj_stack_info->result_stack + obj_stack_maxsize;
    // #endif
    return true;
}

force_inline bool init_decode_ctn_stack_info(DecodeCtnStackInfo *restrict decode_ctn_stack_info) {
    DecodeCtnWithSize *new_buffer = get_decode_ctn_stack_buffer();
    if (unlikely(!new_buffer)) {
        PyErr_NoMemory();
        return false;
    }
    decode_ctn_stack_info->ctn_start = new_buffer;
    decode_ctn_stack_info->ctn = new_buffer;
    decode_ctn_stack_info->ctn_end = new_buffer + PYYJSON_DECODE_MAX_RECURSION;
    return true;
}

#if PYYJSON_ENABLE_TRACE
#define PYYJSON_TRACE_OP(x)                                 \
    do {                                                    \
        for (int i = 0; i < PYYJSON_OP_BITCOUNT_MAX; i++) { \
            if (x & (1 << i)) {                             \
                __count_trace[i]++;                         \
                break;                                      \
            }                                               \
        }                                                   \
        __op_counter++;                                     \
    } while (0)
#else
#define PYYJSON_TRACE_OP(x)    (void)0
#endif

#ifndef NDEBUG
#define PYYJSON_RESULT_STACK_GROW()                            \
    if (unlikely(decode_obj_stack_info->cur_write_result_addr >= decode_obj_stack_info->result_stack_end)) { \
        assert(false);                                         \
        Py_UNREACHABLE();                                      \
    }
#else
#define PYYJSON_RESULT_STACK_GROW() (void) (0)
#endif

#define PUSH_STACK_NO_CHECK(obj)      \
    do {                              \
        *decode_obj_stack_info->cur_write_result_addr = obj; \
        decode_obj_stack_info->cur_write_result_addr++;      \
    } while (0)


#define PYYJSON_PUSH_STACK(obj)      \
    do {                             \
        PYYJSON_RESULT_STACK_GROW(); \
        PUSH_STACK_NO_CHECK(obj);    \
    } while (0)


force_inline bool pyyjson_push_stack(DecodeObjStackInfo *restrict decode_obj_stack_info, PyObject *obj) {
    static_assert((PYYJSON_DECODE_OBJ_BUFFER_INIT_SIZE << 1) > 0, "(PYYJSON_DECODE_OBJSTACK_BUFFER_SIZE << 1) > 0");
    if (unlikely(decode_obj_stack_info->cur_write_result_addr >= decode_obj_stack_info->result_stack_end)) {
        // resize
        if (likely(PYYJSON_DECODE_OBJ_BUFFER_INIT_SIZE == decode_obj_stack_info->result_stack_end - decode_obj_stack_info->result_stack)) {
            void *new_buffer = malloc(sizeof(PyObject *) * (PYYJSON_DECODE_OBJ_BUFFER_INIT_SIZE << 1));
            if (unlikely(!new_buffer)) {
                PyErr_NoMemory();
                return false;
            }
            memcpy(new_buffer, decode_obj_stack_info->result_stack, sizeof(PyObject *) * PYYJSON_DECODE_OBJ_BUFFER_INIT_SIZE);
            decode_obj_stack_info->result_stack = (PyObject **) new_buffer;
            decode_obj_stack_info->cur_write_result_addr = decode_obj_stack_info->result_stack + PYYJSON_DECODE_OBJ_BUFFER_INIT_SIZE;
            decode_obj_stack_info->result_stack_end = decode_obj_stack_info->result_stack + (PYYJSON_DECODE_OBJ_BUFFER_INIT_SIZE << 1);
        } else {
            Py_ssize_t old_capacity = decode_obj_stack_info->result_stack_end - decode_obj_stack_info->result_stack;
            if (unlikely((PY_SSIZE_T_MAX >> 1) < old_capacity)) {
                PyErr_NoMemory();
                return false;
            }
            Py_ssize_t new_capacity = old_capacity << 1;
            void *new_buffer = realloc(decode_obj_stack_info->result_stack, sizeof(PyObject *) * new_capacity);
            if (unlikely(!new_buffer)) {
                PyErr_NoMemory();
                return false;
            }
            decode_obj_stack_info->result_stack = (PyObject **) new_buffer;
            decode_obj_stack_info->cur_write_result_addr = decode_obj_stack_info->result_stack + old_capacity;
            decode_obj_stack_info->result_stack_end = decode_obj_stack_info->result_stack + new_capacity;
        }
    }
    *decode_obj_stack_info->cur_write_result_addr++ = obj;
    return true;
}

force_inline bool pyyjson_decode_string(DecodeObjStackInfo* restrict decode_obj_stack_info, const u8 *utf8_str, Py_ssize_t len, int type_flag, bool is_key) {
    PYYJSON_TRACE_OP(PYYJSON_OP_STRING);
    // pyyjson_string_op *op_str = (pyyjson_string_op *) decode_obj_stack_info->op;
    PyObject *new_val = make_string(utf8_str, len, type_flag, is_key);
    RETURN_ON_UNLIKELY_ERR(!new_val);
    return pyyjson_push_stack(decode_obj_stack_info, new_val);
    // op_str++;
    // decode_obj_stack_info->op = (pyyjson_op *) op_str;
    // return true;
}

force_inline bool pyyjson_decode_double(DecodeObjStackInfo* restrict decode_obj_stack_info, double val) {
    PYYJSON_TRACE_OP(PYYJSON_OP_NUMBER);
    PyObject* obj = PyFloat_FromDouble(val);
    RETURN_ON_UNLIKELY_ERR(!obj);
    return pyyjson_push_stack(decode_obj_stack_info, obj);
}

force_inline bool pyyjson_decode_longlong(DecodeObjStackInfo* restrict decode_obj_stack_info, i64 val) {
    PYYJSON_TRACE_OP(PYYJSON_OP_NUMBER);
    PyObject* obj = PyLong_FromLongLong(val);
    RETURN_ON_UNLIKELY_ERR(!obj);
    return pyyjson_push_stack(decode_obj_stack_info, obj);
}

force_inline bool pyyjson_decode_unsignedlonglong(DecodeObjStackInfo* restrict decode_obj_stack_info, u64 val) {
    PYYJSON_TRACE_OP(PYYJSON_OP_NUMBER);
    PyObject* obj = PyLong_FromUnsignedLongLong(val);
    RETURN_ON_UNLIKELY_ERR(!obj);
    return pyyjson_push_stack(decode_obj_stack_info, obj);
}

force_inline bool pyyjson_decode_arr(DecodeObjStackInfo *restrict decode_obj_stack_info, Py_ssize_t arr_len) {
    assert(arr_len >= 0);
    // pyyjson_container_op* op_container;
    //decode_obj_stack_info->
    // op_container = (pyyjson_container_op *) decode_obj_stack_info->op;
    // Py_ssize_t len = //decode_obj_stack_info->
    // op_container->len;
    PyObject *list = PyList_New(arr_len);
    RETURN_ON_UNLIKELY_ERR(!list);
    // if (unlikely(list == NULL)) goto fail;
    // if (unlikely(arr_len == 0)) {
    //     // goto arr_end;
    //     return pyyjson_push_stack(decode_obj_stack_info, list);
    // }
    PyObject **list_val_start = decode_obj_stack_info->cur_write_result_addr - arr_len;
    assert(list_val_start >= decode_obj_stack_info->result_stack);
    for (Py_ssize_t j = 0; j < arr_len; j++) {
        PyObject *val = list_val_start[j];
        assert(val);
        PyList_SET_ITEM(list, j, val); // this never fails
    }
    decode_obj_stack_info->cur_write_result_addr -= arr_len;
    return pyyjson_push_stack(decode_obj_stack_info, list);
    // arr_end:
    //decode_obj_stack_info->
    // op_container++;
    // decode_obj_stack_info->op = (pyyjson_op *) //decode_obj_stack_info->
    // op_container;
    // return true;
}

force_inline bool pyyjson_decode_obj(DecodeObjStackInfo *restrict decode_obj_stack_info, Py_ssize_t dict_len) {
    // pyyjson_container_op* op_container;
    // //decode_obj_stack_info->
    // op_container = (pyyjson_container_op *) decode_obj_stack_info->op;
    // Py_ssize_t len = //decode_obj_stack_info->
    // op_container->len;
    PyObject *dict = _PyDict_NewPresized(dict_len);
    RETURN_ON_UNLIKELY_ERR(!dict);
    // if (unlikely(dict == NULL)) goto fail;
    // if (unlikely(len == 0)) {
    //     PYYJSON_PUSH_STACK(dict);
    //     goto dict_end;
    // }
    // PYYJSON_POP_STACK_PRE_CHECK(len * 2 - 1);
    PyObject **dict_val_start = decode_obj_stack_info->cur_write_result_addr - dict_len * 2;
    PyObject **dict_val_view = dict_val_start;
    for (size_t j = 0; j < dict_len; j++) {
        PyObject *key = *dict_val_view++;
        assert(PyUnicode_Check(key));
        PyObject *val = *dict_val_view++;
        assert(((PyASCIIObject *) key)->hash != -1);
        int retcode = _PyDict_SetItem_KnownHash(dict, key, val, ((PyASCIIObject *) key)->hash); // this may fail
        if (likely(0 == retcode)) {
            Py_DecRef_NoCheck(key);
            Py_DecRef_NoCheck(val);
        } else {
            // we already decrefed some objects, have to manually handle all refcnt here
            Py_DECREF(dict);
            // also need to clean up the rest k-v pairs
            for (size_t k = j * 2; k < dict_len * 2; k++) {
                Py_DECREF(dict_val_start[k]);
            }
            // move cur_write_result_addr to the first key addr, avoid double decref
            decode_obj_stack_info->cur_write_result_addr = dict_val_start;
            return false;
        }
    }
    decode_obj_stack_info->cur_write_result_addr -= dict_len * 2;
    return pyyjson_push_stack(decode_obj_stack_info, dict);
    // dict_end:
    //decode_obj_stack_info->
    // op_container++;
    // decode_obj_stack_info->op = (pyyjson_op *) //decode_obj_stack_info->
    // op_container;
    // break;
    // return true;
}

force_inline bool pyyjson_decode_null(DecodeObjStackInfo *restrict decode_obj_stack_info) {
    PYYJSON_TRACE_OP(PYYJSON_OP_CONSTANTS);
    Py_Immortal_IncRef(Py_None);
    return pyyjson_push_stack(decode_obj_stack_info, Py_None);
}

force_inline bool pyyjson_decode_false(DecodeObjStackInfo *restrict decode_obj_stack_info) {
    PYYJSON_TRACE_OP(PYYJSON_OP_CONSTANTS);
    Py_Immortal_IncRef(Py_False);
    return pyyjson_push_stack(decode_obj_stack_info, Py_False);
}

force_inline bool pyyjson_decode_true(DecodeObjStackInfo *restrict decode_obj_stack_info) {
    PYYJSON_TRACE_OP(PYYJSON_OP_CONSTANTS);
    Py_Immortal_IncRef(Py_True);
    return pyyjson_push_stack(decode_obj_stack_info, Py_True);
}

force_inline bool pyyjson_decode_nan(DecodeObjStackInfo *restrict decode_obj_stack_info, bool is_signed) {
    PYYJSON_TRACE_OP(PYYJSON_OP_NAN_INF);
    double val = is_signed ? -fabs(Py_NAN) : fabs(Py_NAN);
    PyObject *o = PyFloat_FromDouble(val);
    RETURN_ON_UNLIKELY_ERR(!o);
    return pyyjson_push_stack(decode_obj_stack_info, o);
    // decode_obj_stack_info->op++;
    // break;
}

force_inline bool pyyjson_decode_inf(DecodeObjStackInfo *restrict decode_obj_stack_info, bool is_signed) {
    PYYJSON_TRACE_OP(PYYJSON_OP_NAN_INF);
    double val = is_signed ? -fabs(Py_HUGE_VAL) : fabs(Py_HUGE_VAL);
    PyObject *o = PyFloat_FromDouble(val);
    RETURN_ON_UNLIKELY_ERR(!o);
    return pyyjson_push_stack(decode_obj_stack_info, o);
    // decode_obj_stack_info->op++;
    // break;
}

// PyObject *pyyjson_op_loads(pyyjson_op *restrict op_head, size_t obj_stack_maxsize) {
// #if PYYJSON_ENABLE_TRACE
//     size_t __op_counter = 0;

// #define PYYJSON_SHOW_OP_TRACE()                                   \
//     do {                                                          \
//         for (int i = 0; i < PYYJSON_OP_BITCOUNT_MAX; i++) {       \
//             if (__count_trace[i] > 0) {                           \
//                 printf("op %d: %d\n", i, __count_trace[i]);       \
//             }                                                     \
//         }                                                         \
//         printf("total op: %lld\n", (long long int) __op_counter); \
//     } while (0)

// #define PYYJSON_SHOW_STR_TRACE() printf("max str len: %lld\n", (long long int) max_str_len)
// #else // PYYJSON_ENABLE_TRACE
// #define PYYJSON_SHOW_OP_TRACE() (void) (0)
// #define PYYJSON_SHOW_STR_TRACE() (void) (0)
// #endif // PYYJSON_ENABLE_TRACE
//     DecodeObjStackInfo _decode_obj_stack_info;
//     DecodeObjStackInfo* const decode_obj_stack_info = &_decode_obj_stack_info;
//     // if(unlikely(!init_decode_obj_stack_info(decode_obj_stack_info, op_head, obj_stack_maxsize))){
//     //     return NULL;
//     // }
// //     pyyjson_op *op = op_head;
// //     PyObject **result_stack;
// //     result_stack = pyobject_stack_buffer;
// //     if (unlikely(obj_stack_maxsize > PYYJSON_DECODE_OBJSTACK_BUFFER_SIZE)) {
// //         result_stack = (PyObject **) malloc(obj_stack_maxsize * sizeof(PyObject *));
// //         if (unlikely(result_stack == NULL)) {
// //             PyErr_NoMemory();
// //             return NULL;
// //         }
// //     }
// //     PyObject **cur_write_result_addr = result_stack;
// // #ifndef NDEBUG
// //     PyObject **result_stack_end = result_stack + obj_stack_maxsize;
// // #endif
// //     //
// //     pyyjson_container_op *op_container;

// #define PYYJSON_RESULT_STACK_REALLOC_CHECK() \
//     do {                                     \
//         if (new_result_stack == NULL) {      \
//             PyErr_NoMemory();                \
//             goto fail;                       \
//         }                                    \
//     } while (0)



// #ifndef NDEBUG
// #define PYYJSON_POP_STACK_PRE_CHECK(size)                                              \
//     do {                                                                               \
//         if (unlikely((size) > (Py_ssize_t) (decode_obj_stack_info->cur_write_result_addr - decode_obj_stack_info->result_stack))) {  \
//             PyErr_Format(PyExc_RuntimeError,                                           \
//                          "Stack underflow: expected to have at least %lld, got %lld",  \
//                          (size), (Py_ssize_t) (decode_obj_stack_info->cur_write_result_addr - decode_obj_stack_info->result_stack)); \
//             goto fail;                                                                 \
//         }                                                                              \
//     } while (0)
// #else // NDEBUG
// #define PYYJSON_POP_STACK_PRE_CHECK(size) (void) (0)
// #endif // NDEBUG
//     while (1) {
//         switch (PYYJSON_READ_OP(decode_obj_stack_info->op) & PYYJSON_OP_MASK) {
//             case PYYJSON_OP_STRING: {
//                 // PYYJSON_TRACE_OP(PYYJSON_OP_STRING);
//                 // pyyjson_string_op *op_str = (pyyjson_string_op *) decode_obj_stack_info->op;
//                 // PyObject *new_val = make_string(op_str->data, op_str->len, PYYJSON_READ_OP(decode_obj_stack_info->op));
//                 // if (new_val == NULL) goto fail;
//                 // PYYJSON_PUSH_STACK(new_val);
//                 // op_str++;
//                 // decode_obj_stack_info->op = (pyyjson_op *) op_str;
//                 break;
//             }
//             case PYYJSON_OP_NUMBER: {
//                 PYYJSON_TRACE_OP(PYYJSON_OP_NUMBER);
//                 pyyjson_number_op *op_num = (pyyjson_number_op *) decode_obj_stack_info->op;
//                 switch (PYYJSON_GET_FLAG(PYYJSON_READ_OP(decode_obj_stack_info->op), PYYJSON_NUM_FLAG_MASK)) {
//                     PYYJSON_MATCH_FLAG(PYYJSON_NUM_FLAG_FLOAT) : {
//                         PYYJSON_PUSH_STACK(PyFloat_FromDouble(op_num->data.f));
//                         break;
//                     }
//                     PYYJSON_MATCH_FLAG(PYYJSON_NUM_FLAG_INT) : {
//                         PYYJSON_PUSH_STACK(PyLong_FromLongLong(op_num->data.i));
//                         break;
//                     }
//                     PYYJSON_MATCH_FLAG(PYYJSON_NUM_FLAG_UINT) : {
//                         PYYJSON_PUSH_STACK(PyLong_FromUnsignedLongLong(op_num->data.u));
//                         break;
//                     }
//                     default:
//                         assert(false);
//                         Py_UNREACHABLE();
//                 }
//                 op_num++;
//                 decode_obj_stack_info->op = (pyyjson_op *) op_num;
//                 break;
//             }
//             case PYYJSON_OP_CONTAINER: {
//                 // PYYJSON_TRACE_OP(PYYJSON_OP_CONTAINER);
//                 // switch (PYYJSON_GET_FLAG(PYYJSON_READ_OP(decode_obj_stack_info->op), PYYJSON_CONTAINER_FLAG_MASK)) {
//                 //     PYYJSON_MATCH_FLAG(PYYJSON_CONTAINER_FLAG_ARRAY) : {
//                 //         goto container_array;
//                 //     }
//                 //     PYYJSON_MATCH_FLAG(PYYJSON_CONTAINER_FLAG_DICT) : {
//                 //         goto container_dict;
//                 //     }
//                 //     default:
//                 //         assert(false);
//                 //         Py_UNREACHABLE();
//                 // }
//                 // assert(false);
//                 // Py_UNREACHABLE();
//                 // {
//                 // container_array:
//                 //     pyyjson_container_op* op_container;
//                 //     //decode_obj_stack_info->
//                 //     op_container = (pyyjson_container_op *) decode_obj_stack_info->op;
//                 //     Py_ssize_t len = //decode_obj_stack_info->
//                 //     op_container->len;
//                 //     PyObject *list = PyList_New(len);
//                 //     if (unlikely(list == NULL)) goto fail;
//                 //     if (unlikely(len == 0)) {
//                 //         PYYJSON_PUSH_STACK(list);
//                 //         goto arr_end;
//                 //     }
//                 //     PYYJSON_POP_STACK_PRE_CHECK(len - 1);
//                 //     PyObject **list_val_start = decode_obj_stack_info->cur_write_result_addr - len;
//                 //     for (size_t j = 0; j < len; j++) {
//                 //         PyObject *val = list_val_start[j];
//                 //         PyList_SET_ITEM(list, j, val); // this never fails
//                 //     }
//                 //     decode_obj_stack_info->cur_write_result_addr -= len;
//                 //     PUSH_STACK_NO_CHECK(list);
//                 // arr_end:
//                 //     //decode_obj_stack_info->
//                 //     op_container++;
//                 //     decode_obj_stack_info->op = (pyyjson_op *) //decode_obj_stack_info->
//                 //     op_container;
//                 //     break;
//                 // }
//                 // {
//                 //     pyyjson_container_op* op_container;
//                 // container_dict:
//                 //     //decode_obj_stack_info->
//                 //     op_container = (pyyjson_container_op *) decode_obj_stack_info->op;
//                 //     Py_ssize_t len = //decode_obj_stack_info->
//                 //     op_container->len;
//                 //     PyObject *dict = _PyDict_NewPresized(len);
//                 //     if (unlikely(dict == NULL)) goto fail;
//                 //     if (unlikely(len == 0)) {
//                 //         PYYJSON_PUSH_STACK(dict);
//                 //         goto dict_end;
//                 //     }
//                 //     PYYJSON_POP_STACK_PRE_CHECK(len * 2 - 1);
//                 //     PyObject **dict_val_start = decode_obj_stack_info->cur_write_result_addr - len * 2;
//                 //     for (size_t j = 0; j < len; j++) {
//                 //         PyObject *key = dict_val_start[j * 2];
//                 //         assert(PyUnicode_Check(key));
//                 //         PyObject *val = dict_val_start[j * 2 + 1];
//                 //         assert(((PyASCIIObject *) key)->hash != -1);
//                 //         int retcode = _PyDict_SetItem_KnownHash(dict, key, val, ((PyASCIIObject *) key)->hash); // this may fail
//                 //         if (likely(0 == retcode)) {
//                 //             Py_DecRef_NoCheck(key);
//                 //             Py_DecRef_NoCheck(val);
//                 //         } else {
//                 //             Py_DECREF(dict);
//                 //             // also need to clean up the rest k-v pairs
//                 //             for (size_t k = j * 2; k < len * 2; k++) {
//                 //                 Py_DECREF(dict_val_start[k]);
//                 //             }
//                 //             // move cur_write_result_addr to the first key addr, avoid double decref
//                 //             decode_obj_stack_info->cur_write_result_addr = dict_val_start;
//                 //             goto fail;
//                 //         }
//                 //     }
//                 //     decode_obj_stack_info->cur_write_result_addr -= len * 2;
//                 //     PUSH_STACK_NO_CHECK(dict);
//                 // dict_end:
//                 //     //decode_obj_stack_info->
//                 //     op_container++;
//                 //     decode_obj_stack_info->op = (pyyjson_op *) //decode_obj_stack_info->
//                 //     op_container;
//                 //     break;
//                 // }
//                 break;
//             }
//             case PYYJSON_OP_CONSTANTS: {
//                 // PYYJSON_TRACE_OP(PYYJSON_OP_CONSTANTS);
//                 // switch (PYYJSON_GET_FLAG(PYYJSON_READ_OP(decode_obj_stack_info->op), PYYJSON_CONSTANTS_FLAG_MASK)) {
//                 //     PYYJSON_MATCH_FLAG(PYYJSON_CONSTANTS_FLAG_NULL) : {
//                 //         Py_Immortal_IncRef(Py_None);
//                 //         PYYJSON_PUSH_STACK(Py_None);
//                 //         decode_obj_stack_info->op++;
//                 //         break;
//                 //     }
//                 //     PYYJSON_MATCH_FLAG(PYYJSON_CONSTANTS_FLAG_FALSE) : {
//                 //         Py_Immortal_IncRef(Py_False);
//                 //         PYYJSON_PUSH_STACK(Py_False);
//                 //         decode_obj_stack_info->op++;
//                 //         break;
//                 //     }
//                 //     PYYJSON_MATCH_FLAG(PYYJSON_CONSTANTS_FLAG_TRUE) : {
//                 //         Py_Immortal_IncRef(Py_True);
//                 //         PYYJSON_PUSH_STACK(Py_True);
//                 //         decode_obj_stack_info->op++;
//                 //         break;
//                 //     }
//                 //     default:
//                 //         assert(false);
//                 //         Py_UNREACHABLE();
//                 //         break;
//                 // }
//                 break;
//             }
//             case PYYJSON_OP_NAN_INF: {
//                 // PYYJSON_TRACE_OP(PYYJSON_OP_NAN_INF);
//                 // switch (PYYJSON_GET_FLAG(PYYJSON_READ_OP(decode_obj_stack_info->op), PYYJSON_NAN_INF_FLAG_MASK_WITHOUT_SIGNED)) {
//                 //     PYYJSON_MATCH_FLAG(PYYJSON_NAN_INF_FLAG_NAN) : {
//                 //         double val = (PYYJSON_READ_OP(decode_obj_stack_info->op) & PYYJSON_NAN_INF_FLAG_SIGNED) ? -fabs(Py_NAN) : fabs(Py_NAN);
//                 //         PyObject *o = PyFloat_FromDouble(val);
//                 //         assert(o);
//                 //         PYYJSON_PUSH_STACK(o);
//                 //         decode_obj_stack_info->op++;
//                 //         break;
//                 //     }
//                 //     PYYJSON_MATCH_FLAG(PYYJSON_NAN_INF_FLAG_INF) : {
//                 //         double val = (PYYJSON_READ_OP(decode_obj_stack_info->op) & PYYJSON_NAN_INF_FLAG_SIGNED) ? -Py_HUGE_VAL : Py_HUGE_VAL;
//                 //         PyObject *o = PyFloat_FromDouble(val);
//                 //         assert(o);
//                 //         PYYJSON_PUSH_STACK(o);
//                 //         decode_obj_stack_info->op++;
//                 //         break;
//                 //     }
//                 //     default:
//                 //         assert(false);
//                 //         Py_UNREACHABLE();
//                 //         break;
//                 // }
//                 break;
//             }
//             case PYYJSON_NO_OP: {
//                 goto success;
//             }
//             default:
//                 assert(false);
//                 Py_UNREACHABLE();
//         }
//     }
// success:
//     assert(decode_obj_stack_info->cur_write_result_addr - decode_obj_stack_info->result_stack == 1);
//     PyObject *result = *decode_obj_stack_info->result_stack;
//     if (unlikely(decode_obj_stack_info->result_stack != pyobject_stack_buffer)) free(decode_obj_stack_info->result_stack);
//     PYYJSON_SHOW_OP_TRACE();
//     PYYJSON_SHOW_STR_TRACE();
//     return result;
// fail:
//     // decref all objects in the stack
//     for (PyObject **p = decode_obj_stack_info->result_stack; p < decode_obj_stack_info->cur_write_result_addr; p++) {
//         Py_DECREF(*p);
//     }
//     if (unlikely(decode_obj_stack_info->result_stack != pyobject_stack_buffer)) free(decode_obj_stack_info->result_stack);
//     if (PyErr_Occurred() == NULL) {
//         PyErr_Format(PyExc_RuntimeError, "Analyze pyyjson opcode failed at %lld", decode_obj_stack_info->cur_write_result_addr - decode_obj_stack_info->result_stack);
//     }
//     return NULL;
// }

/** Character type table (generate with misc/make_tables.c) */
static const u8 char_table[256] = {
    0x44, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04,
    0x04, 0x05, 0x45, 0x04, 0x04, 0x45, 0x04, 0x04,
    0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04,
    0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04,
    0x01, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x20,
    0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82,
    0x82, 0x82, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x10, 0x04, 0x00, 0x00, 0x00,
    0x00, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00,
    0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08,
    0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08,
    0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08,
    0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08,
    0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08,
    0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08,
    0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08,
    0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08,
    0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08,
    0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08,
    0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08,
    0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08,
    0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08,
    0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08,
    0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08,
    0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08
};


/** Match a character with specified type. */
force_inline bool char_is_type(u8 c, u8 type) {
    return (char_table[c] & type) != 0;
}

/** Match a whitespace: ' ', '\\t', '\\n', '\\r'. */
force_inline bool char_is_space(u8 c) {
    return char_is_type(c, (u8)CHAR_TYPE_SPACE);
}

/** Match a whitespace or comment: ' ', '\\t', '\\n', '\\r', '/'. */
force_inline bool char_is_space_or_comment(u8 c) {
    return char_is_type(c, (u8)(CHAR_TYPE_SPACE | CHAR_TYPE_COMMENT));
}

/** Match a JSON number: '-', [0-9]. */
force_inline bool char_is_number(u8 c) {
    return char_is_type(c, (u8)CHAR_TYPE_NUMBER);
}

/** Match a JSON container: '{', '['. */
force_inline bool char_is_container(u8 c) {
    return char_is_type(c, (u8)CHAR_TYPE_CONTAINER);
}

/** Match a stop character in ASCII string: '"', '\', [0x00-0x1F,0x80-0xFF]. */
force_inline bool char_is_ascii_stop(u8 c) {
    return char_is_type(c, (u8)(CHAR_TYPE_ESC_ASCII |
                                       CHAR_TYPE_NON_ASCII));
}

/** Match a line end character: '\\n', '\\r', '\0'. */
// force_inline bool char_is_line_end(u8 c) {
//     return char_is_type(c, (u8)CHAR_TYPE_LINE_END);
// }

/** Match a hexadecimal numeric character: [0-9a-fA-F]. */
// force_inline bool char_is_hex(u8 c) {
//     return char_is_type(c, (u8)CHAR_TYPE_HEX);
// }



force_inline u16 read_b2_unicode(u32 uni) {
#if PY_BIG_ENDIAN
    return ((uni & 0x1f000000) >> 18) | ((uni & 0x3f0000) >> 16);
#else
    return ((uni & 0x1f) << 6) | ((uni & 0x3f00) >> 8);
#endif
}

force_inline u16 read_b3_unicode(u32 uni) {
#if PY_BIG_ENDIAN
    return ((uni & 0x0f000000) >> 12) | ((uni & 0x3f0000) >> 10) | ((uni & 0x3f00) >> 8);
#else
    return ((uni & 0x0f) << 12) | ((uni & 0x3f00) >> 2) | ((uni & 0x3f0000) >> 16);
#endif
}

force_inline u32 read_b4_unicode(u32 uni) {
#if PY_BIG_ENDIAN
    return ((uni & 0x07000000) >> 6) | ((uni & 0x3f0000) >> 4) | ((uni & 0x3f00) >> 2)  | ((uni & 0x3f));
#else
    return ((uni & 0x07) << 18) | ((uni & 0x3f00) << 4) | ((uni & 0x3f0000) >> 10) | ((uni & 0x3f000000) >> 24);
#endif
}


/**
 Read a JSON string.
 @param ptr The head pointer of string before '"' prefix (inout).
 @param lst JSON last position.
 @param inv Allow invalid unicode.
 @param val The string value to be written.
 @param msg The error message pointer.
 @return Whether success.
 */
force_inline bool read_string(DecodeObjStackInfo *restrict decode_obj_stack_info, u8 **ptr, u8 *write_buffer, bool is_key) {
    /*
     Each unicode code point is encoded as 1 to 4 bytes in UTF-8 encoding,
     we use 4-byte mask and pattern value to validate UTF-8 byte sequence,
     this requires the input data to have 4-byte zero padding.
     ---------------------------------------------------
     1 byte
     unicode range [U+0000, U+007F]
     unicode min   [.......0]
     unicode max   [.1111111]
     bit pattern   [0.......]
     ---------------------------------------------------
     2 byte
     unicode range [U+0080, U+07FF]
     unicode min   [......10 ..000000]
     unicode max   [...11111 ..111111]
     bit require   [...xxxx. ........] (1E 00)
     bit mask      [xxx..... xx......] (E0 C0)
     bit pattern   [110..... 10......] (C0 80)
     ---------------------------------------------------
     3 byte
     unicode range [U+0800, U+FFFF]
     unicode min   [........ ..100000 ..000000]
     unicode max   [....1111 ..111111 ..111111]
     bit require   [....xxxx ..x..... ........] (0F 20 00)
     bit mask      [xxxx.... xx...... xx......] (F0 C0 C0)
     bit pattern   [1110.... 10...... 10......] (E0 80 80)
     ---------------------------------------------------
     3 byte invalid (reserved for surrogate halves)
     unicode range [U+D800, U+DFFF]
     unicode min   [....1101 ..100000 ..000000]
     unicode max   [....1101 ..111111 ..111111]
     bit mask      [....xxxx ..x..... ........] (0F 20 00)
     bit pattern   [....1101 ..1..... ........] (0D 20 00)
     ---------------------------------------------------
     4 byte
     unicode range [U+10000, U+10FFFF]
     unicode min   [........ ...10000 ..000000 ..000000]
     unicode max   [.....100 ..001111 ..111111 ..111111]
     bit require   [.....xxx ..xx.... ........ ........] (07 30 00 00)
     bit mask      [xxxxx... xx...... xx...... xx......] (F8 C0 C0 C0)
     bit pattern   [11110... 10...... 10...... 10......] (F0 80 80 80)
     ---------------------------------------------------
     */
#if PY_BIG_ENDIAN
    const u32 b1_mask = 0x80000000UL;
    const u32 b1_patt = 0x00000000UL;
    const u32 b2_mask = 0xE0C00000UL;
    const u32 b2_patt = 0xC0800000UL;
    const u32 b2_requ = 0x1E000000UL;
    const u32 b3_mask = 0xF0C0C000UL;
    const u32 b3_patt = 0xE0808000UL;
    const u32 b3_requ = 0x0F200000UL;
    const u32 b3_erro = 0x0D200000UL;
    const u32 b4_mask = 0xF8C0C0C0UL;
    const u32 b4_patt = 0xF0808080UL;
    const u32 b4_requ = 0x07300000UL;
    const u32 b4_err0 = 0x04000000UL;
    const u32 b4_err1 = 0x03300000UL;
#else
    const u32 b1_mask = 0x00000080UL;
    const u32 b1_patt = 0x00000000UL;
    const u32 b2_mask = 0x0000C0E0UL;
    const u32 b2_patt = 0x000080C0UL;
    const u32 b2_requ = 0x0000001EUL;
    const u32 b3_mask = 0x00C0C0F0UL;
    const u32 b3_patt = 0x008080E0UL;
    const u32 b3_requ = 0x0000200FUL;
    const u32 b3_erro = 0x0000200DUL;
    const u32 b4_mask = 0xC0C0C0F8UL;
    const u32 b4_patt = 0x808080F0UL;
    const u32 b4_requ = 0x00003007UL;
    const u32 b4_err0 = 0x00000004UL;
    const u32 b4_err1 = 0x00003003UL;
#endif
    
#define is_valid_seq_1(uni) ( \
    ((uni & b1_mask) == b1_patt) \
)

#define is_valid_seq_2(uni) ( \
    ((uni & b2_mask) == b2_patt) && \
    ((uni & b2_requ)) \
)
    
#define is_valid_seq_3(uni) ( \
    ((uni & b3_mask) == b3_patt) && \
    ((tmp = (uni & b3_requ))) && \
    ((tmp != b3_erro)) \
)
    
#define is_valid_seq_4(uni) ( \
    ((uni & b4_mask) == b4_patt) && \
    ((tmp = (uni & b4_requ))) && \
    ((tmp & b4_err0) == 0 || (tmp & b4_err1) == 0) \
)

#define return_err(_end, _msg)                                                        \
    do {                                                                              \
        PyErr_Format(JSONDecodeError, "%s, at position %zu", _msg, _end - src_start); \
        return false;                                                                 \
    } while (0)

    // #define return_err(_end, _msg) do { \
//     *msg = _msg; \
//     *end = _end; \
//     goto failed; \
// } while (false)

    u8 *cur = (u8 *)*ptr;
    // u8 **end = (u8 **)ptr;
    /* modified BEGIN */
    // u8 *src = ++cur, *dst, *pos;
    u8 *src = ++cur, *pos;
    /* modified END */
    u16 hi, lo;
    u32 uni, tmp;
    /* modified BEGIN */
    u8* const src_start = src;
    size_t len_ucs1 = 0, len_ucs2 = 0, len_ucs4 = 0;
    u8* temp_string_buf = write_buffer;//(u8*)*buffer;
    u8* dst = temp_string_buf;
    u8 cur_max_ucs_size = 1;
    u16* dst_ucs2;
    u32* dst_ucs4;
    bool is_ascii = true;
    /* modified END */

skip_ascii:
    /* Most strings have no escaped characters, so we can jump them quickly. */
    
skip_ascii_begin:
    /*
     We want to make loop unrolling, as shown in the following code. Some
     compiler may not generate instructions as expected, so we rewrite it with
     explicit goto statements. We hope the compiler can generate instructions
     like this: https://godbolt.org/z/8vjsYq
     
         while (true) repeat16({
            if (likely(!(char_is_ascii_stop(*src)))) src++;
            else break;
         })
     */
#define expr_jump(i) \
    if (likely(!char_is_ascii_stop(src[i]))) {} \
    else goto skip_ascii_stop##i;
    
#define expr_stop(i) \
    skip_ascii_stop##i: \
    src += i; \
    goto skip_ascii_end;
    
    REPEAT_INCR_16(expr_jump)
    src += 16;
    goto skip_ascii_begin;
    REPEAT_INCR_16(expr_stop)
    
#undef expr_jump
#undef expr_stop
    
skip_ascii_end:
    
    /*
     GCC may store src[i] in a register at each line of expr_jump(i) above.
     These instructions are useless and will degrade performance.
     This inline asm is a hint for gcc: "the memory has been modified,
     do not cache it".
     
     MSVC, Clang, ICC can generate expected instructions without this hint.
     */
#if PYYJSON_IS_REAL_GCC
    __asm__ volatile("":"=m"(*src));
#endif
    if (likely(*src == '"')) {
        /* modified BEGIN */
        // this is a fast path for ascii strings. directly copy the buffer to pyobject
        *ptr = src + 1;
        return pyyjson_decode_string(decode_obj_stack_info, src_start, src - src_start, PYYJSON_STRING_TYPE_ASCII, is_key);
        // pyyjson_string_op* string_op =(pyyjson_string_op*) *op;
        // PYYJSON_WRITE_OP(string_op, PYYJSON_OP_STRING | PYYJSON_STRING_FLAG_ASCII);
        // string_op->data = (char *)src_start;
        // string_op->len = src - src_start;
        // *op = (pyyjson_op*)(string_op + 1);
        // // buffer unchanged
        // return true;
        
    } else if(src != src_start){
        memcpy(temp_string_buf, src_start, src - src_start);
        len_ucs1 = src - src_start;
        dst += len_ucs1;
    }
    goto copy_utf8_ucs1;
    /* modified END */

    /* modified BEGIN */    
// skip_utf8:
//     if (*src & 0x80) { /* non-ASCII character */
//         /*
//          Non-ASCII character appears here, which means that the text is likely
//          to be written in non-English or emoticons. According to some common
//          data set statistics, byte sequences of the same length may appear
//          consecutively. We process the byte sequences of the same length in each
//          loop, which is more friendly to branch prediction.
//          */
//         pos = src;

//         while (true) repeat8({
//             if (likely((*src & 0xF0) == 0xE0)) src += 3;
//             else break;
//         })
//         if (*src < 0x80) goto skip_ascii;
//         while (true) repeat8({
//             if (likely((*src & 0xE0) == 0xC0)) src += 2;
//             else break;
//         })
//         while (true) repeat8({
//             if (likely((*src & 0xF8) == 0xF0)) src += 4;
//             else break;
//         })

//         if (unlikely(pos == src)) {
//             if (!inv) return_err(src, "invalid UTF-8 encoding in string");
//             ++src;
//         }
//         goto skip_ascii;
//     }
    
    /* The escape character appears, we need to copy it. */
    // dst = src;
    /* modified END */

    /* modified BEGIN */
copy_escape_ucs1:
    if (likely(*src == '\\')) {
    /* modified END */
        switch (*++src) {
            case '"':  *dst++ = '"';  src++; break;
            case '\\': *dst++ = '\\'; src++; break;
            case '/':  *dst++ = '/';  src++; break;
            case 'b':  *dst++ = '\b'; src++; break;
            case 'f':  *dst++ = '\f'; src++; break;
            case 'n':  *dst++ = '\n'; src++; break;
            case 'r':  *dst++ = '\r'; src++; break;
            case 't':  *dst++ = '\t'; src++; break;
            case 'u':
                if (unlikely(!read_hex_u16(++src, &hi))) {
                    return_err(src - 2, "invalid escaped sequence in string");
                }
                src += 4;
                if (likely((hi & 0xF800) != 0xD800)) {
                    /* a BMP character */
                    /* modified BEGIN */
                    if (hi >= 0x100) {
                        // BEGIN ucs1 -> ucs2
                        assert(cur_max_ucs_size == 1);
                        len_ucs1 = dst - (u8*)temp_string_buf;
                        dst_ucs2 = ((u16*)temp_string_buf) + len_ucs1;
                        cur_max_ucs_size = 2;
                        // END ucs1 -> ucs2
                        *dst_ucs2++ = hi;
                        goto copy_ascii_ucs2;
                    } else {
                        if (hi >= 0x80) is_ascii = false;  // latin1
                        *dst++ = (u8)hi;
                        goto copy_ascii_ucs1;
                    }
                    // if (hi >= 0x800) {
                    //     *dst++ = (u8)(0xE0 | (hi >> 12));
                    //     *dst++ = (u8)(0x80 | ((hi >> 6) & 0x3F));
                    //     *dst++ = (u8)(0x80 | (hi & 0x3F));
                    // } else if (hi >= 0x80) {
                    //     *dst++ = (u8)(0xC0 | (hi >> 6));
                    //     *dst++ = (u8)(0x80 | (hi & 0x3F));
                    // } else {
                    //     *dst++ = (u8)hi;
                    // }
                    /* modified END */
                } else {
                    /* a non-BMP character, represented as a surrogate pair */
                    if (unlikely((hi & 0xFC00) != 0xD800)) {
                        return_err(src - 6, "invalid high surrogate in string");
                    }
                    if (unlikely(!byte_match_2(src, "\\u"))) {
                        return_err(src, "no low surrogate in string");
                    }
                    if (unlikely(!read_hex_u16(src + 2, &lo))) {
                        return_err(src, "invalid escaped sequence in string");
                    }
                    if (unlikely((lo & 0xFC00) != 0xDC00)) {
                        return_err(src, "invalid low surrogate in string");
                    }
                    uni = ((((u32)hi - 0xD800) << 10) |
                            ((u32)lo - 0xDC00)) + 0x10000;
                    /* modified BEGIN */
                    // BEGIN ucs1 -> ucs4
                    assert(cur_max_ucs_size == 1);
                    len_ucs1 = dst - (u8*)temp_string_buf;
                    dst_ucs4 = ((u32*)temp_string_buf) + len_ucs1;
                    cur_max_ucs_size = 4;
                    // END ucs1 -> ucs4
                    *dst_ucs4++ = uni;
                    // *dst++ = (u8)(0xF0 | (uni >> 18));
                    // *dst++ = (u8)(0x80 | ((uni >> 12) & 0x3F));
                    // *dst++ = (u8)(0x80 | ((uni >> 6) & 0x3F));
                    // *dst++ = (u8)(0x80 | (uni & 0x3F));
                    src += 6;
                    goto copy_ascii_ucs4;
                    /* modified END */
                }
                break;
            default: return_err(src, "invalid escaped character in string");
        }
        /* modified BEGIN */
        goto copy_ascii_ucs1;
        /* modified END */
        /* modified BEGIN */
    } else if (likely(*src == '"')) {
        goto read_finalize;
        
        /* modified END */
    } else {
        /* modified BEGIN */
        return_err(src, "unexpected control character in string");
        // if (!inv) return_err(src, "unexpected control character in string");
        // if (src >= lst) return_err(src, "unclosed string");
        // *dst++ = *src++;
        /* modified END */
    }
    assert(false);
    Py_UNREACHABLE();

    /* modified BEGIN */
copy_ascii_ucs1:
    /* modified END */
    /*
     Copy continuous ASCII, loop unrolling, same as the following code:
     
         while (true) repeat16({
            if (unlikely(char_is_ascii_stop(*src))) break;
            *dst++ = *src++;
         })
     */
#if PYYJSON_IS_REAL_GCC
    /* modified BEGIN */
#   define expr_jump(i) \
    if (likely(!(char_is_ascii_stop(src[i])))) {} \
    else { __asm__ volatile("":"=m"(src[i])); goto copy_ascii_ucs1_stop_##i; }
    /* modified END */
#else
    /* modified BEGIN */
#   define expr_jump(i) \
    if (likely(!(char_is_ascii_stop(src[i])))) {} \
    else { goto copy_ascii_ucs1_stop_##i; }
    /* modified END */
#endif
    REPEAT_INCR_16(expr_jump)
#undef expr_jump
    
    byte_move_16(dst, src);
    src += 16;
    dst += 16;
    /* modified BEGIN */
    goto copy_ascii_ucs1;
    /* modified END */
    
    /*
     The memory will be moved forward by at least 1 byte. So the `byte_move`
     can be one byte more than needed to reduce the number of instructions.
     */
    /* modified BEGIN */
copy_ascii_ucs1_stop_0:
    goto copy_utf8_ucs1;
copy_ascii_ucs1_stop_1:
    /* modified END */
    byte_move_2(dst, src);
    src += 1;
    dst += 1;
    /* modified BEGIN */
    goto copy_utf8_ucs1;
copy_ascii_ucs1_stop_2:
    /* modified END */
    byte_move_2(dst, src);
    src += 2;
    dst += 2;
    /* modified BEGIN */
    goto copy_utf8_ucs1;
copy_ascii_ucs1_stop_3:
    /* modified END */
    byte_move_4(dst, src);
    src += 3;
    dst += 3;
    /* modified BEGIN */
    goto copy_utf8_ucs1;
copy_ascii_ucs1_stop_4:
    /* modified END */
    byte_move_4(dst, src);
    src += 4;
    dst += 4;
    /* modified BEGIN */
    goto copy_utf8_ucs1;
copy_ascii_ucs1_stop_5:
    /* modified END */
    byte_move_4(dst, src);
    byte_move_2(dst + 4, src + 4);
    src += 5;
    dst += 5;
    /* modified BEGIN */
    goto copy_utf8_ucs1;
copy_ascii_ucs1_stop_6:
    /* modified END */
    byte_move_4(dst, src);
    byte_move_2(dst + 4, src + 4);
    src += 6;
    dst += 6;
    /* modified BEGIN */
    goto copy_utf8_ucs1;
copy_ascii_ucs1_stop_7:
    /* modified END */
    byte_move_8(dst, src);
    src += 7;
    dst += 7;
    /* modified BEGIN */
    goto copy_utf8_ucs1;
copy_ascii_ucs1_stop_8:
    /* modified END */
    byte_move_8(dst, src);
    src += 8;
    dst += 8;
    /* modified BEGIN */
    goto copy_utf8_ucs1;
copy_ascii_ucs1_stop_9:
    /* modified END */
    byte_move_8(dst, src);
    byte_move_2(dst + 8, src + 8);
    src += 9;
    dst += 9;
    /* modified BEGIN */
    goto copy_utf8_ucs1;
copy_ascii_ucs1_stop_10:
    /* modified END */
    byte_move_8(dst, src);
    byte_move_2(dst + 8, src + 8);
    src += 10;
    dst += 10;
    /* modified BEGIN */
    goto copy_utf8_ucs1;
copy_ascii_ucs1_stop_11:
    /* modified END */
    byte_move_8(dst, src);
    byte_move_4(dst + 8, src + 8);
    src += 11;
    dst += 11;
    /* modified BEGIN */
    goto copy_utf8_ucs1;
copy_ascii_ucs1_stop_12:
    /* modified END */
    byte_move_8(dst, src);
    byte_move_4(dst + 8, src + 8);
    src += 12;
    dst += 12;
    /* modified BEGIN */
    goto copy_utf8_ucs1;
copy_ascii_ucs1_stop_13:
    /* modified END */
    byte_move_8(dst, src);
    byte_move_4(dst + 8, src + 8);
    byte_move_2(dst + 12, src + 12);
    src += 13;
    dst += 13;
    /* modified BEGIN */
    goto copy_utf8_ucs1;
copy_ascii_ucs1_stop_14:
    /* modified END */
    byte_move_8(dst, src);
    byte_move_4(dst + 8, src + 8);
    byte_move_2(dst + 12, src + 12);
    src += 14;
    dst += 14;
    /* modified BEGIN */
    goto copy_utf8_ucs1;
copy_ascii_ucs1_stop_15:
    /* modified END */
    byte_move_16(dst, src);
    src += 15;
    dst += 15;
    /* modified BEGIN */
    goto copy_utf8_ucs1;
    /* modified END */
    



    /* modified BEGIN */
copy_utf8_ucs1:
    assert(cur_max_ucs_size==1);
    /* modified END */
    if (*src & 0x80) { /* non-ASCII character */
copy_utf8_inner_ucs1:
        pos = src;
        uni = byte_load_4(src);
        // TODO remove the repeat4 later
        while (true) REPEAT_CALL_4({
            if ((uni & b3_mask) == b3_patt) {
                /* modified BEGIN */
                // code point: [U+0800, U+FFFF]
                // BEGIN ucs1 -> ucs2
                assert(cur_max_ucs_size == 1);
                len_ucs1 = dst - (u8*)temp_string_buf;
                dst_ucs2 = ((u16*)temp_string_buf) + len_ucs1;
                cur_max_ucs_size = 2;
                // END ucs1 -> ucs2
                // write
                *dst_ucs2++ = read_b3_unicode(uni);
                // byte_copy_4(dst, &uni);
                // dst += 3;
                // move src and load
                src += 3;
                goto copy_utf8_inner_ucs2;
                // uni = byte_load_4(src);
                /* modified END */
            } else break;
        })
        if ((uni & b1_mask) == b1_patt) goto copy_ascii_ucs1;
        while (true) REPEAT_CALL_4({
            if ((uni & b2_mask) == b2_patt) {
                /* modified BEGIN */
                assert(cur_max_ucs_size == 1);
                u16 to_write = read_b2_unicode(uni);
                if (likely(to_write >= 0x100)) {
                    // UCS2
                    // BEGIN ucs1 -> ucs2
                    assert(cur_max_ucs_size == 1);
                    len_ucs1 = dst - (u8*)temp_string_buf;
                    dst_ucs2 = ((u16*)temp_string_buf) + len_ucs1;
                    cur_max_ucs_size = 2;
                    // END ucs1 -> ucs2
                    // write
                    *dst_ucs2++ = to_write;
                    // move src and load
                    src += 2;
                    goto copy_utf8_inner_ucs2;
                    // uni = byte_load_4(src);
                } else {
                    is_ascii = false;
                    // write
                    *dst++ = (u8)to_write;
                    // move src and load
                    src += 2;
                    // still ascii, no need goto
                    uni = byte_load_4(src);
                }
                // code point: [U+0080, U+07FF], latin1 or ucs2
                // byte_copy_2(dst, &uni);
                // dst += 2;
                /* modified END */
            } else break;
        })
        while (true) REPEAT_CALL_4({
            if ((uni & b4_mask) == b4_patt) {
                /* modified BEGIN */
                // code point: [U+10000, U+10FFFF]
                // must be ucs4
                // BEGIN ucs1 -> ucs4
                assert(cur_max_ucs_size == 1);
                len_ucs1 = dst - (u8*)temp_string_buf;
                dst_ucs4 = ((u32*)temp_string_buf) + len_ucs1;
                cur_max_ucs_size = 4;
                // END ucs1 -> ucs4
                *dst_ucs4++ = read_b4_unicode(uni);
                // byte_copy_4(dst, &uni);
                // dst += 4;
                src += 4;
                goto copy_utf8_inner_ucs4;
                // uni = byte_load_4(src);
                /* modified END */
            } else break;
        })

        /* modified BEGIN */
        if (unlikely(pos == src)) {
            return_err(src, "invalid UTF-8 encoding in string");
            // goto copy_ascii_stop_1;
        }
        goto copy_ascii_ucs1;
        /* modified END */
    }
    /* modified BEGIN */
    goto copy_escape_ucs1;
    /* modified END */
    

    /* modified BEGIN */
copy_escape_ucs2:
    assert(cur_max_ucs_size==2);
    if (likely(*src == '\\')) {
    /* modified END */
        switch (*++src) {
            case '"':  *dst_ucs2++ = '"';  src++; break;
            case '\\': *dst_ucs2++ = '\\'; src++; break;
            case '/':  *dst_ucs2++ = '/';  src++; break;
            case 'b':  *dst_ucs2++ = '\b'; src++; break;
            case 'f':  *dst_ucs2++ = '\f'; src++; break;
            case 'n':  *dst_ucs2++ = '\n'; src++; break;
            case 'r':  *dst_ucs2++ = '\r'; src++; break;
            case 't':  *dst_ucs2++ = '\t'; src++; break;
            case 'u':
                if (unlikely(!read_hex_u16(++src, &hi))) {
                    return_err(src - 2, "invalid escaped sequence in string");
                }
                src += 4;
                if (likely((hi & 0xF800) != 0xD800)) {
                    /* a BMP character */
                    /* modified BEGIN */
                    *dst_ucs2++ = hi;
                    goto copy_ascii_ucs2;
                    // if (hi >= 0x800) {
                    //     *dst++ = (u8)(0xE0 | (hi >> 12));
                    //     *dst++ = (u8)(0x80 | ((hi >> 6) & 0x3F));
                    //     *dst++ = (u8)(0x80 | (hi & 0x3F));
                    // } else if (hi >= 0x80) {
                    //     *dst++ = (u8)(0xC0 | (hi >> 6));
                    //     *dst++ = (u8)(0x80 | (hi & 0x3F));
                    // } else {
                    //     *dst++ = (u8)hi;
                    // }
                    /* modified END */
                } else {
                    /* a non-BMP character, represented as a surrogate pair */
                    if (unlikely((hi & 0xFC00) != 0xD800)) {
                        return_err(src - 6, "invalid high surrogate in string");
                    }
                    if (unlikely(!byte_match_2(src, "\\u"))) {
                        return_err(src, "no low surrogate in string");
                    }
                    if (unlikely(!read_hex_u16(src + 2, &lo))) {
                        return_err(src, "invalid escaped sequence in string");
                    }
                    if (unlikely((lo & 0xFC00) != 0xDC00)) {
                        return_err(src, "invalid low surrogate in string");
                    }
                    uni = ((((u32)hi - 0xD800) << 10) |
                            ((u32)lo - 0xDC00)) + 0x10000;
                    /* modified BEGIN */
                    // BEGIN ucs2 -> ucs4
                    assert(cur_max_ucs_size == 2);
                    len_ucs2 = dst_ucs2 - (u16*)temp_string_buf - len_ucs1;
                    dst_ucs4 = ((u32*)temp_string_buf) + len_ucs1 + len_ucs2;
                    cur_max_ucs_size = 4;
                    // END ucs2 -> ucs4
                    *dst_ucs4++ = uni;
                    // *dst++ = (u8)(0xF0 | (uni >> 18));
                    // *dst++ = (u8)(0x80 | ((uni >> 12) & 0x3F));
                    // *dst++ = (u8)(0x80 | ((uni >> 6) & 0x3F));
                    // *dst++ = (u8)(0x80 | (uni & 0x3F));
                    src += 6;
                    goto copy_ascii_ucs4;
                    /* modified END */
                }
                break;
            default: return_err(src, "invalid escaped character in string");
        }
        /* modified BEGIN */
        goto copy_ascii_ucs2;
        /* modified END */
        /* modified BEGIN */
    } else if (likely(*src == '"')) {
        goto read_finalize;
        
        /* modified END */
    } else {
        /* modified BEGIN */
        return_err(src, "unexpected control character in string");
        // if (!inv) return_err(src, "unexpected control character in string");
        // if (src >= lst) return_err(src, "unclosed string");
        // *dst++ = *src++;
        /* modified END */
    }
    assert(false);
    Py_UNREACHABLE();

    /* modified BEGIN */
copy_ascii_ucs2:
    assert(cur_max_ucs_size==2);
    while (true) REPEAT_CALL_16({
        if (unlikely(char_is_ascii_stop(*src))) break;
        *dst_ucs2++ = *src++;
    })
    /* modified END */




    /* modified BEGIN */
copy_utf8_ucs2:
    assert(cur_max_ucs_size==2);
    /* modified END */
    if (*src & 0x80) { /* non-ASCII character */
copy_utf8_inner_ucs2:
        pos = src;
        uni = byte_load_4(src);
        // TODO remove the repeat4 later
        while (true) REPEAT_CALL_4({
            if ((uni & b3_mask) == b3_patt) {
                /* modified BEGIN */
                // code point: [U+0800, U+FFFF]
                assert(cur_max_ucs_size == 2);
                // write
                *dst_ucs2++ = read_b3_unicode(uni);
                // byte_copy_4(dst, &uni);
                // dst += 3;
                // move src and load
                src += 3;
                // goto copy_utf8_ucs2;
                uni = byte_load_4(src);
                /* modified END */
            } else break;
        })
        if ((uni & b1_mask) == b1_patt) goto copy_ascii_ucs2;
        while (true) REPEAT_CALL_4({
            if ((uni & b2_mask) == b2_patt) {
                /* modified BEGIN */
                assert(cur_max_ucs_size == 2);
                u16 to_write = read_b2_unicode(uni);
                *dst_ucs2++ = to_write;
                src += 2;
                uni = byte_load_4(src);
                // if (likely(to_write >= 0x100)) {
                //     // UCS2
                //     // write
                //     *dst_ucs2++ = to_write;
                //     // move src and load
                //     src += 2;
                //     goto copy_utf8_ucs2;
                //     // uni = byte_load_4(src);
                // } else {
                //     // write
                //     *dst_ucs2++ = (u8)to_write;
                //     // move src and load
                //     src += 2;
                //     // still ascii, no need goto
                //     uni = byte_load_4(src);
                // }
                // code point: [U+0080, U+07FF], latin1 or ucs2
                // byte_copy_2(dst, &uni);
                // dst += 2;
                /* modified END */
            } else break;
        })
        while (true) REPEAT_CALL_4({
            if ((uni & b4_mask) == b4_patt) {
                /* modified BEGIN */
                // code point: [U+10000, U+10FFFF]
                // must be ucs4
                // BEGIN ucs2 -> ucs4
                assert(cur_max_ucs_size == 2);
                len_ucs2 = dst_ucs2 - (u16*)temp_string_buf - len_ucs1;
                dst_ucs4 = ((u32*)temp_string_buf) + len_ucs1 + len_ucs2;
                cur_max_ucs_size = 4;
                // END ucs2 -> ucs4
                *dst_ucs4++ = read_b4_unicode(uni);
                // byte_copy_4(dst, &uni);
                // dst += 4;
                src += 4;
                goto copy_utf8_inner_ucs4;
                // uni = byte_load_4(src);
                /* modified END */
            } else break;
        })

        /* modified BEGIN */
        if (unlikely(pos == src)) {
            return_err(src, "invalid UTF-8 encoding in string");
            // goto copy_ascii_stop_1;
        }
        goto copy_ascii_ucs2;
        /* modified END */
    }
    /* modified BEGIN */
    goto copy_escape_ucs2;
    /* modified END */
    

    /* modified BEGIN */
copy_escape_ucs4:
    assert(cur_max_ucs_size==4);
    if (likely(*src == '\\')) {
    /* modified END */
        switch (*++src) {
            case '"':  *dst_ucs4++ = '"';  src++; break;
            case '\\': *dst_ucs4++ = '\\'; src++; break;
            case '/':  *dst_ucs4++ = '/';  src++; break;
            case 'b':  *dst_ucs4++ = '\b'; src++; break;
            case 'f':  *dst_ucs4++ = '\f'; src++; break;
            case 'n':  *dst_ucs4++ = '\n'; src++; break;
            case 'r':  *dst_ucs4++ = '\r'; src++; break;
            case 't':  *dst_ucs4++ = '\t'; src++; break;
            case 'u':
                if (unlikely(!read_hex_u16(++src, &hi))) {
                    return_err(src - 2, "invalid escaped sequence in string");
                }
                src += 4;
                if (likely((hi & 0xF800) != 0xD800)) {
                    /* a BMP character */
                    /* modified BEGIN */
                    *dst_ucs4++ = hi;
                    goto copy_ascii_ucs4;
                    // if (hi >= 0x800) {
                    //     *dst++ = (u8)(0xE0 | (hi >> 12));
                    //     *dst++ = (u8)(0x80 | ((hi >> 6) & 0x3F));
                    //     *dst++ = (u8)(0x80 | (hi & 0x3F));
                    // } else if (hi >= 0x80) {
                    //     *dst++ = (u8)(0xC0 | (hi >> 6));
                    //     *dst++ = (u8)(0x80 | (hi & 0x3F));
                    // } else {
                    //     *dst++ = (u8)hi;
                    // }
                    /* modified END */
                } else {
                    /* a non-BMP character, represented as a surrogate pair */
                    if (unlikely((hi & 0xFC00) != 0xD800)) {
                        return_err(src - 6, "invalid high surrogate in string");
                    }
                    if (unlikely(!byte_match_2(src, "\\u"))) {
                        return_err(src, "no low surrogate in string");
                    }
                    if (unlikely(!read_hex_u16(src + 2, &lo))) {
                        return_err(src, "invalid escaped sequence in string");
                    }
                    if (unlikely((lo & 0xFC00) != 0xDC00)) {
                        return_err(src, "invalid low surrogate in string");
                    }
                    uni = ((((u32)hi - 0xD800) << 10) |
                            ((u32)lo - 0xDC00)) + 0x10000;
                    /* modified BEGIN */
                    
                    // END ucs2 -> ucs4
                    *dst_ucs4++ = uni;
                    // *dst++ = (u8)(0xF0 | (uni >> 18));
                    // *dst++ = (u8)(0x80 | ((uni >> 12) & 0x3F));
                    // *dst++ = (u8)(0x80 | ((uni >> 6) & 0x3F));
                    // *dst++ = (u8)(0x80 | (uni & 0x3F));
                    src += 6;
                    goto copy_ascii_ucs4;
                    /* modified END */
                }
                break;
            default: return_err(src, "invalid escaped character in string");
        }
        /* modified BEGIN */
        goto copy_ascii_ucs4;
        /* modified END */
        /* modified BEGIN */
    } else if (likely(*src == '"')) {
        goto read_finalize;
        
        /* modified END */
    } else {
        /* modified BEGIN */
        return_err(src, "unexpected control character in string");
        // if (!inv) return_err(src, "unexpected control character in string");
        // if (src >= lst) return_err(src, "unclosed string");
        // *dst++ = *src++;
        /* modified END */
    }
    assert(false);
    Py_UNREACHABLE();

    /* modified BEGIN */
copy_ascii_ucs4:
    assert(cur_max_ucs_size==4);
    while (true) REPEAT_CALL_16({
        if (unlikely(char_is_ascii_stop(*src))) break;
        *dst_ucs4++ = *src++;
    })
    /* modified END */




    /* modified BEGIN */
copy_utf8_ucs4:
    assert(cur_max_ucs_size==4);
    /* modified END */
    if (*src & 0x80) { /* non-ASCII character */
copy_utf8_inner_ucs4:
        pos = src;
        uni = byte_load_4(src);
        // TODO remove the repeat4 later
        while (true) REPEAT_CALL_4({
            if ((uni & b3_mask) == b3_patt) {
                /* modified BEGIN */
                // code point: [U+0800, U+FFFF]
                assert(cur_max_ucs_size == 4);
                // write
                *dst_ucs4++ = read_b3_unicode(uni);
                // byte_copy_4(dst, &uni);
                // dst += 3;
                // move src and load
                src += 3;
                // goto copy_utf8_ucs2;
                uni = byte_load_4(src);
                /* modified END */
            } else break;
        })
        if ((uni & b1_mask) == b1_patt) goto copy_ascii_ucs4;
        while (true) REPEAT_CALL_4({
            if ((uni & b2_mask) == b2_patt) {
                /* modified BEGIN */
                assert(cur_max_ucs_size == 4);
                *dst_ucs4++ = read_b2_unicode(uni);
                src += 2;
                uni = byte_load_4(src);
                // if (likely(to_write >= 0x100)) {
                //     // UCS2
                //     // write
                //     *dst_ucs2++ = to_write;
                //     // move src and load
                //     src += 2;
                //     goto copy_utf8_ucs2;
                //     // uni = byte_load_4(src);
                // } else {
                //     // write
                //     *dst_ucs2++ = (u8)to_write;
                //     // move src and load
                //     src += 2;
                //     // still ascii, no need goto
                //     uni = byte_load_4(src);
                // }
                // code point: [U+0080, U+07FF], latin1 or ucs2
                // byte_copy_2(dst, &uni);
                // dst += 2;
                /* modified END */
            } else break;
        })
        while (true) REPEAT_CALL_4({
            if ((uni & b4_mask) == b4_patt) {
                /* modified BEGIN */
                // code point: [U+10000, U+10FFFF]
                // must be ucs4
                *dst_ucs4++ = read_b4_unicode(uni);
                // byte_copy_4(dst, &uni);
                // dst += 4;
                src += 4;
                // goto copy_utf8_ucs4;
                uni = byte_load_4(src);
                /* modified END */
            } else break;
        })

        /* modified BEGIN */
        if (unlikely(pos == src)) {
            return_err(src, "invalid UTF-8 encoding in string");
            // goto copy_ascii_stop_1;
        }
        goto copy_ascii_ucs4;
        /* modified END */
    }
    /* modified BEGIN */
    goto copy_escape_ucs4;
    /* modified END */
    
read_finalize:
    *ptr = src + 1;
    if(unlikely(cur_max_ucs_size==4)) {
        u32* start = (u32*)temp_string_buf + len_ucs1 + len_ucs2 - 1;
        u16* ucs2_back = (u16*)temp_string_buf + len_ucs1 + len_ucs2 - 1;
        u8* ucs1_back = (u8*)temp_string_buf + len_ucs1 - 1;
        while (len_ucs2) {
            *start-- = *ucs2_back--;
            len_ucs2--;
        }
        while (len_ucs1) {
            *start-- = *ucs1_back--;
            len_ucs1--;
        }
        return pyyjson_decode_string(decode_obj_stack_info, temp_string_buf, dst_ucs4 - (u32*)temp_string_buf, PYYJSON_STRING_TYPE_UCS4, is_key);
        // pyyjson_string_op* string_op = (pyyjson_string_op*)*op;
        // PYYJSON_WRITE_OP(string_op, PYYJSON_OP_STRING | PYYJSON_STRING_FLAG_UCS4);
        // string_op->data = temp_string_buf;
        // string_op->len = dst_ucs4 - (u32*)temp_string_buf;
        // *op = (pyyjson_op*)(string_op + 1);
        // *buffer = (char*)dst_ucs4;
        // return true;//create_py_unicode(temp_string_buf, dst_ucs4 - (u32*)temp_string_buf, false, 4);
    } else if (unlikely(cur_max_ucs_size==2)) {
        u16* start = (u16*)temp_string_buf + len_ucs1 - 1;
        u8* ucs1_back = (u8*)temp_string_buf + len_ucs1 - 1;
        while (len_ucs1) {
            *start-- = *ucs1_back--;
            len_ucs1--;
        }
        return pyyjson_decode_string(decode_obj_stack_info, temp_string_buf, dst_ucs2 - (u16*)temp_string_buf, PYYJSON_STRING_TYPE_UCS2, is_key);
        // pyyjson_string_op* string_op = (pyyjson_string_op*) *op;
        // PYYJSON_WRITE_OP(string_op, PYYJSON_OP_STRING | PYYJSON_STRING_FLAG_UCS2);
        // string_op->data = temp_string_buf;
        // string_op->len = dst_ucs2 - (u16*)temp_string_buf;
        // *op = (pyyjson_op*)(string_op + 1);
        // *buffer = (char*)dst_ucs2;
        // return true;//create_py_unicode(temp_string_buf, dst_ucs2 - (u16*)temp_string_buf, false, 2);
    } else {
        return pyyjson_decode_string(decode_obj_stack_info, temp_string_buf, dst - (u8*)temp_string_buf, is_ascii?PYYJSON_STRING_TYPE_ASCII:PYYJSON_STRING_TYPE_LATIN1, is_key);
        // pyyjson_string_op* string_op = (pyyjson_string_op*) *op;
        // PYYJSON_WRITE_OP(string_op, PYYJSON_OP_STRING | (is_ascii ? PYYJSON_STRING_FLAG_ASCII : PYYJSON_STRING_FLAG_LATIN1));
        // string_op->data = temp_string_buf;
        // string_op->len = dst - (u8*)temp_string_buf;
        // *op = (pyyjson_op*)(string_op + 1);
        // *buffer = (char*)dst;
        // return true;//create_py_unicode(temp_string_buf, dst - (u8*)temp_string_buf, false, 1);
    }

#undef return_err
#undef is_valid_seq_1
#undef is_valid_seq_2
#undef is_valid_seq_3
#undef is_valid_seq_4
}



/** Read JSON document (accept all style, but optimized for pretty). */
force_inline PyObject *read_root_pretty(const char *dat, usize len) {
#define return_err(_pos, _type, _msg)                                                               \
    do {                                                                                            \
        if (_type == JSONDecodeError) {                                                             \
            PyErr_Format(JSONDecodeError, "%s, at position %zu", _msg, ((u8 *) _pos) - (u8 *) dat); \
        } else {                                                                                    \
            PyErr_SetString(_type, _msg);                                                           \
        }                                                                                           \
        goto failed_cleanup;                                                                        \
    } while (0)

    // typedef struct container_type {
    //     union {
    //         struct {
    //             Py_ssize_t size : sizeof(Py_ssize_t) * 8 - 1;
    //             Py_ssize_t tag : 1;
    //         };
    //         Py_ssize_t raw;
    //     };
    // } container_type;
    // static_assert(sizeof(container_type) == sizeof(Py_ssize_t), "size of container_type must be equal to size of Py_ssize_t");

// #define CONTAINER_ARR_TYPE (~0)
// #define CONTAINER_OBJ_TYPE 0

    // stack buffer length
// #define STACK_BUFFER_SIZE 1024
// #define COMMON_OPSIZE_RATIO (sizeof(pyyjson_string_op) / sizeof(pyyjson_op))
// #define OP_BUFFER_INIT_SIZE (STACK_BUFFER_SIZE * COMMON_OPSIZE_RATIO)
// #ifdef NDEBUG
// #define OBJ_STACK_MAX_SIZE_UPDATE(update_val)                                             \
//     do {                                                                                  \
//         object_stack_cur_size += (update_val);                                            \
//         object_stack_max_size = PYYJSON_MAX(object_stack_max_size, object_stack_cur_size); \
//     } while (0)
// #else
// #define OBJ_STACK_MAX_SIZE_UPDATE(update_val)                                             \
//     do {                                                                                  \
//         if ((update_val) < 0 && object_stack_cur_size < (usize) (-(update_val))) {        \
//             assert(false);                                                                \
//             Py_UNREACHABLE();                                                             \
//         }                                                                                 \
//         object_stack_cur_size += (update_val);                                            \
//         object_stack_max_size = PYYJSON_MAX(object_stack_max_size, object_stack_cur_size); \
//     } while (0)
// #endif
// #define OP_BUFFER_REALLOC_CHECK()        \
//     do {                                 \
//         if (new_py_operations == NULL) { \
//             PyErr_NoMemory();            \
//             goto fail_alloc;             \
//         }                                \
//     } while (0)
// #define OP_BUFFER_GROW(offset)                                                                                \
//     do {                                                                                                      \
//         if (unlikely(offset + (u8 *) cur_write_op > (u8 *) py_operations_end)) {                              \
//             size_t write_offset = cur_write_op - py_operations;                                               \
//             size_t old_capacity = py_operations_end - py_operations;                                          \
//             size_t new_capacity = old_capacity + old_capacity / 2;                                            \
//             pyyjson_op *new_py_operations;                                                                    \
//             if (likely(py_operations == pyobject_stack_buffer)) {                                             \
//                 new_py_operations = (pyyjson_op *) malloc(new_capacity * sizeof(pyyjson_op));                 \
//                 OP_BUFFER_REALLOC_CHECK();                                                                    \
//                 memcpy(new_py_operations, py_operations, old_capacity * sizeof(pyyjson_op));                  \
//             } else {                                                                                          \
//                 new_py_operations = (pyyjson_op *) realloc(py_operations, new_capacity * sizeof(pyyjson_op)); \
//                 OP_BUFFER_REALLOC_CHECK();                                                                    \
//             }                                                                                                 \
//             py_operations = new_py_operations;                                                                \
//             cur_write_op = new_py_operations + write_offset;                                                  \
//             py_operations_end = new_py_operations + new_capacity;                                             \
//         }                                                                                                     \
//     } while (0)
    // container stack info
    DecodeCtnStackInfo _decode_ctn_info;
    DecodeCtnStackInfo* decode_ctn_info = &_decode_ctn_info;
    // object stack info
    DecodeObjStackInfo _decode_obj_stack_info;
    DecodeObjStackInfo * const decode_obj_stack_info = &_decode_obj_stack_info;
    memset(decode_ctn_info, 0, sizeof(DecodeCtnStackInfo));
    memset(decode_obj_stack_info, 0, sizeof(DecodeObjStackInfo));
    // init
    if(!init_decode_ctn_stack_info(decode_ctn_info) || !init_decode_obj_stack_info(decode_obj_stack_info)) goto failed_cleanup;
    // DecodeCtnWithSize *const __stack_ctn_buffer = get_decode_obj_stack_buffer();
    // assert(__stack_ctn_buffer);
    // DecodeCtnWithSize *ctn_start = __stack_ctn_buffer;
    // // container ptr
    // DecodeCtnWithSize *const ctn_end = __stack_ctn_buffer + PYYJSON_DECODE_CONTAINER_BUFFER_INIT_SIZE;
    // DecodeCtnWithSize *ctn = ctn_start;
    // pyyjson_op pyobject_stack_buffer[OP_BUFFER_INIT_SIZE];
    // // buffer start pointer
    // pyyjson_op *py_operations;
    u8 * string_buffer_head =(u8 *)pyyjson_string_buffer;
    
    // usize required_len = PYYJSON_MAX(
    //         OP_BUFFER_INIT_SIZE,
    //         (len / PYYJSON_READER_ESTIMATED_PRETTY_RATIO) * COMMON_OPSIZE_RATIO);

    // py_operations ptr
    // pyyjson_op *py_operations_end;
    // if (unlikely(required_len > OP_BUFFER_INIT_SIZE)) {
    //     py_operations = (pyyjson_op *) malloc(required_len * sizeof(pyyjson_op));
    //     if (!py_operations) goto fail_alloc;
    //     py_operations_end = py_operations + required_len;
    // } else {
    //     py_operations = pyobject_stack_buffer;
    //     py_operations_end = pyobject_stack_buffer + OP_BUFFER_INIT_SIZE;
    // }
    // pyyjson_op *cur_write_op = py_operations;
    // usize object_stack_max_size = 0;
    // usize object_stack_cur_size = 0;

    // container buffer grow macros
// #define CTN_REALLOC_CHECK()          \
//     do {                             \
//         if (new_ctn_start == NULL) { \
//             PyErr_NoMemory();        \
//             goto fail_alloc;         \
//         }                            \
//     } while (0)
// #define CTN_BUFFER_GROW()                                                                                           \
//     do {                                                                                                            \
//         if (unlikely(ctn + 1 >= ctn_end)) {                                                                         \
//             size_t old_capacity = ctn_end - ctn_start;                                                              \
//             size_t new_capacity = old_capacity << 1;                                                                \
//             DecodeCtnWithSize *new_ctn_start;                                                                       \
//             if (likely(old_capacity == PYYJSON_DECODE_CONTAINER_BUFFER_INIT_SIZE)) {                                \
//                 new_ctn_start = (DecodeCtnWithSize *) malloc(new_capacity * sizeof(DecodeCtnWithSize));             \
//                 CTN_REALLOC_CHECK();                                                                                \
//                 memcpy(new_ctn_start, ctn_start, old_capacity * sizeof(DecodeCtnWithSize));                         \
//             } else {                                                                                                \
//                 new_ctn_start = (DecodeCtnWithSize *) realloc(ctn_start, new_capacity * sizeof(DecodeCtnWithSize)); \
//                 CTN_REALLOC_CHECK();                                                                                \
//             }                                                                                                       \
//             ctn_start = new_ctn_start;                                                                              \
//             ctn = new_ctn_start + old_capacity;                                                                     \
//             ctn_end = new_ctn_start + new_capacity;                                                                 \
//         }                                                                                                           \
//         ctn++;                                                                                                      \
//     } while (0)
// #ifdef NDEBUG
// #define CHECK_STRING_BUFFER_OVERFLOW() ((void) 0)
// #else
// #define CHECK_STRING_BUFFER_OVERFLOW()                                                          \
//     do {                                                                                        \
//         if ((((size_t) string_buffer) + 3) / 4 * 4 > ((size_t) string_buffer_head) + 4 * len) { \
//             assert(false);                                                                      \
//             Py_UNREACHABLE();                                                                   \
//         }                                                                                       \
//     } while (0)
// #endif
// #define MOVE_STRING_BUFFER_ALIGNED_4()                                     \
//     do {                                                                   \
//         string_buffer = (char *) ((((size_t) string_buffer) + 3) / 4 * 4); \
//     } while (0)
    //
    if (unlikely(len > ((size_t) (-1)) / 4)) {
        goto fail_alloc;
    }
    if (unlikely(4 * len > PYYJSON_STRING_BUFFER_SIZE)) {
        string_buffer_head = malloc(4 * len);
        if (!string_buffer_head) goto fail_alloc;
    }
    // char *string_buffer = string_buffer_head;
    //
    u8 *cur = (u8 *) dat;
    u8 *end = (u8 *) dat + len;

    // static_assert(STACK_BUFFER_SIZE > 0, "STACK_BUFFER_SIZE should be greater than 0");
    if (*cur++ == '{') {
        set_decode_ctn(decode_ctn_info->ctn, 0, false);
        // ctn->tag = CONTAINER_OBJ_TYPE;
        // ctn->size = 0;
        // ctn++;
        if (*cur == '\n') cur++;
        goto obj_key_begin;
    } else {
        set_decode_ctn(decode_ctn_info->ctn, 0, true);
        // ctn->tag = CONTAINER_ARR_TYPE;
        // ctn->size = 0;
        // ctn++;
        if (*cur == '\n') cur++;
        goto arr_val_begin;
    }

arr_begin:
    /* save current container */
    /* create a new array value, save parent container offset */
    if(unlikely(!ctn_grow_check(decode_ctn_info))) goto fail_ctn_grow;
    // ctn->tag = CONTAINER_ARR_TYPE;
    // ctn->size = 0;
    set_decode_ctn(decode_ctn_info->ctn, 0, true);


    /* push the new array value as current container */
    if (*cur == '\n') cur++;

arr_val_begin:
#if PYYJSON_IS_REAL_GCC
    while (true) REPEAT_CALL_16({
        if (byte_match_2((void *) cur, "  ")) cur += 2;
        else
            break;
    })
#else
    while (true) REPEAT_CALL_16({
        if (likely(byte_match_2(cur, "  "))) cur += 2;
        else
            break;
    })
#endif

            if (*cur == '{') {
            cur++;
            goto obj_begin;
        }
    if (*cur == '[') {
        cur++;
        goto arr_begin;
    }
    if (char_is_number(*cur)) {
        // OP_BUFFER_GROW(sizeof(pyyjson_number_op));
        // pyyjson_op *now_write_op = cur_write_op;
        if (likely(read_number(decode_obj_stack_info, &cur))) {
            // OBJ_STACK_MAX_SIZE_UPDATE(1);
            incr_decode_ctn_size(decode_ctn_info->ctn);
            goto arr_val_end;
        }
        goto fail_number;
    }
    if (*cur == '"') {
        // OP_BUFFER_GROW(sizeof(pyyjson_string_op));
        // MOVE_STRING_BUFFER_ALIGNED_4();
        if (likely(read_string(decode_obj_stack_info, &cur, string_buffer_head, false))) {
            // CHECK_STRING_BUFFER_OVERFLOW();
            // OBJ_STACK_MAX_SIZE_UPDATE(1);
            incr_decode_ctn_size(decode_ctn_info->ctn);
            goto arr_val_end;
        }
        goto fail_string;
    }
    if (*cur == 't') {
        // OP_BUFFER_GROW(sizeof(pyyjson_op));
        if (likely(_read_true(&cur) && pyyjson_decode_true(decode_obj_stack_info))) {
            // PYYJSON_WRITE_OP(cur_write_op, PYYJSON_OP_CONSTANTS | PYYJSON_CONSTANTS_FLAG_TRUE);
            // OBJ_STACK_MAX_SIZE_UPDATE(1);
            // cur_write_op++;
            incr_decode_ctn_size(decode_ctn_info->ctn);
            goto arr_val_end;
        }
        goto fail_literal_true;
    }
    if (*cur == 'f') {
        // OP_BUFFER_GROW(sizeof(pyyjson_op));
        if (likely(_read_false(&cur) && pyyjson_decode_false(decode_obj_stack_info))) {
            // PYYJSON_WRITE_OP(cur_write_op, PYYJSON_OP_CONSTANTS | PYYJSON_CONSTANTS_FLAG_FALSE);
            // OBJ_STACK_MAX_SIZE_UPDATE(1);
            // cur_write_op++;
            incr_decode_ctn_size(decode_ctn_info->ctn);
            goto arr_val_end;
        }
        goto fail_literal_false;
    }
    if (*cur == 'n') {
        // OP_BUFFER_GROW(sizeof(pyyjson_op));
        if (likely(_read_null(&cur) && pyyjson_decode_null(decode_obj_stack_info))) {
            // PYYJSON_WRITE_OP(cur_write_op, PYYJSON_OP_CONSTANTS | PYYJSON_CONSTANTS_FLAG_NULL);
            // OBJ_STACK_MAX_SIZE_UPDATE(1);
            // cur_write_op++;
            incr_decode_ctn_size(decode_ctn_info->ctn);
            goto arr_val_end;
        }
        if (likely(_read_nan(false, &cur) && pyyjson_decode_nan(decode_obj_stack_info, false))) {
            incr_decode_ctn_size(decode_ctn_info->ctn);
            goto arr_val_end;
        }
        goto fail_literal_null;
    }
    if (*cur == ']') {
        cur++;
        // if (likely(ctn_len == 0)) goto arr_end;
        if (likely(get_decode_ctn_len(decode_ctn_info->ctn) == 0)) goto arr_end;
        //if (has_read_flag(ALLOW_TRAILING_COMMAS)) goto arr_end;
        while (*cur != ',') cur--;
        goto fail_trailing_comma;
    }
    if (char_is_space(*cur)) {
        while (char_is_space(*++cur))
            ;
        goto arr_val_begin;
    }
    if ((*cur == 'i' || *cur == 'I' || *cur == 'N')) {
        // OP_BUFFER_GROW(sizeof(pyyjson_op));
        if (read_inf_or_nan(decode_obj_stack_info, false, &cur)) {
            // OBJ_STACK_MAX_SIZE_UPDATE(1);
            incr_decode_ctn_size(decode_ctn_info->ctn);
            goto arr_val_end;
        }
        goto fail_character_val;
    }

    goto fail_character_val;

arr_val_end:
    if (byte_match_2((void *) cur, ",\n")) {
        cur += 2;
        goto arr_val_begin;
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
        while (char_is_space(*++cur))
            ;
        goto arr_val_end;
    }

    goto fail_character_arr_end;

arr_end:
    assert(decode_ctn_is_arr(decode_ctn_info->ctn));
    if(!pyyjson_decode_arr(decode_obj_stack_info, get_decode_ctn_len(decode_ctn_info->ctn))) goto failed_cleanup;
    // OP_BUFFER_GROW(sizeof(pyyjson_container_op));
    // pyyjson_container_op *list_op = (pyyjson_container_op *) cur_write_op;
    // PYYJSON_WRITE_OP(list_op, PYYJSON_OP_CONTAINER | PYYJSON_CONTAINER_FLAG_ARRAY);
    {
        // Py_ssize_t cur_ctn_size = get_decode_ctn_len(ctn);
        // OBJ_STACK_MAX_SIZE_UPDATE(1 - cur_ctn_size);
        // list_op->len = cur_ctn_size;
    }
    // cur_write_op = (pyyjson_op *) (list_op + 1);
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
    // CTN_BUFFER_GROW();
    ctn_grow_check(decode_ctn_info);
    set_decode_ctn(decode_ctn_info->ctn, 0, false);
    // ctn->tag = CONTAINER_OBJ_TYPE;
    // ctn->size = 0;
    if (*cur == '\n') cur++;

obj_key_begin:
#if PYYJSON_IS_REAL_GCC
    while (true) REPEAT_CALL_16({
        if (byte_match_2((void *) cur, "  ")) cur += 2;
        else
            break;
    })
#else
    while (true) REPEAT_CALL_16({
        if (likely(byte_match_2(cur, "  "))) cur += 2;
        else
            break;
    })
#endif
        if (likely(*cur == '"')) {
            // OP_BUFFER_GROW(sizeof(pyyjson_string_op));
            // pyyjson_op *write_key_op = cur_write_op;
            // MOVE_STRING_BUFFER_ALIGNED_4();
            if (likely(read_string(decode_obj_stack_info, &cur, string_buffer_head, true))) {
                // CHECK_STRING_BUFFER_OVERFLOW();
                // OBJ_STACK_MAX_SIZE_UPDATE(1);
                // ((pyyjson_op_base *) write_key_op)->op |= PYYJSON_STRING_FLAG_OBJ_KEY;
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
        while (char_is_space(*++cur))
            ;
        goto obj_key_begin;
    }
    goto fail_character_obj_key;

obj_key_end:
    if (byte_match_2((void *) cur, ": ")) {
        cur += 2;
        goto obj_val_begin;
    }
    if (*cur == ':') {
        cur++;
        goto obj_val_begin;
    }
    if (char_is_space(*cur)) {
        while (char_is_space(*++cur))
            ;
        goto obj_key_end;
    }
    goto fail_character_obj_sep;

obj_val_begin:
    if (*cur == '"') {
        // OP_BUFFER_GROW(sizeof(pyyjson_string_op));
        // MOVE_STRING_BUFFER_ALIGNED_4();
        if (likely(read_string(decode_obj_stack_info, &cur, string_buffer_head, false))) {
            // CHECK_STRING_BUFFER_OVERFLOW();
            // OBJ_STACK_MAX_SIZE_UPDATE(1);
            incr_decode_ctn_size(decode_ctn_info->ctn);
            goto obj_val_end;
        }
        goto fail_string;
    }
    if (char_is_number(*cur)) {
        // OP_BUFFER_GROW(PYYJSON_MAX(sizeof(pyyjson_op), sizeof(pyyjson_number_op)));
        // pyyjson_op *now_write_op = cur_write_op;
        if (likely(read_number(decode_obj_stack_info, &cur))) {
            // OBJ_STACK_MAX_SIZE_UPDATE(1);
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
        // OP_BUFFER_GROW(sizeof(pyyjson_op));
        if (likely(_read_true(&cur) && pyyjson_decode_true(decode_obj_stack_info))) {
            // PYYJSON_WRITE_OP(cur_write_op, PYYJSON_OP_CONSTANTS | PYYJSON_CONSTANTS_FLAG_TRUE);
            // cur_write_op++;
            // OBJ_STACK_MAX_SIZE_UPDATE(1);
            incr_decode_ctn_size(decode_ctn_info->ctn);
            goto obj_val_end;
        }
        goto fail_literal_true;
    }
    if (*cur == 'f') {
        // OP_BUFFER_GROW(sizeof(pyyjson_op));
        if (likely(_read_false(&cur) && pyyjson_decode_false(decode_obj_stack_info))) {
            // PYYJSON_WRITE_OP(cur_write_op, PYYJSON_OP_CONSTANTS | PYYJSON_CONSTANTS_FLAG_FALSE);
            // cur_write_op++;
            // OBJ_STACK_MAX_SIZE_UPDATE(1);
            incr_decode_ctn_size(decode_ctn_info->ctn);
            goto obj_val_end;
        }
        goto fail_literal_false;
    }
    if (*cur == 'n') {
        // OP_BUFFER_GROW(sizeof(pyyjson_op));
        if (likely(_read_null(&cur) && pyyjson_decode_null(decode_obj_stack_info))) {
            // PYYJSON_WRITE_OP(cur_write_op, PYYJSON_OP_CONSTANTS | PYYJSON_CONSTANTS_FLAG_NULL);
            // cur_write_op++;
            // OBJ_STACK_MAX_SIZE_UPDATE(1);
            incr_decode_ctn_size(decode_ctn_info->ctn);
            goto obj_val_end;
        }
        if (likely(_read_nan(false, &cur) && pyyjson_decode_nan(decode_obj_stack_info, false))) {
            incr_decode_ctn_size(decode_ctn_info->ctn);
            goto obj_val_end;
        }
        goto fail_literal_null;
    }
    if (char_is_space(*cur)) {
        while (char_is_space(*++cur))
            ;
        goto obj_val_begin;
    }
    if ((*cur == 'i' || *cur == 'I' || *cur == 'N')) {
        // OP_BUFFER_GROW(sizeof(pyyjson_op));
        if (read_inf_or_nan(decode_obj_stack_info, false, &cur)) {
            incr_decode_ctn_size(decode_ctn_info->ctn);
            // OBJ_STACK_MAX_SIZE_UPDATE(1);
            goto obj_val_end;
        }
        goto fail_character_val;
    }

    goto fail_character_val;

obj_val_end:
    if (byte_match_2((void *) cur, ",\n")) {
        cur += 2;
        goto obj_key_begin;
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
        while (char_is_space(*++cur))
            ;
        goto obj_val_end;
    }

    goto fail_character_obj_end;

obj_end:
    assert(!decode_ctn_is_arr(decode_ctn_info->ctn));
    if(unlikely(!pyyjson_decode_obj(decode_obj_stack_info, get_decode_ctn_len(decode_ctn_info->ctn)))) goto failed_cleanup;
    // OP_BUFFER_GROW(sizeof(pyyjson_container_op));
    // pyyjson_container_op *dict_op = (pyyjson_container_op *) cur_write_op;
    // PYYJSON_WRITE_OP(dict_op, PYYJSON_OP_CONTAINER | PYYJSON_CONTAINER_FLAG_DICT);
    // {
    //     Py_ssize_t cur_ctn_size = get_decode_ctn_len(ctn);
    //     OBJ_STACK_MAX_SIZE_UPDATE(1 - cur_ctn_size);
    //     dict_op->len = cur_ctn_size;
    // }
    // cur_write_op = (pyyjson_op *) (dict_op + 1);
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
    if (unlikely(cur < end) // && !has_read_flag(STOP_WHEN_DONE)
    ) {
        while (char_is_space(*cur)) cur++;
        if (unlikely(cur < end)) goto fail_garbage;
    }

    assert(decode_ctn_info->ctn == decode_ctn_info->ctn_start - 1);
    // free ctn buffer before calling pyyjson_op_loads.
    // if (unlikely(decode_ctn_info->ctn_start != __stack_ctn_buffer)) {
    //     free(ctn_start);
    // }
    // add null terminate to op buffer
    // OP_BUFFER_GROW(sizeof(pyyjson_op_base));
    // PYYJSON_WRITE_OP(cur_write_op, PYYJSON_NO_OP);
    // OBJ_STACK_MAX_SIZE_UPDATE(1);
    assert(decode_obj_stack_info->cur_write_result_addr == decode_obj_stack_info->result_stack + 1);
    PyObject *obj = *decode_obj_stack_info->result_stack;
    assert(obj);
    assert(obj->ob_refcnt == 1);
    // TODO
    // obj = pyyjson_op_loads(py_operations, object_stack_max_size);
    // free py_operations
    // if (py_operations != pyobject_stack_buffer) {
    //     free(py_operations);
    // }
    // free string buffer
    if (unlikely(string_buffer_head != pyyjson_string_buffer)) {
        free(string_buffer_head);
    }
    if(unlikely(decode_obj_stack_info->result_stack_end - decode_obj_stack_info->result_stack > PYYJSON_DECODE_OBJ_BUFFER_INIT_SIZE)){
        free(decode_obj_stack_info->result_stack);
    }
    // check
    if (!obj && !PyErr_Occurred()) {
        PyErr_SetString(PyExc_RuntimeError, "Unknown error: pyyjson_op_loads() failed");
    }

    return obj;

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
    // free ctn_start
    // if (ctn_start != __stack_ctn_buffer) {
    //     free(ctn_start);
    // }
    // free py_operations
    // if (py_operations != pyobject_stack_buffer) {
    //     free(py_operations);
    // }
    // free string buffer. this need NULL check
    if (unlikely(string_buffer_head != pyyjson_string_buffer)) {
        free(string_buffer_head);
    }
    return NULL;
// #undef val_incr
#undef MOVE_STRING_BUFFER_ALIGNED_4
#undef CHECK_STRING_BUFFER_OVERFLOW
#undef CTN_BUFFER_GROW
#undef CTN_REALLOC_CHECK
#undef OP_BUFFER_GROW
#undef OP_BUFFER_REALLOC_CHECK
#undef OBJ_STACK_MAX_SIZE_UPDATE
// #undef OP_BUFFER_INIT_SIZE
// #undef COMMON_OPSIZE_RATIO
// #undef STACK_BUFFER_SIZE
// #undef CONTAINER_OBJ_TYPE
// #undef CONTAINER_ARR_TYPE
#undef return_err
}


PyObject *yyjson_read_opts(const char *dat,
                           usize len
                           //  yyjson_read_flag flg,
                           //  const yyjson_alc *alc_ptr,
                           //yyjson_read_err *err
) {

#define return_err(_pos, _type, _msg)                               \
    do {                                                            \
        if (_type == JSONDecodeError) {                             \
            PyErr_Format(JSONDecodeError, "%s at %zu", _msg, _pos); \
        } else {                                                    \
            PyErr_SetString(_type, _msg);                           \
        }                                                           \
        return NULL;                                                \
    } while (0)

    PyObject *obj;
    const char *end = dat + len;

    if (unlikely(!dat)) {
        return_err(0, JSONDecodeError, "input data is NULL");
    }
    if (unlikely(!len)) {
        return_err(0, JSONDecodeError, "input length is 0");
    }

    if (unlikely(len >= USIZE_MAX)) {
        return_err(0, PyExc_MemoryError, "memory allocation failed");
    }

    /* skip empty contents before json document */
    if (unlikely(char_is_space_or_comment(*dat))) {
        if (likely(char_is_space(*dat))) {
            while (char_is_space(*++dat))
                ;
        }
        if (unlikely(dat >= end)) {
            return_err(0, JSONDecodeError, "input data is empty");
        }
    }

    /* read json document */
    obj = read_root_pretty(dat, len);
    // TODO
    // if (likely(char_is_container(*dat))) {
    //     if (char_is_space(dat[1]) && char_is_space(dat[2])) {
    //         obj = read_root_pretty(dat, len);
    //     } else {
    //         obj = read_root_minify(dat, len);
    //     }
    // } else {
    //     obj = read_root_single(dat, len);
    // }

    /* check result */
    // if (likely(obj)) {
    //     memset(err, 0, sizeof(yyjson_read_err));
    // } else {
    //     /* RFC 8259: JSON text MUST be encoded using UTF-8 */
    //     if (err->pos == 0 && err->code != YYJSON_READ_ERROR_MEMORY_ALLOCATION) {
    //         if ((hdr[0] == 0xEF && hdr[1] == 0xBB && hdr[2] == 0xBF)) {
    //             err->msg = "byte order mark (BOM) is not supported";
    //         } else if (len >= 4 &&
    //                    ((hdr[0] == 0x00 && hdr[1] == 0x00 &&
    //                      hdr[2] == 0xFE && hdr[3] == 0xFF) ||
    //                     (hdr[0] == 0xFF && hdr[1] == 0xFE &&
    //                      hdr[2] == 0x00 && hdr[3] == 0x00))) {
    //             err->msg = "UTF-32 encoding is not supported";
    //         } else if (len >= 2 &&
    //                    ((hdr[0] == 0xFE && hdr[1] == 0xFF) ||
    //                     (hdr[0] == 0xFF && hdr[1] == 0xFE))) {
    //             err->msg = "UTF-16 encoding is not supported";
    //         }
    //     }
    //     if (!has_read_flag(INSITU)) alc.free(alc.ctx, (void *)hdr);
    // }
    return obj;

#undef return_err
}
