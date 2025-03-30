#ifndef PYYJSON_WRITE_UTILS_WRAP_H
#define PYYJSON_WRITE_UTILS_WRAP_H


#define COMPILE_WRITE_UCS_LEVEL 1
#include "_write_utils.inl.h"
#undef COMPILE_WRITE_UCS_LEVEL

#define COMPILE_WRITE_UCS_LEVEL 2
#include "_write_utils.inl.h"
#undef COMPILE_WRITE_UCS_LEVEL

#define COMPILE_WRITE_UCS_LEVEL 4
#include "_write_utils.inl.h"
#undef COMPILE_WRITE_UCS_LEVEL


#endif // PYYJSON_WRITE_UTILS_WRAP_H
