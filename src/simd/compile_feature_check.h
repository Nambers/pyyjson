#if __AVX512F__ && __AVX512CD__ && __AVX512VL__ && __AVX512DQ__ && __AVX512BW__
#    define COMPILE_SIMD_BITS 512
#elif __AVX2__
#    define COMPILE_SIMD_BITS 256
#else
#    define COMPILE_SIMD_BITS 128
#endif
