#ifndef PYYJSON_DECODE_STR_H
#define PYYJSON_DECODE_STR_H

#include "common.h"

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

// _sr_checkmax
#define COMPILE_SIMD_BITS 128
#define COMPILE_UCS_LEVEL 0
#include "_sr_checkmax.inl.h"
#undef COMPILE_UCS_LEVEL

#define COMPILE_UCS_LEVEL 1
#include "_sr_checkmax.inl.h"
#undef COMPILE_UCS_LEVEL

#define COMPILE_UCS_LEVEL 2
#include "_sr_checkmax.inl.h"
#undef COMPILE_UCS_LEVEL

#define COMPILE_UCS_LEVEL 4
#include "_sr_checkmax.inl.h"
#undef COMPILE_UCS_LEVEL
#undef COMPILE_SIMD_BITS

#ifdef PYYJSON_SIMD_AVX2_CHECKMAX_H
#    define COMPILE_SIMD_BITS 256
#    define COMPILE_UCS_LEVEL 0
#    include "_sr_checkmax.inl.h"
#    undef COMPILE_UCS_LEVEL

#    define COMPILE_UCS_LEVEL 1
#    include "_sr_checkmax.inl.h"
#    undef COMPILE_UCS_LEVEL

#    define COMPILE_UCS_LEVEL 2
#    include "_sr_checkmax.inl.h"
#    undef COMPILE_UCS_LEVEL

#    define COMPILE_UCS_LEVEL 4
#    include "_sr_checkmax.inl.h"
#    undef COMPILE_UCS_LEVEL
#    undef COMPILE_SIMD_BITS
#endif

#ifdef PYYJSON_SIMD_AVX512FCD_CHECKMAX_H
#    define COMPILE_SIMD_BITS 512
#    define COMPILE_UCS_LEVEL 0
#    include "_sr_checkmax.inl.h"
#    undef COMPILE_UCS_LEVEL

#    define COMPILE_UCS_LEVEL 1
#    include "_sr_checkmax.inl.h"
#    undef COMPILE_UCS_LEVEL

#    define COMPILE_UCS_LEVEL 2
#    include "_sr_checkmax.inl.h"
#    undef COMPILE_UCS_LEVEL

#    define COMPILE_UCS_LEVEL 4
#    include "_sr_checkmax.inl.h"
#    undef COMPILE_UCS_LEVEL
#    undef COMPILE_SIMD_BITS
#endif

#endif // PYYJSON_DECODE_STR_H
