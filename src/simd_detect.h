#ifndef PYYJSON_SIMD_DETECT_H
#define PYYJSON_SIMD_DETECT_H

#if defined(__AVX512F__) && defined(__AVX512BW__) && defined(__AVX512VL__)
#define SIMD_BIT_SIZE 512
#elif defined(__AVX2__)
#define SIMD_BIT_SIZE 256
#else
#define SIMD_BIT_SIZE 128
#endif

#define SIMD_32 __m128i
#define SIMD_64 __m128i
#define SIMD_128 __m128i
#define SIMD_256 __m256i
#define SIMD_512 __m512i

#endif // PYYJSON_SIMD_DETECT_H
