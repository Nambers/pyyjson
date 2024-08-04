#include "pyglue.h"
#include "yyjson.h"

#include <assert.h>
#include <math.h>
#include <stdbool.h>
#include <stddef.h>

#ifndef NDEBUG
int __count_trace[PYYJSON_OP_COUNT_MAX] = {0};
#define DEBUG_TRACE(x)      \
    do {                    \
        __count_trace[x]++; \
    } while (0)
#else
#define DEBUG_TRACE(x) (void) (0)
#endif


_Static_assert(sizeof(void *) >= 4 && (sizeof(void *) % 4) == 0, "void* should be at least 4 bytes");
_Static_assert(sizeof(size_t) >= 4 && (ALIGNOF_SIZE_T % 4) == 0, "size_t should be at least 4 bytes");


PyObject *make_string(const char *utf8_str, Py_ssize_t len, op_type flag) {
    _Static_assert(_Alignof(struct pyyjson_int_op) == 8, "pyyjson_int_op size mismatch");

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

PyObject *pyyjson_op_loads(pyyjson_op *op) {
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
            case PYYJSON_OP_NULL: {
                DEBUG_TRACE(PYYJSON_OP_NULL);
                Py_INCREF(Py_None);
                PUSH_STACK(Py_None);
                op++;
                break;
            }
            case PYYJSON_OP_TRUE: {
                DEBUG_TRACE(PYYJSON_OP_TRUE);
                Py_INCREF(Py_True);
                PUSH_STACK(Py_True);
                op++;
                break;
            }
            case PYYJSON_OP_FALSE: {
                DEBUG_TRACE(PYYJSON_OP_FALSE);
                Py_INCREF(Py_False);
                PUSH_STACK(Py_False);
                op++;
                break;
            }
            case PYYJSON_OP_INT: {
                DEBUG_TRACE(PYYJSON_OP_INT);
                pyyjson_int_op *op_int = (pyyjson_int_op *) op;
                PUSH_STACK(PyLong_FromLongLong(op_int->data));
                op_int++;
                op = (pyyjson_op *) op_int;
                break;
            }
            case PYYJSON_OP_UINT: {
                DEBUG_TRACE(PYYJSON_OP_UINT);
                pyyjson_uint_op *op_uint = (pyyjson_uint_op *) op;
                PUSH_STACK(PyLong_FromUnsignedLongLong(op_uint->data));
                op_uint++;
                op = (pyyjson_op *) op_uint;
                break;
            }
            case PYYJSON_OP_FLOAT: {
                DEBUG_TRACE(PYYJSON_OP_FLOAT);
                pyyjson_float_op *op_float = (pyyjson_float_op *) op;
                PUSH_STACK(PyFloat_FromDouble(op_float->data));
                op_float++;
                op = (pyyjson_op *) op_float;
                break;
            }
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
            case PYYJSON_OP_ARRAY_END: {
                DEBUG_TRACE(PYYJSON_OP_ARRAY_END);
                pyyjson_list_op *op_list = (pyyjson_list_op *) op;
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
            case PYYJSON_OP_OBJECT_END: {
                DEBUG_TRACE(PYYJSON_OP_OBJECT_END);
                pyyjson_dict_op *op_dict = (pyyjson_dict_op *) op;
                Py_ssize_t len = op_dict->len;
                PyObject *dict = PyDict_New();
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
                    int retcode = PyDict_SetItem(dict, key, val); // this may fail
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
                    assert(!PyUnicode_Check(val) || Py_REFCNT(val) == 1);
                }
                cur_write_result_addr -= len * 2;
                PUSH_STACK_NO_CHECK(dict);
            dict_end:
                op_dict++;
                op = (pyyjson_op *) op_dict;
                break;
            }
            case PYYJSON_OP_NAN: {
                DEBUG_TRACE(PYYJSON_OP_NAN);
                pyyjson_nan_op *op_nan = (pyyjson_nan_op *) op;
                double val = op_nan->sign ? -fabs(Py_NAN) : fabs(Py_NAN);
                PyObject *o = PyFloat_FromDouble(val);
                assert(o);
                PUSH_STACK(o);
                op_nan++;
                op = (pyyjson_op *) op_nan;
                break;
            }
            case PYYJSON_OP_INF: {
                DEBUG_TRACE(PYYJSON_OP_INF);
                pyyjson_inf_op *op_inf = (pyyjson_inf_op *) op;
                double val = op_inf->sign ? -Py_HUGE_VAL : Py_HUGE_VAL;
                PyObject *o = PyFloat_FromDouble(val);
                assert(o);
                PUSH_STACK(o);
                op_inf++;
                op = (pyyjson_op *) op_inf;
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
