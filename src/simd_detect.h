#ifndef PYYJSON_SIMD_DETECT_H
#define PYYJSON_SIMD_DETECT_H

#if defined(__x86_64__) || defined(_M_X64)
#if __AVX512F__ && __AVX512BW__ && __AVX512VL__
#define SIMD_BIT_SIZE 512
#elif __AVX2__
#define SIMD_BIT_SIZE 256
#else
#define SIMD_BIT_SIZE 128
#endif

#define SIMD_32 __m128i
#define SIMD_64 __m128i
#define SIMD_128 __m128i
#define SIMD_256 __m256i
#define SIMD_512 __m512i
#define HAS_SIMD 1
#elif defined(i386) || defined(__i386__) || defined(__i386) || defined(_M_IX86)
#elif defined(__aarch64__) || defined(_M_ARM64)
// aarch64
#endif

#endif // PYYJSON_SIMD_DETECT_H
