#include "pyglue.h"
#include "yyjson.h"

#include <assert.h>
#include <math.h>
#include <stdbool.h>
#include <stddef.h>

#ifndef NDEBUG
Py_ssize_t max_str_len = 0;
int __count_trace[PYYJSON_OP_COUNT_MAX] = {0};
int __hash_trace[PYYJSON_KEY_CACHE_SIZE] = {0};
size_t __hash_hit_counter = 0;
size_t __hash_add_key_call_count = 0;
#define DEBUG_TRACE(x)                                                          \
    do {                                                                        \
        for (int i = 0; i < PYYJSON_OP_COUNT_MAX; i++) {                        \
            if (x & (1 << i)) {                                                 \
                __count_trace[i]++;                                             \
                /* printf("cur op: %d\n", i);                                 \
                if (x == PYYJSON_OP_STRING) {                              \
                    char buf[16] = {0};                                    \
                    const char *source = ((pyyjson_string_op *) op)->data; \
                    memcpy(buf, source, 15);                               \
                    printf("string: %s", (const char *) buf);              \
                } */ \
                break;                                                          \
            }                                                                   \
        }                                                                       \
        __op_counter++;                                                         \
    } while (0)
#define TRACE_STR_LEN(_len) max_str_len = max_str_len > _len ? max_str_len : _len
#define TRACE_HASH(_hash)        \
    __hash_add_key_call_count++; \
    __hash_trace[_hash & (PYYJSON_KEY_CACHE_SIZE - 1)]++
#define TRACE_CACHE_HIT() __hash_hit_counter++
#define TRACE_HASH_CONFLICT(_hash) printf("hash conflict: %lld, index=%lld\n", _hash, (_hash & (PYYJSON_KEY_CACHE_SIZE - 1)))
#else
#define DEBUG_TRACE(x) (void) (0)
#define TRACE_STR_LEN(_len) (void) (0)
#define TRACE_HASH(_hash) (void) (0)
#define TRACE_CACHE_HIT() (void) (0)
#define TRACE_HASH_CONFLICT(_hash) (void) (0)
#endif

static inline Py_ALWAYS_INLINE void Py_DECREF_NOCHECK(PyObject *op) {
    // Non-limited C API and limited C API for Python 3.9 and older access
    // directly PyObject.ob_refcnt.
#if PY_MINOR_VERSION >= 12
    if (_Py_IsImmortal(op)) {
        return;
    }
#endif
    _Py_DECREF_STAT_INC();
    --op->ob_refcnt;
    assert(op->ob_refcnt >= 0);
}

PyObject *AssociativeKeyCache[PYYJSON_KEY_CACHE_SIZE];

static yyjson_inline void add_key_cache(Py_hash_t hash, PyObject *obj) {
    TRACE_HASH(hash);
    assert(PyUnicode_GET_LENGTH(obj) * PyUnicode_KIND(obj) <= 64);
    // TODO
    PyObject *old = AssociativeKeyCache[hash & (PYYJSON_KEY_CACHE_SIZE - 1)];
    if (old) {
        TRACE_HASH_CONFLICT(hash);
        Py_DECREF(old);
    }
    Py_INCREF(obj);
    AssociativeKeyCache[hash & (PYYJSON_KEY_CACHE_SIZE - 1)] = obj;
    assert(((PyASCIIObject *) obj)->hash == -1);
    assert(hash != -1);
    ((PyASCIIObject *) obj)->hash = hash;
}

static yyjson_inline PyObject *get_key_cache(const char *utf8_str, Py_hash_t hash, size_t real_len) {
    assert(real_len <= 64);
    PyObject *cache = AssociativeKeyCache[hash & (PYYJSON_KEY_CACHE_SIZE - 1)];
    if (!cache || ((PyASCIIObject *) cache)->hash != hash) return NULL;
    if (yyjson_likely(((real_len == PyUnicode_GET_LENGTH(cache) * PyUnicode_KIND(cache))) && (memcmp(PyUnicode_DATA(cache), utf8_str, real_len) == 0))) {
        TRACE_CACHE_HIT();
        return cache;
    }
    return NULL;
}

