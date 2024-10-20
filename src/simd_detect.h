#ifndef PYYJSON_SIMD_DETECT_H
#define PYYJSON_SIMD_DETECT_H

#if defined(__AVX512F__) && defined(__AVX512BW__) && defined(__AVX512VL__)
#define SIMD_BIT_SIZE 512
#elif defined(__AVX2__)
#define SIMD_BIT_SIZE 256
#else
#define SIMD_BIT_SIZE 128
#endif

#endif // PYYJSON_SIMD_DETECT_H
