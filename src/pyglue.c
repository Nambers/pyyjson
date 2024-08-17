#include "pyglue.h"
#include "yyjson.h"
#define XXH_INLINE_ALL
#include "xxhash.h"
#include <assert.h>
#include <math.h>
#include <stdbool.h>
#include <stddef.h>
#include <threads.h>

#if PY_MINOR_VERSION >= 13
// these are hidden in Python 3.13
PyAPI_FUNC(Py_hash_t) _Py_HashBytes(const void *, Py_ssize_t);
PyAPI_FUNC(int) _PyDict_SetItem_KnownHash(PyObject *mp, PyObject *key, PyObject *item, Py_hash_t hash);
#endif

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
#define REHASHER(_x, _size) (((size_t) (_x)) % (_size))

typedef XXH64_hash_t pyyjson_hash_t;


thread_local PyObject *pyobject_stack_buffer[PYYJSON_OBJSTACK_BUFFER_SIZE];


#if PYYJSON_ENABLE_TRACE
Py_ssize_t max_str_len = 0;
int __count_trace[PYYJSON_OP_COUNT_MAX] = {0};
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

static yyjson_inline void Py_DecRef_NoCheck(PyObject *op) {
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

static yyjson_inline void Py_Immortal_IncRef(PyObject *op) {
    // Non-limited C API and limited C API for Python 3.9 and older access
    // directly PyObject.ob_refcnt.
#if PY_MINOR_VERSION >= 12
#if SIZEOF_VOID_P > 4
    // Portable saturated add, branching on the carry flag and set low bits
#ifndef NDEBUG
    PY_UINT32_T cur_refcnt = op->ob_refcnt_split[PY_BIG_ENDIAN];
    PY_UINT32_T new_refcnt = cur_refcnt + 1;
    assert(new_refcnt == 0);
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

static yyjson_inline void add_key_cache(pyyjson_hash_t hash, PyObject *obj) {
    assert(PyUnicode_GET_LENGTH(obj) * PyUnicode_KIND(obj) <= 64);
    size_t index = REHASHER(hash, PYYJSON_KEY_CACHE_SIZE);
    PYYJSON_TRACE_HASH(index);
    pyyjson_cache_type old = AssociativeKeyCache[index];
    if (old) {
        PYYJSON_TRACE_HASH_CONFLICT(hash);
        Py_DECREF(old);
    }
    Py_INCREF(obj);
    AssociativeKeyCache[index] = obj;
}

static yyjson_inline PyObject *get_key_cache(const char *utf8_str, pyyjson_hash_t hash, size_t real_len) {
    assert(real_len <= 64);
    pyyjson_cache_type cache = AssociativeKeyCache[REHASHER(hash, PYYJSON_KEY_CACHE_SIZE)];
    if (!cache) return NULL;
    if (yyjson_likely(((real_len == PyUnicode_GET_LENGTH(cache) * PyUnicode_KIND(cache))) && (memcmp(PyUnicode_DATA(cache), utf8_str, real_len) == 0))) {
        PYYJSON_TRACE_CACHE_HIT();
        return cache;
    }
    return NULL;
}

#define CALC_MAX_CHAR_AND_REAL_LEN()                    \
    switch (flag & PYYJSON_STRING_FLAG_UCS_TYPE_MASK) { \
        case PYYJSON_STRING_FLAG_ASCII:                 \
            max_char = 0x80;                            \
            real_len = len;                             \
            break;                                      \
        case PYYJSON_STRING_FLAG_LATIN1:                \
            max_char = 0xff;                            \
            real_len = len;                             \
            break;                                      \
        case PYYJSON_STRING_FLAG_UCS2:                  \
            max_char = 0xffff;                          \
            real_len = len * 2;                         \
            break;                                      \
        case PYYJSON_STRING_FLAG_UCS4:                  \
            max_char = 0x10ffff;                        \
            real_len = len * 4;                         \
            break;                                      \
        default:                                        \
            assert(false);                              \
            Py_UNREACHABLE();                           \
    }


static yyjson_inline PyObject *make_string(const char *utf8_str, Py_ssize_t len, op_type flag) {
    PYYJSON_TRACE_STR_LEN(len);
    PyObject *obj;
    Py_UCS4 max_char;
    size_t real_len;
    XXH64_hash_t hash;

    CALC_MAX_CHAR_AND_REAL_LEN();

    bool should_cache = ((flag & PYYJSON_STRING_FLAG_OBJ_KEY) && yyjson_likely(real_len <= 64));

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
    if (flag & PYYJSON_STRING_FLAG_OBJ_KEY) {
        assert(((PyASCIIObject *) obj)->hash == -1);
        ((PyASCIIObject *) obj)->hash = _Py_HashBytes(utf8_str, real_len);
    }
    return obj;
}

PyObject *pyyjson_op_loads(pyyjson_op *op_head, size_t obj_stack_maxsize) {
#if PYYJSON_ENABLE_TRACE
    size_t __op_counter = 0;
#define PYYJSON_TRACE_OP(x)                              \
    do {                                                 \
        for (int i = 0; i < PYYJSON_OP_COUNT_MAX; i++) { \
            if (x & (1 << i)) {                          \
                __count_trace[i]++;                      \
                break;                                   \
            }                                            \
        }                                                \
        __op_counter++;                                  \
    } while (0)
#define PYYJSON_SHOW_OP_TRACE()                                   \
    do {                                                          \
        for (int i = 0; i < PYYJSON_OP_COUNT_MAX; i++) {          \
            if (__count_trace[i] > 0) {                           \
                printf("op %d: %d\n", i, __count_trace[i]);       \
            }                                                     \
        }                                                         \
        printf("total op: %lld\n", (long long int) __op_counter); \
    } while (0)

#define PYYJSON_SHOW_STR_TRACE() printf("max str len: %lld\n", (long long int) max_str_len)
#else // PYYJSON_ENABLE_TRACE
#define PYYJSON_TRACE_OP(x) (void) (0)
#define PYYJSON_SHOW_OP_TRACE() (void) (0)
#define PYYJSON_SHOW_STR_TRACE() (void) (0)
#endif // PYYJSON_ENABLE_TRACE
    pyyjson_op *op = op_head;
    PyObject **result_stack;
    result_stack = pyobject_stack_buffer;
    if (yyjson_unlikely(obj_stack_maxsize > PYYJSON_OBJSTACK_BUFFER_SIZE)) {
        result_stack = (PyObject **) malloc(obj_stack_maxsize * sizeof(PyObject *));
        if (yyjson_unlikely(result_stack == NULL)) {
            PyErr_NoMemory();
            return NULL;
        }
    }
    PyObject **cur_write_result_addr = result_stack;
#ifndef NDEBUG
    PyObject **result_stack_end = result_stack + obj_stack_maxsize;
#endif
    //
    pyyjson_container_op *op_container;

#define PYYJSON_RESULT_STACK_REALLOC_CHECK() \
    do {                                     \
        if (new_result_stack == NULL) {      \
            PyErr_NoMemory();                \
            goto fail;                       \
        }                                    \
    } while (0)

#ifndef NDEBUG
#define PYYJSON_RESULT_STACK_GROW()                                   \
    if (yyjson_unlikely(cur_write_result_addr >= result_stack_end)) { \
        assert(false);                                                \
        Py_UNREACHABLE();                                             \
    }
#else
#define PYYJSON_RESULT_STACK_GROW() (void) (0)
#endif
#define PUSH_STACK_NO_CHECK(obj)      \
    do {                              \
        *cur_write_result_addr = obj; \
        cur_write_result_addr++;      \
    } while (0)

#define PYYJSON_PUSH_STACK(obj)      \
    do {                             \
        PYYJSON_RESULT_STACK_GROW(); \
        PUSH_STACK_NO_CHECK(obj);    \
    } while (0)
#ifndef NDEBUG
#define PYYJSON_POP_STACK_PRE_CHECK(size)                                                    \
    do {                                                                                     \
        if (yyjson_unlikely((size) > (Py_ssize_t) (cur_write_result_addr - result_stack))) { \
            PyErr_Format(PyExc_RuntimeError,                                                 \
                         "Stack underflow: expected to have at least %lld, got %lld",        \
                         (size), (Py_ssize_t) (cur_write_result_addr - result_stack));       \
            goto fail;                                                                       \
        }                                                                                    \
    } while (0)
#else // NDEBUG
#define PYYJSON_POP_STACK_PRE_CHECK(size) (void) (0)
#endif // NDEBUG
    while (1) {
        switch (PYYJSON_READ_OP(op) & PYYJSON_OP_MASK) {
            case PYYJSON_OP_STRING: {
                PYYJSON_TRACE_OP(PYYJSON_OP_STRING);
                pyyjson_string_op *op_str = (pyyjson_string_op *) op;
                PyObject *new_val = make_string(op_str->data, op_str->len, PYYJSON_READ_OP(op));
                if (new_val == NULL) goto fail;
                PYYJSON_PUSH_STACK(new_val);
                op_str++;
                op = (pyyjson_op *) op_str;
                break;
            }
            case PYYJSON_OP_NUMBER: {
                PYYJSON_TRACE_OP(PYYJSON_OP_NUMBER);
                pyyjson_number_op *op_num = (pyyjson_number_op *) op;
                switch (PYYJSON_READ_OP(op) & PYYJSON_NUM_FLAG_MASK) {
                    case PYYJSON_NUM_FLAG_FLOAT: {
                        PYYJSON_PUSH_STACK(PyFloat_FromDouble(op_num->data.f));
                        break;
                    }
                    case PYYJSON_NUM_FLAG_INT: {
                        PYYJSON_PUSH_STACK(PyLong_FromLongLong(op_num->data.i));
                        break;
                    }
                    case PYYJSON_NUM_FLAG_UINT: {
                        PYYJSON_PUSH_STACK(PyLong_FromUnsignedLongLong(op_num->data.u));
                        break;
                    }
                    default:
                        assert(false);
                        Py_UNREACHABLE();
                }
                op_num++;
                op = (pyyjson_op *) op_num;
                break;
            }
            case PYYJSON_OP_CONTAINER: {
                PYYJSON_TRACE_OP(PYYJSON_OP_CONTAINER);
                switch (PYYJSON_READ_OP(op) & PYYJSON_CONTAINER_FLAG_MASK) {
                    case PYYJSON_CONTAINER_FLAG_ARRAY: {
                        goto container_array;
                    }
                    case PYYJSON_CONTAINER_FLAG_DICT: {
                        goto container_dict;
                    }
                    default:
                        assert(false);
                        Py_UNREACHABLE();
                }
                assert(false);
                Py_UNREACHABLE();
                {
                container_array:
                    op_container = (pyyjson_container_op *) op;
                    Py_ssize_t len = op_container->len;
                    PyObject *list = PyList_New(len);
                    if (yyjson_unlikely(list == NULL)) goto fail;
                    if (yyjson_unlikely(len == 0)) {
                        PYYJSON_PUSH_STACK(list);
                        goto arr_end;
                    }
                    PYYJSON_POP_STACK_PRE_CHECK(len - 1);
                    PyObject **list_val_start = cur_write_result_addr - len;
                    for (size_t j = 0; j < len; j++) {
                        PyObject *val = list_val_start[j];
                        PyList_SET_ITEM(list, j, val); // this never fails
                    }
                    cur_write_result_addr -= len;
                    PUSH_STACK_NO_CHECK(list);
                arr_end:
                    op_container++;
                    op = (pyyjson_op *) op_container;
                    break;
                }
                {
                container_dict:
                    op_container = (pyyjson_container_op *) op;
                    Py_ssize_t len = op_container->len;
                    PyObject *dict = _PyDict_NewPresized(len);
                    if (yyjson_unlikely(dict == NULL)) goto fail;
                    if (yyjson_unlikely(len == 0)) {
                        PYYJSON_PUSH_STACK(dict);
                        goto dict_end;
                    }
                    PYYJSON_POP_STACK_PRE_CHECK(len * 2 - 1);
                    PyObject **dict_val_start = cur_write_result_addr - len * 2;
                    for (size_t j = 0; j < len; j++) {
                        PyObject *key = dict_val_start[j * 2];
                        assert(PyUnicode_Check(key));
                        PyObject *val = dict_val_start[j * 2 + 1];
                        assert(((PyASCIIObject *) key)->hash != -1);
                        int retcode = _PyDict_SetItem_KnownHash(dict, key, val, ((PyASCIIObject *) key)->hash); // this may fail
                        if (yyjson_likely(0 == retcode)) {
                            Py_DecRef_NoCheck(key);
                            Py_DecRef_NoCheck(val);
                        } else {
                            Py_DECREF(dict);
                            // also need to clean up the rest k-v pairs
                            for (size_t k = j * 2; k < len * 2; k++) {
                                Py_DECREF(dict_val_start[k]);
                            }
                            // move cur_write_result_addr to the first key addr, avoid double decref
                            cur_write_result_addr = dict_val_start;
                            goto fail;
                        }
                    }
                    cur_write_result_addr -= len * 2;
                    PUSH_STACK_NO_CHECK(dict);
                dict_end:
                    op_container++;
                    op = (pyyjson_op *) op_container;
                    break;
                }
            }
            case PYYJSON_OP_CONSTANTS: {
                PYYJSON_TRACE_OP(PYYJSON_OP_CONSTANTS);
                switch (PYYJSON_READ_OP(op) & PYYJSON_CONSTANTS_FLAG_MASK) {
                    case PYYJSON_CONSTANTS_FLAG_NULL:
                        Py_Immortal_IncRef(Py_None);
                        PYYJSON_PUSH_STACK(Py_None);
                        op++;
                        break;
                    case PYYJSON_CONSTANTS_FLAG_FALSE:
                        Py_Immortal_IncRef(Py_False);
                        PYYJSON_PUSH_STACK(Py_False);
                        op++;
                        break;
                    case PYYJSON_CONSTANTS_FLAG_TRUE:
                        Py_Immortal_IncRef(Py_True);
                        PYYJSON_PUSH_STACK(Py_True);
                        op++;
                        break;
                    default:
                        assert(false);
                        Py_UNREACHABLE();
                        break;
                }
                break;
            }
            case PYYJSON_OP_NAN_INF: {
                PYYJSON_TRACE_OP(PYYJSON_OP_NAN_INF);
                switch (PYYJSON_READ_OP(op) & PYYJSON_NAN_INF_FLAG_MASK_WITHOUT_SIGNED) {
                    case PYYJSON_NAN_INF_FLAG_NAN: {
                        bool sign = (PYYJSON_READ_OP(op) & PYYJSON_NAN_INF_FLAG_SIGNED) != 0;
                        double val = sign ? -fabs(Py_NAN) : fabs(Py_NAN);
                        PyObject *o = PyFloat_FromDouble(val);
                        assert(o);
                        PYYJSON_PUSH_STACK(o);
                        op++;
                        break;
                    }
                    case PYYJSON_NAN_INF_FLAG_INF: {
                        bool sign = (PYYJSON_READ_OP(op) & PYYJSON_NAN_INF_FLAG_SIGNED) != 0;
                        double val = sign ? -Py_HUGE_VAL : Py_HUGE_VAL;
                        PyObject *o = PyFloat_FromDouble(val);
                        assert(o);
                        PYYJSON_PUSH_STACK(o);
                        op++;
                        break;
                    }
                    default:
                        assert(false);
                        Py_UNREACHABLE();
                        break;
                }
                break;
            }
            case PYYJSON_NO_OP: {
                goto success;
            }
            default:
                assert(false);
                Py_UNREACHABLE();
        }
    }
success:
    assert(cur_write_result_addr - result_stack == 1);
    PyObject *result = *result_stack;
    if (yyjson_unlikely(result_stack != pyobject_stack_buffer)) free(result_stack);
    PYYJSON_SHOW_OP_TRACE();
    PYYJSON_SHOW_STR_TRACE();
    return result;
fail:
    // decref all objects in the stack
    for (PyObject **p = result_stack; p < cur_write_result_addr; p++) {
        Py_DECREF(*p);
    }
    if (yyjson_unlikely(result_stack != pyobject_stack_buffer)) free(result_stack);
    if (PyErr_Occurred() == NULL) {
        PyErr_Format(PyExc_RuntimeError, "Analyze pyyjson opcode failed at %lld", cur_write_result_addr - result_stack);
    }
    return NULL;
}