static yyjson_inline PyObject *make_string(const char *utf8_str, Py_ssize_t len, op_type flag) {
    TRACE_STR_LEN(len);
    PyObject *obj;
    Py_UCS4 max_char;
    size_t real_len;
    Py_hash_t hash;

    switch (flag & PYYJSON_STRING_FLAG_UCS_TYPE_MASK) {
        case PYYJSON_STRING_FLAG_ASCII:
            max_char = 0x80;
            real_len = len;
            break;
        case PYYJSON_STRING_FLAG_LATIN1:
            max_char = 0xff;
            real_len = len;
            break;
        case PYYJSON_STRING_FLAG_UCS2:
            max_char = 0xffff;
            real_len = len * 2;
            break;
        case PYYJSON_STRING_FLAG_UCS4:
            max_char = 0x10ffff;
            real_len = len * 4;
            break;
        default:
            assert(false);
            break;
    }
    if (real_len <= 64) {
        hash = _Py_HashBytes(utf8_str, real_len);
        obj = get_key_cache(utf8_str, hash, real_len);
        if (obj) {
            Py_INCREF(obj);
            return obj;
        }
    }

    obj = PyUnicode_New(real_len, max_char);
    if (obj == NULL) return NULL;
    memcpy(PyUnicode_DATA(obj), utf8_str, real_len);
    if (flag & PYYJSON_STRING_FLAG_OBJ_KEY) {
        // TODO
        if (yyjson_likely(real_len <= 64)) {
            add_key_cache(hash, obj);
        } else {
            hash = _Py_HashBytes(utf8_str, real_len);
            ((PyASCIIObject *) obj)->hash = hash;
        }
    }
    return obj;
}

