#ifndef PYYJSON_READWRITE_UTILS_WRAP_H
#define PYYJSON_READWRITE_UTILS_WRAP_H

#include "compile_feature_check.h"
#define COMPILE_READ_UCS_LEVEL 1
#define COMPILE_WRITE_UCS_LEVEL 1
#include "_readwrite_utils.inl.h"
#undef COMPILE_WRITE_UCS_LEVEL
#undef COMPILE_READ_UCS_LEVEL

#define COMPILE_READ_UCS_LEVEL 1
#define COMPILE_WRITE_UCS_LEVEL 2
#include "_readwrite_utils.inl.h"
#undef COMPILE_WRITE_UCS_LEVEL
#undef COMPILE_READ_UCS_LEVEL

#define COMPILE_READ_UCS_LEVEL 1
#define COMPILE_WRITE_UCS_LEVEL 4
#include "_readwrite_utils.inl.h"
#undef COMPILE_WRITE_UCS_LEVEL
#undef COMPILE_READ_UCS_LEVEL

#define COMPILE_READ_UCS_LEVEL 2
#define COMPILE_WRITE_UCS_LEVEL 2
#include "_readwrite_utils.inl.h"
#undef COMPILE_WRITE_UCS_LEVEL
#undef COMPILE_READ_UCS_LEVEL

#define COMPILE_READ_UCS_LEVEL 2
#define COMPILE_WRITE_UCS_LEVEL 4
#include "_readwrite_utils.inl.h"
#undef COMPILE_WRITE_UCS_LEVEL
#undef COMPILE_READ_UCS_LEVEL

#define COMPILE_READ_UCS_LEVEL 4
#define COMPILE_WRITE_UCS_LEVEL 4
#include "_readwrite_utils.inl.h"
#undef COMPILE_WRITE_UCS_LEVEL
#undef COMPILE_READ_UCS_LEVEL

#undef COMPILE_SIMD_BITS

#endif // PYYJSON_READWRITE_UTILS_WRAP_H
