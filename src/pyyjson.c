#define PY_SSIZE_T_CLEAN
#include "decode.h"
#include "yyjson.h"

#define MODULE_STATE(o) ((modulestate *) PyModule_GetState(o))

PyObject *pyyjson_Encode(PyObject *self, PyObject *args, PyObject *kwargs);
PyObject *pyyjson_Decode(PyObject *self, PyObject *args, PyObject *kwargs);
PyObject *pyyjson_FileEncode(PyObject *self, PyObject *args, PyObject *kwargs);
PyObject *pyyjson_DecodeFile(PyObject *self, PyObject *args, PyObject *kwargs);

PyObject *JSONDecodeError = NULL;
PyObject *JSONEncodeError = NULL;

static PyMethodDef pyyjson_Methods[] = {
        // {"encode", (PyCFunction)pyyjson_Encode, METH_VARARGS | METH_KEYWORDS, "Converts arbitrary object recursively into JSON. "},
        {"decode", (PyCFunction) pyyjson_Decode, METH_VARARGS | METH_KEYWORDS, "Converts JSON as string to dict object structure."},
        // {"dumps", (PyCFunction)pyyjson_Encode, METH_VARARGS | METH_KEYWORDS, "Converts arbitrary object recursively into JSON. "},
        {"loads", (PyCFunction) pyyjson_Decode, METH_VARARGS | METH_KEYWORDS, "Converts JSON as string to dict object structure."},
        // {"dump", (PyCFunction)pyyjson_FileEncode, METH_VARARGS | METH_KEYWORDS, "Converts arbitrary object recursively into JSON file. "},
        // {"load", (PyCFunction)pyyjson_DecodeFile, METH_VARARGS | METH_KEYWORDS, "Converts JSON as file to dict object structure."},
        {NULL, NULL, 0, NULL} /* Sentinel */
};

static int module_traverse(PyObject *m, visitproc visit, void *arg);
static int module_clear(PyObject *m);
static void module_free(void *m);
typedef struct
{
    PyObject *type_decimal;
} modulestate;

static struct PyModuleDef moduledef = {
        PyModuleDef_HEAD_INIT,
        "pyyjson",
        0,                   /* m_doc */
        sizeof(modulestate), /* m_size */
        pyyjson_Methods,     /* m_methods */
        NULL,                /* m_slots */
        module_traverse,     /* m_traverse */
        module_clear,        /* m_clear */
        module_free          /* m_free */
};

static int module_traverse(PyObject *m, visitproc visit, void *arg) {
    Py_VISIT(MODULE_STATE(m)->type_decimal);
    return 0;
}

static int module_clear(PyObject *m) {
    Py_CLEAR(MODULE_STATE(m)->type_decimal);
    return 0;
}

static void module_free(void *m) {
    module_clear((PyObject *) m);

    for (size_t i = 0; i < PYYJSON_KEY_CACHE_SIZE; i++) {
        Py_XDECREF(AssociativeKeyCache[i]);
    }
#if PYYJSON_ENABLE_TRACE
    size_t cached = 0;
    for (size_t i = 0; i < PYYJSON_KEY_CACHE_SIZE; i++) {
        if (AssociativeKeyCache[i]) cached++;
    }
    printf("key cache: %zu/%d\n", cached, PYYJSON_KEY_CACHE_SIZE);
#endif // PYYJSON_ENABLE_TRACE
}

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

    PyModule_AddStringConstant(module, "__version__", YYJSON_VERSION_STRING);

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

    // do pyyjson internal init.
    memset(AssociativeKeyCache, 0, sizeof(AssociativeKeyCache));

    return module;
}

PyObject *pyyjson_Decode(PyObject *self, PyObject *args, PyObject *kwargs) {
    const char *string = NULL;
    size_t len = 0;
    static const char *kwlist[] = {"s", NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "s#", (char **) kwlist, &string, &len)) {
        PyErr_SetString(PyExc_TypeError, "Invalid argument");
        return NULL;
    }

    PyObject *root = yyjson_read_opts(string, len);
    if (yyjson_unlikely(!root)) {
        if (!PyErr_Occurred()) {
            PyErr_SetString(JSONDecodeError, "Failed to decode JSON: unknown error");
        }
    }

    return root;
}