PyObject *pyyjson_op_loads(pyyjson_op *op_head) {
#ifndef NDEBUG
    size_t __op_counter = 0;
#define SHOW_OP_TRACE()                                     \
    do {                                                    \
        for (int i = 0; i < PYYJSON_OP_COUNT_MAX; i++) {    \
            if (__count_trace[i] > 0) {                     \
                printf("op %d: %d\n", i, __count_trace[i]); \
            }                                               \
        }                                                   \
        printf("total op: %lld\n", __op_counter);           \
    } while (0)
#define SHOW_STR_TRACE() printf("max str len: %lld\n", max_str_len)
#else
#define SHOW_OP_TRACE() (void) (0)
#define SHOW_STR_TRACE() (void) (0)
#endif
    pyyjson_op *op = op_head;
#define STACK_BUFFER_SIZE 1024
    PyObject *__stack_buffer[STACK_BUFFER_SIZE];
    //
    PyObject **result_stack;
    result_stack = __stack_buffer;
    PyObject **cur_write_result_addr = result_stack;
    PyObject **result_stack_end = result_stack + STACK_BUFFER_SIZE;

#define RESULT_STACK_REALLOC_CHECK()    \
    do {                                \
        if (new_result_stack == NULL) { \
            PyErr_NoMemory();           \
            goto fail;                  \
        }                               \
    } while (0)

#define RESULT_STACK_GROW()                                                                                \
    do {                                                                                                   \
        if (yyjson_unlikely(cur_write_result_addr >= result_stack_end)) {                                  \
            size_t old_capacity = result_stack_end - result_stack;                                         \
            size_t new_capacity = old_capacity + old_capacity / 2;                                         \
            PyObject **new_result_stack;                                                                   \
            if (yyjson_likely(result_stack == __stack_buffer)) {                                           \
                new_result_stack = (PyObject **) malloc(new_capacity * sizeof(PyObject *));                \
                RESULT_STACK_REALLOC_CHECK();                                                              \
                memcpy(new_result_stack, result_stack, old_capacity * sizeof(PyObject *));                 \
            } else {                                                                                       \
                new_result_stack = (PyObject **) realloc(result_stack, new_capacity * sizeof(PyObject *)); \
                RESULT_STACK_REALLOC_CHECK();                                                              \
            }                                                                                              \
            result_stack = new_result_stack;                                                               \
            cur_write_result_addr = new_result_stack + old_capacity;                                       \
            result_stack_end = new_result_stack + new_capacity;                                            \
        }                                                                                                  \
    } while (0)

#define PUSH_STACK_NO_CHECK(obj)      \
    do {                              \
        *cur_write_result_addr = obj; \
        cur_write_result_addr++;      \
    } while (0)

#define PUSH_STACK(obj)           \
    do {                          \
        RESULT_STACK_GROW();      \
        PUSH_STACK_NO_CHECK(obj); \
    } while (0)

#define POP_STACK_PRE_CHECK(size)                                                          \
    do {                                                                                   \
        if (yyjson_unlikely(size > (Py_ssize_t) (cur_write_result_addr - result_stack))) { \
            PyErr_Format(PyExc_RuntimeError,                                               \
                         "Stack underflow: expected to have at least %lld, got %lld",      \
                         size, (Py_ssize_t) (cur_write_result_addr - result_stack));       \
            goto fail;                                                                     \
        }                                                                                  \
    } while (0)

    while (1) {
        switch (PYYJSON_READ_OP(op) & PYYJSON_OP_MASK) {
            case PYYJSON_OP_STRING: {
                DEBUG_TRACE(PYYJSON_OP_STRING);
                pyyjson_string_op *op_str = (pyyjson_string_op *) op;
                PyObject *new_val = make_string(op_str->data, op_str->len, PYYJSON_READ_OP(op));
                if (new_val == NULL) goto fail;
                PUSH_STACK(new_val);
                op_str++;
                op = (pyyjson_op *) op_str;
                break;
            }
            case PYYJSON_OP_NUMBER: {
                DEBUG_TRACE(PYYJSON_OP_NUMBER);
                pyyjson_number_op *op_num = (pyyjson_number_op *) op;
                switch (PYYJSON_READ_OP(op) & PYYJSON_NUM_FLAG_MASK) {
                    case PYYJSON_NUM_FLAG_FLOAT: {
                        PUSH_STACK(PyFloat_FromDouble(op_num->data.f));
                        break;
                    }
                    case PYYJSON_NUM_FLAG_INT: {
                        PUSH_STACK(PyLong_FromLongLong(op_num->data.i));
                        break;
                    }
                    case PYYJSON_NUM_FLAG_UINT: {
                        PUSH_STACK(PyLong_FromUnsignedLongLong(op_num->data.u));
                        break;
                    }
                    default:
                        assert(false);
                }
                op_num++;
                op = (pyyjson_op *) op_num;
                break;
            }
            case PYYJSON_OP_CONTAINER: {
                DEBUG_TRACE(PYYJSON_OP_CONTAINER);
                switch (PYYJSON_READ_OP(op) & PYYJSON_CONTAINER_FLAG_MASK) {
                    case PYYJSON_CONTAINER_FLAG_ARRAY: {
                        goto container_array;
                    }
                    case PYYJSON_CONTAINER_FLAG_DICT: {
                        goto container_dict;
                    }
                    default:
                        assert(false);
                }
                assert(false);
                {
                container_array:
                    pyyjson_container_op *op_list = (pyyjson_container_op *) op;
                    Py_ssize_t len = op_list->len;
                    PyObject *list = PyList_New(len);
                    if (list == NULL) goto fail;
                    if (yyjson_unlikely(len == 0)) {
                        PUSH_STACK(list);
                        goto arr_end;
                    }
                    POP_STACK_PRE_CHECK(len - 1);
                    PyObject **list_val_start = cur_write_result_addr - len;
                    for (size_t j = 0; j < len; j++) {
                        PyObject *val = list_val_start[j];
                        PyList_SET_ITEM(list, j, val); // this do not fail
                        assert(!PyUnicode_Check(val) || Py_REFCNT(val) == 1);
                    }
                    cur_write_result_addr -= len;
                    PUSH_STACK_NO_CHECK(list);
                arr_end:
                    op_list++;
                    op = (pyyjson_op *) op_list;
                    break;
                }
                {
                container_dict:
                    pyyjson_container_op *op_dict = (pyyjson_container_op *) op;
                    Py_ssize_t len = op_dict->len;
                    PyObject *dict = _PyDict_NewPresized(len);
                    if (dict == NULL) goto fail;
                    if (yyjson_unlikely(len == 0)) {
                        PUSH_STACK(dict);
                        goto dict_end;
                    }
                    POP_STACK_PRE_CHECK(len * 2 - 1);
                    PyObject **dict_val_start = cur_write_result_addr - len * 2;
                    for (size_t j = 0; j < len; j++) {
                        PyObject *key = dict_val_start[j * 2];
                        assert(PyUnicode_Check(key));
                        PyObject *val = dict_val_start[j * 2 + 1];
                        assert(((PyASCIIObject *) key)->hash != -1);
                        int retcode = _PyDict_SetItem_KnownHash(dict, key, val, ((PyASCIIObject *) key)->hash); // this may fail
                        if (yyjson_likely(0 == retcode)) {
                            Py_DECREF_NOCHECK(key);
                            Py_DECREF_NOCHECK(val);
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
                        // assert(!PyUnicode_Check(val) || Py_REFCNT(val) == 1);
                    }
                    cur_write_result_addr -= len * 2;
                    PUSH_STACK_NO_CHECK(dict);
                dict_end:
                    op_dict++;
                    op = (pyyjson_op *) op_dict;
                    break;
                }
            }
            case PYYJSON_OP_CONSTANTS: {
                DEBUG_TRACE(PYYJSON_OP_CONSTANTS);
                switch (PYYJSON_READ_OP(op) & PYYJSON_CONSTANTS_FLAG_MASK) {
                    case PYYJSON_CONSTANTS_FLAG_NULL:
                        Py_INCREF(Py_None);
                        PUSH_STACK(Py_None);
                        op++;
                        break;
                    case PYYJSON_CONSTANTS_FLAG_FALSE:
                        Py_INCREF(Py_False);
                        PUSH_STACK(Py_False);
                        op++;
                        break;
                    case PYYJSON_CONSTANTS_FLAG_TRUE:
                        Py_INCREF(Py_True);
                        PUSH_STACK(Py_True);
                        op++;
                        break;
                    default:
                        assert(false);
                        break;
                }
                break;
            }
            case PYYJSON_OP_NAN_INF: {
                DEBUG_TRACE(PYYJSON_OP_NAN_INF);
                switch (PYYJSON_READ_OP(op) & PYYJSON_NAN_INF_FLAG_MASK_WITHOUT_SIGNED) {
                    case PYYJSON_NAN_INF_FLAG_NAN: {
                        bool sign = (PYYJSON_READ_OP(op) & PYYJSON_NAN_INF_FLAG_SIGNED) != 0;
                        double val = sign ? -fabs(Py_NAN) : fabs(Py_NAN);
                        PyObject *o = PyFloat_FromDouble(val);
                        assert(o);
                        PUSH_STACK(o);
                        op++;
                        break;
                    }
                    case PYYJSON_NAN_INF_FLAG_INF: {
                        bool sign = (PYYJSON_READ_OP(op) & PYYJSON_NAN_INF_FLAG_SIGNED) != 0;
                        double val = sign ? -Py_HUGE_VAL : Py_HUGE_VAL;
                        PyObject *o = PyFloat_FromDouble(val);
                        assert(o);
                        PUSH_STACK(o);
                        op++;
                        break;
                    }
                    default:
                        assert(false);
                        break;
                }
                break;
            }
            case PYYJSON_NO_OP: {
                goto success;
            }
            default:
                assert(false);
        }
    }
success:
    assert(cur_write_result_addr - result_stack == 1);
    PyObject *result = *result_stack;
    if (yyjson_unlikely(result_stack != __stack_buffer)) free(result_stack);
    SHOW_OP_TRACE();
    SHOW_STR_TRACE();
    return result;
fail:
    // decref all objects in the stack
    for (PyObject **p = result_stack; p < cur_write_result_addr; p++) {
        Py_DECREF(*p);
    }
    if (yyjson_unlikely(result_stack != __stack_buffer)) free(result_stack);
    if (PyErr_Occurred() == NULL) {
        PyErr_Format(PyExc_RuntimeError, "Analyze pyyjson opcode failed at %lld", cur_write_result_addr - result_stack);
    }
    return NULL;
}

char pyyjson_string_buffer[PYYJSON_STRING_BUFFER_SIZE];
