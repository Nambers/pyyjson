#ifndef PYYJSON_DECODE_STR_UCS_H
#define PYYJSON_DECODE_STR_UCS_H

#include "escape.h"
#include "pythonlib.h"
#include "simd/long_cvt.h"
//
#include "simd/compile_feature_check.h"
#define COMPILE_UCS_LEVEL 1
#include "_ucs.inl.h"
#undef COMPILE_UCS_LEVEL
#define COMPILE_UCS_LEVEL 2
#include "_ucs.inl.h"
#undef COMPILE_UCS_LEVEL
#define COMPILE_UCS_LEVEL 4
#include "_ucs.inl.h"
#undef COMPILE_UCS_LEVEL
#undef COMPILE_SIMD_BITS

#endif // PYYJSON_DECODE_STR_UCS_H
