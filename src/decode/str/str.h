#ifndef PYYJSON_DECODE_STR_H
#define PYYJSON_DECODE_STR_H

#include "common.h"
#include "decode/decode.h"
#include "decoder_impl_wrap.h"
#include "simd/simd_impl.h"
#include "simd/union_vector.h"
#include "tools.h"


// decode impl

#include "simd/compile_feature_check.h"

#define COMPILE_UCS_LEVEL 1
#define COMPILE_WRITE_UCS_LEVEL 1
#include "_srw_ucs_decoder.inl.h"
#undef COMPILE_WRITE_UCS_LEVEL

#define COMPILE_WRITE_UCS_LEVEL 2
#include "_srw_ucs_decoder.inl.h"
#undef COMPILE_WRITE_UCS_LEVEL

#define COMPILE_WRITE_UCS_LEVEL 4
#include "_srw_ucs_decoder.inl.h"
#undef COMPILE_WRITE_UCS_LEVEL
#undef COMPILE_UCS_LEVEL

#define COMPILE_UCS_LEVEL 2
#define COMPILE_WRITE_UCS_LEVEL 2
#include "_srw_ucs_decoder.inl.h"
#undef COMPILE_WRITE_UCS_LEVEL

#define COMPILE_WRITE_UCS_LEVEL 4
#include "_srw_ucs_decoder.inl.h"
#undef COMPILE_WRITE_UCS_LEVEL
#undef COMPILE_UCS_LEVEL

#define COMPILE_UCS_LEVEL 4
#define COMPILE_WRITE_UCS_LEVEL 4
#include "_srw_ucs_decoder.inl.h"
#undef COMPILE_WRITE_UCS_LEVEL
#undef COMPILE_UCS_LEVEL

#undef COMPILE_SIMD_BITS


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
