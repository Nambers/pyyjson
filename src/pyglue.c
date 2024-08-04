#include "pyglue.h"
#include "yyjson.h"

#include <assert.h>
#include <math.h>
#include <stdbool.h>
#include <stddef.h>

#ifndef NDEBUG
int __count_trace[PYYJSON_OP_COUNT_MAX] = {0};
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
#else
#define DEBUG_TRACE(x) (void) (0)
#endif


static yyjson_inline PyObject *make_string(const char *utf8_str, Py_ssize_t len, op_type flag) {
    PyObject *obj;
    switch (flag & PYYJSON_STRING_FLAG_MASK) {
        case PYYJSON_STRING_FLAG_ASCII:
            obj = PyUnicode_New(len, 0x80);
            if (obj != NULL) memcpy(PyUnicode_DATA(obj), utf8_str, len);
            break;
        case PYYJSON_STRING_FLAG_LATIN1:
            obj = PyUnicode_New(len, 0xff);
            if (obj != NULL) memcpy(PyUnicode_DATA(obj), utf8_str, len);
            break;
        case PYYJSON_STRING_FLAG_UCS2:
            obj = PyUnicode_New(len, 0xffff);
            if (obj != NULL) memcpy(PyUnicode_DATA(obj), utf8_str, len * 2);
            break;
        case PYYJSON_STRING_FLAG_UCS4:
            obj = PyUnicode_New(len, 0x10ffff);
            if (obj != NULL) memcpy(PyUnicode_DATA(obj), utf8_str, len * 4);
            break;
        default:
            assert(false);
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
#else
#define SHOW_OP_TRACE() (void) (0)
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
                        assert(((PyASCIIObject *) key)->hash == -1);
                        ((PyASCIIObject *) key)->hash = _Py_HashBytes(PyUnicode_DATA(key), PyUnicode_GET_LENGTH(key) * PyUnicode_KIND(key));
                        // Py_hash_t hash = PyUnicode_Type.tp_hash(key);
                        assert(((PyASCIIObject *) key)->hash != -1);
                        int retcode = _PyDict_SetItem_KnownHash(dict, key, val, ((PyASCIIObject *) key)->hash); // this may fail
                        // int retcode = PyDict_SetItem(dict, key, val); // this may fail
                        if (yyjson_likely(0 == retcode)) {
                            Py_DECREF(key);
                            Py_DECREF(val);
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
                switch (PYYJSON_READ_OP(op) & PYYJSON_NAN_INF_FLAG_MASK) {
                    case PYYJSON_NAN_INF_FLAG_NAN: {
                        pyyjson_nan_inf_op *op_nan = (pyyjson_nan_inf_op *) op;
                        double val = op_nan->sign ? -fabs(Py_NAN) : fabs(Py_NAN);
                        PyObject *o = PyFloat_FromDouble(val);
                        assert(o);
                        PUSH_STACK(o);
                        op_nan++;
                        op = (pyyjson_op *) op_nan;
                        break;
                    }
                    case PYYJSON_NAN_INF_FLAG_INF: {
                        pyyjson_nan_inf_op *op_inf = (pyyjson_nan_inf_op *) op;
                        double val = op_inf->sign ? -Py_HUGE_VAL : Py_HUGE_VAL;
                        PyObject *o = PyFloat_FromDouble(val);
                        assert(o);
                        PUSH_STACK(o);
                        op_inf++;
                        op = (pyyjson_op *) op_inf;
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
