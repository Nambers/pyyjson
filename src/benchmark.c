#include "pyyjson.h"

#if defined(_WIN32) || defined(_WIN64)
#    include <windows.h>

typedef unsigned long long usize;

LARGE_INTEGER _get_frequency(void) {
    LARGE_INTEGER frequency;
    QueryPerformanceFrequency(&frequency);
    return frequency;
}

usize perf_counter(void) {
    static LARGE_INTEGER *frequency = NULL;
    if (!frequency) {
        frequency = (LARGE_INTEGER *)malloc(sizeof(LARGE_INTEGER));
        if (!frequency) {
            return 0;
        }
        *frequency = _get_frequency();
    }
    LARGE_INTEGER counter;
    QueryPerformanceCounter(&counter);
    return (usize)((counter.QuadPart * 1000000000LL) / frequency->QuadPart);
}

#else
#    include <time.h>

usize perf_counter(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (usize)ts.tv_sec * 1000000000LL + (usize)ts.tv_nsec;
}

#endif

typedef struct PyUnicodeNewCallArg {
    Py_ssize_t size;
    int kind;
    Py_UCS4 max_char;
    bool valid;
} PyUnicodeNewCallArg;

PyObject *_copy_unicode(PyObject *unicode, PyUnicodeNewCallArg *call_arg) {
    if (!call_arg->valid) {
        // create copy of unicode object.
        int kind = PyUnicode_KIND(unicode);
        Py_UCS4 max_char;
        if (kind == 4) {
            max_char = 0x10ffff;
        } else if (kind == 2) {
            max_char = 0xffff;
        } else if (PyUnicode_IS_ASCII(unicode)) {
            max_char = 0x7f;
        } else {
            max_char = 0xff;
        }
        //
        call_arg->size = PyUnicode_GET_LENGTH(unicode);
        call_arg->kind = kind;
        call_arg->max_char = max_char;
        call_arg->valid = true;
    }

    PyObject *unicode_copy = PyUnicode_New(call_arg->size, call_arg->max_char);
    memcpy(PyUnicode_DATA(unicode_copy), PyUnicode_DATA(unicode), call_arg->size * call_arg->kind);
    return unicode_copy;
}

PyObject *_parse_additional_args(PyObject *additional_args) {
    Py_ssize_t new_args_count = 1;
    if (additional_args) {
        new_args_count += PyTuple_GET_SIZE(additional_args);
    }
    PyObject *new_args = PyTuple_New(new_args_count);
    if (!new_args) {
        return NULL;
    }
    if (additional_args) {
        for (Py_ssize_t i = 0; i < PyTuple_GET_SIZE(additional_args); i++) {
            PyObject *item = PyTuple_GET_ITEM(additional_args, i);
            Py_INCREF(item);
            PyTuple_SET_ITEM(new_args, i + 1, item);
        }
    }
    return new_args;
}

PyObject *run_unicode_accumulate_benchmark(PyObject *self, PyObject *args, PyObject *kwargs) {
    PyObject *callable;
    usize repeat;
    PyObject *unicode;
    PyObject *additional_args = NULL;
    static const char *kwlist[] = {"func", "repeat", "unicode", "args", NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "OKO|O", (char **)kwlist, &callable, &repeat, &unicode, &additional_args)) {
        PyErr_SetString(PyExc_TypeError, "Invalid argument");
        goto fail;
    }
    if (!PyCallable_Check(callable)) {
        PyErr_SetString(PyExc_TypeError, "First argument must be callable");
        goto fail;
    }
    if (!PyUnicode_Check(unicode)) {
        PyErr_SetString(PyExc_TypeError, "Third argument must be unicode");
        goto fail;
    }
    PyUnicodeNewCallArg call_arg;
    call_arg.valid = false;
    usize total = 0;
    for (usize i = 0; i < repeat; i++) {
        // create copy of unicode object.
        PyObject *new_args = _parse_additional_args(additional_args);
        if (!new_args) goto fail;
        PyObject *unicode_copy = _copy_unicode(unicode, &call_arg);
        if (!unicode_copy) {
            Py_DECREF(new_args);
            goto fail;
        }
        PyTuple_SET_ITEM(new_args, 0, unicode_copy);
        usize start = perf_counter();
        PyObject *result = PyObject_Call(callable, new_args, NULL);
        usize end = perf_counter();
        assert(unicode_copy->ob_refcnt == 1);
        Py_DECREF(new_args);
        unicode_copy = NULL;
        new_args = NULL;
        if (unlikely(!result)) {
            if (!PyErr_Occurred()) {
                PyErr_SetString(PyExc_RuntimeError, "Failed to call callable");
            }
            goto fail;
        } else {
            Py_DECREF(result);
        }
        total += end - start;
    }
    return PyLong_FromUnsignedLongLong(total);
fail:;
    return NULL;
}
