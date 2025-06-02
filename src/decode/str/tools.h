#ifndef PYYJSON_DECODE_TOOLS_H
#define PYYJSON_DECODE_TOOLS_H

#include "decode/decode.h"
#include "simd/simd_impl.h"

// _r_tools
#define COMPILE_READ_UCS_LEVEL 1
#include "_r_tools.inl.h"
#undef COMPILE_READ_UCS_LEVEL

#define COMPILE_READ_UCS_LEVEL 2
#include "_r_tools.inl.h"
#undef COMPILE_READ_UCS_LEVEL

#define COMPILE_READ_UCS_LEVEL 4
#include "_r_tools.inl.h"
#undef COMPILE_READ_UCS_LEVEL

#endif // PYYJSON_DECODE_TOOLS_H
