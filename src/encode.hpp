#ifndef PYYJSON_ENCODE_H
#define PYYJSON_ENCODE_H

#include "pyyjson_config.h"

#ifdef __cplusplus
extern "C" {
#endif

extern PyObject *JSONEncodeError;

PyObject *pyyjson_Encode(PyObject *self, PyObject *args, PyObject *kwargs);

#ifdef __cplusplus
}
#endif
#endif // PYYJSON_ENCODE_H
