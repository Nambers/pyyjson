#ifndef PYYJSON_SIMD_AVX2_FULL_H
#define PYYJSON_SIMD_AVX2_FULL_H

#include "checker.h"
#include "checkmax.h"
#include "common.h"
#include "cvt.h"
#include "trailing.h"
#include "utf8.h"
#if defined(COMPILE_CONTEXT_DECODE)
#    include "decode.h"
#endif
#if defined(COMPILE_CONTEXT_ENCODE)
#    include "encode.h"
#endif
#endif // PYYJSON_SIMD_AVX2_FULL_H
