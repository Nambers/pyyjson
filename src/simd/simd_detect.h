#ifndef PYYJSON_SIMD_DETECT_H
#define PYYJSON_SIMD_DETECT_H

#if PYYJSON_DETECT_SIMD

#    if TARGET_SIMD_ARCH == x86
#        if __AVX512F__ && __AVX512BW__
#            define SIMD_BIT_SIZE 512
#        elif __AVX2__
#            define SIMD_BIT_SIZE 256
#        else
#            define SIMD_BIT_SIZE 128
#        endif

#        define SIMD_32 __m128i
#        define SIMD_64 __m128i
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
#        define HAS_SIMD 1

#    elif TARGET_SIMD_ARCH == aarch
// aarch64
#    endif
#endif

#endif // PYYJSON_SIMD_DETECT_H
