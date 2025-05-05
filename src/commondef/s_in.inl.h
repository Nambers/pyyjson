#ifdef PYYJSON_CLANGD_DUMMY
#    ifndef COMPILE_SIMD_BITS
#        define COMPILE_SIMD_BITS 256
#    endif
#endif

#ifndef COMPILE_SIMD_BITS
#    error "COMPILE_SIMD_BITS must be 128, 256 or 512"
#endif

#if COMPILE_SIMD_BITS == 128
#    define SIMD_BITS_DOUBLE 256
#elif COMPILE_SIMD_BITS == 256
#    define SIMD_BITS_DOUBLE 512
#elif COMPILE_SIMD_BITS == 512
#    define SIMD_BITS_DOUBLE 1024
#else
#    error ""
#endif


#define broadcast_u8 PYYJSON_CONCAT2(broadcast_u8, COMPILE_SIMD_BITS)
#define broadcast_u16 PYYJSON_CONCAT2(broadcast_u16, COMPILE_SIMD_BITS)
#define broadcast_u32 PYYJSON_CONCAT2(broadcast_u32, COMPILE_SIMD_BITS)
#define setzero PYYJSON_CONCAT2(setzero, COMPILE_SIMD_BITS)
#define cvt_u16_to_u32 PYYJSON_CONCAT2(cvt_u16_to_u32, COMPILE_SIMD_BITS)
#define cvt_u8_to_u16 PYYJSON_CONCAT2(cvt_u8_to_u16, COMPILE_SIMD_BITS)
#define cvt_u8_to_u32 PYYJSON_CONCAT2(cvt_u8_to_u32, COMPILE_SIMD_BITS)
#define rshift_u16 PYYJSON_CONCAT2(rshift_u16, COMPILE_SIMD_BITS)
#define rshift_u32 PYYJSON_CONCAT2(rshift_u32, COMPILE_SIMD_BITS)
#define to_bitmask PYYJSON_CONCAT2(to_bitmask, COMPILE_SIMD_BITS)
#define testz PYYJSON_CONCAT2(testz, COMPILE_SIMD_BITS)
