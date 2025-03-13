#include "pyyjson.h"

force_inline bool initialize_cpython(void) {
    PyObject *sys_module = NULL, *path = NULL, *add_path = NULL;
    //
    Py_Initialize();
    //
    sys_module = PyImport_ImportModule("sys");
    if (!sys_module) goto fail;
    path = PyObject_GetAttrString(sys_module, "path");
    if (!path) goto fail;
    add_path = PyUnicode_FromString(".");
    if (!add_path) goto fail;
    //
    if (0 != PyList_Append(path, add_path)) goto fail;
    //
    Py_DECREF(sys_module);
    Py_DECREF(path);
    Py_DECREF(add_path);
    return true;
fail:;
    Py_XDECREF(sys_module);
    Py_XDECREF(path);
    Py_XDECREF(add_path);
    return false;
}

// returns a new reference
force_inline PyObject *import_pyyjson(void) {
    PyObject *pModule = PyImport_ImportModule("pyyjson");
    return pModule;
}
