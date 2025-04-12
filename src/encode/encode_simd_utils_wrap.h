#ifndef ENCODE_SIMD_UTILS_WRAP_H
#define ENCODE_SIMD_UTILS_WRAP_H


#define COMPILE_READ_UCS_LEVEL 1
#define COMPILE_WRITE_UCS_LEVEL 1
#include "_encode_simd_utils.inl.h"
#undef COMPILE_WRITE_UCS_LEVEL
#undef COMPILE_READ_UCS_LEVEL

#define COMPILE_READ_UCS_LEVEL 1
#define COMPILE_WRITE_UCS_LEVEL 2
#include "_encode_simd_utils.inl.h"
#undef COMPILE_WRITE_UCS_LEVEL
#undef COMPILE_READ_UCS_LEVEL

#define COMPILE_READ_UCS_LEVEL 1
#define COMPILE_WRITE_UCS_LEVEL 4
#include "_encode_simd_utils.inl.h"
#undef COMPILE_WRITE_UCS_LEVEL
#undef COMPILE_READ_UCS_LEVEL

#define COMPILE_READ_UCS_LEVEL 2
#define COMPILE_WRITE_UCS_LEVEL 2
#include "_encode_simd_utils.inl.h"
#undef COMPILE_WRITE_UCS_LEVEL
#undef COMPILE_READ_UCS_LEVEL

#define COMPILE_READ_UCS_LEVEL 2
#define COMPILE_WRITE_UCS_LEVEL 4
#include "_encode_simd_utils.inl.h"
#undef COMPILE_WRITE_UCS_LEVEL
#undef COMPILE_READ_UCS_LEVEL

#define COMPILE_READ_UCS_LEVEL 4
#define COMPILE_WRITE_UCS_LEVEL 4
#include "_encode_simd_utils.inl.h"
#undef COMPILE_WRITE_UCS_LEVEL
#undef COMPILE_READ_UCS_LEVEL


#endif // ENCODE_SIMD_UTILS_WRAP_H
