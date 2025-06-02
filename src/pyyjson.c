#define PY_SSIZE_T_CLEAN
#include "pyyjson.h"
#include "tls.h"


typedef PyObject *pyyjson_cache_type;

extern pyyjson_cache_type AssociativeKeyCache[PYYJSON_KEY_CACHE_SIZE];

PyObject *pyyjson_Encode(PyObject *self, PyObject *args, PyObject *kwargs);
PyObject *pyyjson_EncodeToBytes(PyObject *self, PyObject *args, PyObject *kwargs);
PyObject *pyyjson_Decode(PyObject *self, PyObject *args, PyObject *kwargs);
PyObject *pyyjson_FileEncode(PyObject *self, PyObject *args, PyObject *kwargs);
PyObject *pyyjson_DecodeFile(PyObject *self, PyObject *args, PyObject *kwargs);
#if PYYJSON_BUILD_BENCHMARK
PyObject *run_unicode_accumulate_benchmark(PyObject *self, PyObject *args, PyObject *kwargs);
PyObject *run_object_accumulate_benchmark(PyObject *self, PyObject *args, PyObject *kwargs);
PyObject *run_object_benchmark(PyObject *self, PyObject *args, PyObject *kwargs);
PyObject *inspect_pyunicode(PyObject *self, PyObject *args, PyObject *kwargs);
#endif
PyObject *pyyjson_print_current_features(PyObject *self, PyObject *);
PyObject *pyyjson_get_current_features(PyObject *self, PyObject *);

PyObject *JSONDecodeError = NULL;
PyObject *JSONEncodeError = NULL;

static PyMethodDef pyyjson_Methods[] = {
        {"encode", (PyCFunction)pyyjson_Encode, METH_VARARGS | METH_KEYWORDS, "dumps(obj, indent=None)\n--\n\nConverts arbitrary object recursively into JSON."},
        {"decode", (PyCFunction)pyyjson_Decode, METH_VARARGS | METH_KEYWORDS, "decode(s)\n--\n\nConverts JSON as string to dict object structure."},
        {"dumps", (PyCFunction)pyyjson_Encode, METH_VARARGS | METH_KEYWORDS, "dumps(obj, indent=None)\n--\n\nConverts arbitrary object recursively into JSON."},
        {"dumps_to_bytes", (PyCFunction)pyyjson_EncodeToBytes, METH_VARARGS | METH_KEYWORDS, "dumps_to_bytes(obj, indent=None)\n--\n\nConverts arbitrary object recursively into JSON."},
        {"loads", (PyCFunction)pyyjson_Decode, METH_VARARGS | METH_KEYWORDS, "loads(s)\n--\n\nConverts JSON as string to dict object structure."},
        {"print_current_features", pyyjson_print_current_features, METH_NOARGS, "print_current_features()\n--\n\nPrints current features."},
        {"get_current_features", pyyjson_get_current_features, METH_NOARGS, "get_current_features()\n--\n\nGet current features."},
#if PYYJSON_BUILD_BENCHMARK
        {"run_unicode_accumulate_benchmark", (PyCFunction)run_unicode_accumulate_benchmark, METH_VARARGS | METH_KEYWORDS, "Benchmark."},
        {"run_object_accumulate_benchmark", (PyCFunction)run_object_accumulate_benchmark, METH_VARARGS | METH_KEYWORDS, "Benchmark."},
        {"run_object_benchmark", (PyCFunction)run_object_benchmark, METH_VARARGS | METH_KEYWORDS, "Benchmark."},
        {"inspect_pyunicode", (PyCFunction)inspect_pyunicode, METH_VARARGS | METH_KEYWORDS, "Inspect PyUnicode."},
#endif
        {NULL, NULL, 0, NULL} /* Sentinel */
};

static void module_free(void *m);

static struct PyModuleDef moduledef = {
        PyModuleDef_HEAD_INIT,
        "pyyjson",
        0,               /* m_doc */
        0,               /* m_size */
        pyyjson_Methods, /* m_methods */
        NULL,            /* m_slots */
        NULL,            /* m_traverse */
        NULL,            /* m_clear */
        module_free      /* m_free */
};

static void module_free(void *m) {
    for (size_t i = 0; i < PYYJSON_KEY_CACHE_SIZE; i++) {
        Py_XDECREF(AssociativeKeyCache[i]);
    }

    if (unlikely(!pyyjson_tls_free())) {
        // critical
        printf("pyyjson: failed to free TLS\n");
    }
#if PYYJSON_ENABLE_TRACE
    size_t cached = 0;
    for (size_t i = 0; i < PYYJSON_KEY_CACHE_SIZE; i++) {
        if (AssociativeKeyCache[i]) cached++;
    }
    printf("key cache: %zu/%d\n", cached, PYYJSON_KEY_CACHE_SIZE);
#endif // PYYJSON_ENABLE_TRACE
}

#if PY_MINOR_VERSION >= 13
PyTypeObject *PyNone_Type = NULL;

force_inline void _init_PyNone_Type(PyTypeObject *none_type) {
    PyNone_Type = none_type;
}
#endif


PyMODINIT_FUNC PyInit_pyyjson(void) {
    PyObject *module;

    // This function is not supported in PyPy.
    if ((module = PyState_FindModule(&moduledef)) != NULL) {
        Py_INCREF(module);
        return module;
    }

    module = PyModule_Create(&moduledef);
    if (module == NULL) {
        return NULL;
    }

    PyModule_AddStringConstant(module, "__version__", PYYJSON_VERSION);

    JSONDecodeError = PyErr_NewException("pyyjson.JSONDecodeError", PyExc_ValueError, NULL);
    Py_XINCREF(JSONDecodeError);
    if (PyModule_AddObject(module, "JSONDecodeError", JSONDecodeError) < 0) {
        Py_XDECREF(JSONDecodeError);
        Py_CLEAR(JSONDecodeError);
        Py_DECREF(module);
        return NULL;
    }

    JSONEncodeError = PyErr_NewException("pyyjson.JSONEncodeError", PyExc_ValueError, NULL);
    Py_XINCREF(JSONEncodeError);
    if (PyModule_AddObject(module, "JSONEncodeError", JSONEncodeError) < 0) {
        Py_XDECREF(JSONEncodeError);
        Py_CLEAR(JSONEncodeError);
        Py_DECREF(module);
        return NULL;
    }

    // TLS init.
    if (unlikely(!pyyjson_tls_init())) {
        PyErr_SetString(PyExc_RuntimeError, "Failed to initialize TLS");
        Py_XDECREF(JSONEncodeError);
        Py_CLEAR(JSONEncodeError);
        Py_DECREF(module);
        return NULL;
    }

    // do pyyjson internal init.
    memset(AssociativeKeyCache, 0, sizeof(AssociativeKeyCache));

#if PY_MINOR_VERSION >= 13
    _init_PyNone_Type(Py_TYPE(Py_None));
#endif

    return module;
}

