#ifndef PYYJSON_SIMD_DETECT_H
#define PYYJSON_SIMD_DETECT_H

#include <Python.h>
#if PYYJSON_DETECT_SIMD

#    if PYYJSON_X86
#        if __AVX512F__ && __AVX512BW__ && __AVX512VL__
#            define SIMD_BIT_SIZE 512
#            define SIMD_FEATURE_NAME avx512
#        elif __AVX2__
#            define SIMD_BIT_SIZE 256
#            define SIMD_FEATURE_NAME avx2
#        else
#            define SIMD_BIT_SIZE 128
#            if __SSE4_2__
#                define SIMD_FEATURE_NAME sse4_2
#            else
#                define SIMD_FEATURE_NAME sse2
#            endif
#        endif

#        define SIMD_128 __m128i
#        if defined(_MSC_VER)
#            define SIMD_128_IU __m128i
#        else
#            define SIMD_128_IU __m128i_u
#        endif
#        define SIMD_256 __m256i
#        if defined(_MSC_VER)
#            define SIMD_256_IU __m256i
#        else
#            define SIMD_256_IU __m256i_u
#        endif
#        define SIMD_512 __m512i

#        if (SIMD_BIT_SIZE > 128) || __SSE4_1__
#            define PYYJSON_HAS_BLENDV 1
#        else
#            define PYYJSON_HAS_BLENDV 0
#        endif
#    elif PYYJSON_AARCH
#        define SIMD_FEATURE_NAME neon
#        define PYYJSON_HAS_BLENDV 0
#        define SIMD_BIT_SIZE 128
// aarch64 TODO
#    else
#        error "unsupported architecture"
#    endif
#else
#    error "cannot detect SIMD feature"
#endif

#if BUILD_MULTI_LIB
#    ifndef SIMD_FEATURE_NAME
#        error "SIMD_FEATURE_NAME is not defined"
#    endif
#    define SIMD_NAME_MODIFIER(x) PYYJSON_CONCAT2(x, SIMD_FEATURE_NAME)
#else
#    define SIMD_NAME_MODIFIER(x) x
#endif

#if PYYJSON_X86
#    include <immintrin.h>
#    if defined(_MSC_VER)
#        include <intrin.h>
#    endif
#elif PYYJSON_AARCH
#    include <arm_neon.h>
static_assert(__LITTLE_ENDIAN__, "currently only little endian is supported");
#endif

#endif // PYYJSON_SIMD_DETECT_H
