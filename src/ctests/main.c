
#include "pyyjson.h"

force_inline PyObject *import_pyyjson(void) {
    PyObject *pModule = PyImport_ImportModule("pyyjson");
    return pModule;
}

int main(int argc, char **argv) {
    Py_Initialize();

    PyObject *pyyjson_module;

    pyyjson_module = import_pyyjson();

    if (!pyyjson_module) {
        PyErr_Print();
        Py_Finalize();
        return 1;
    }

    // extract pyyjson module info
    // TODO

    return 0;
}
