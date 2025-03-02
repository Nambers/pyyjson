
#include "test_common.h"

int main(int argc, char **argv) {
    PyObject *pyyjson_module;

    if (!initialize_cpython()) {
        PyErr_Print();
        Py_Finalize();
        return 1;
    }


    pyyjson_module = import_pyyjson();

    if (!pyyjson_module) {
        PyErr_Print();
        Py_Finalize();
        return 1;
    }

    // extract pyyjson module info
    // TODO
    Py_DECREF(pyyjson_module);

    Py_Finalize();

    return 0;
}
