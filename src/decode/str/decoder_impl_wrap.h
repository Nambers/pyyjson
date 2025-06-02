#ifndef PYYJSON_DECODE_STR_DECODER_IMPL_WRAP_H
#define PYYJSON_DECODE_STR_DECODER_IMPL_WRAP_H

#include "decode/decode.h"
#include "simd/compile_feature_check.h"
#include "simd/simd_impl.h"
#include "simd/union_vector.h"
#include "tools.h"

// _r_impls and _sr_loop_impls
#define COMPILE_READ_UCS_LEVEL 1
#include "_r_impls.inl.h"
#include "_sr_loop_impls.inl.h"
#undef COMPILE_READ_UCS_LEVEL

#define COMPILE_READ_UCS_LEVEL 2
#include "_r_impls.inl.h"
#include "_sr_loop_impls.inl.h"
#undef COMPILE_READ_UCS_LEVEL

#define COMPILE_READ_UCS_LEVEL 4
#include "_r_impls.inl.h"
#include "_sr_loop_impls.inl.h"
#undef COMPILE_READ_UCS_LEVEL


#undef COMPILE_SIMD_BITS

#endif // PYYJSON_DECODE_STR_DECODER_IMPL_WRAP_H
