#ifndef PYYJSON_COMPILE_CONTEXT_S
#define PYYJSON_COMPILE_CONTEXT_S

// fake include and definition to deceive clangd
#ifdef PYYJSON_CLANGD_DUMMY
#    include "pyyjson.h"
#    ifndef COMPILE_SIMD_BITS
#        define COMPILE_SIMD_BITS 256
#    endif
#endif

/*
 * Basic definitions.
 */
#if COMPILE_SIMD_BITS == 128
#    define SIMD_BITS_DOUBLE 256
#elif COMPILE_SIMD_BITS == 256
#    define SIMD_BITS_DOUBLE 512
#elif COMPILE_SIMD_BITS == 512
#    define SIMD_BITS_DOUBLE 1024
#else
#    error "COMPILE_SIMD_BITS must be 128, 256 or 512"
#endif

// Name creation macro.
#define MAKE_S_NAME(_x_) PYYJSON_CONCAT2(_x_, COMPILE_SIMD_BITS)

/*
 * Names using S context.
 */
#define broadcast_u8 MAKE_S_NAME(broadcast_u8)
#define broadcast_u16 MAKE_S_NAME(broadcast_u16)
#define broadcast_u32 MAKE_S_NAME(broadcast_u32)
#define setzero MAKE_S_NAME(setzero)
#define cvt_u16_to_u32 MAKE_S_NAME(cvt_u16_to_u32)
#define cvt_u8_to_u16 MAKE_S_NAME(cvt_u8_to_u16)
#define cvt_u8_to_u32 MAKE_S_NAME(cvt_u8_to_u32)
#define rshift_u16 MAKE_S_NAME(rshift_u16)
#define rshift_u32 MAKE_S_NAME(rshift_u32)
#define to_bitmask MAKE_S_NAME(to_bitmask)
#define testz MAKE_S_NAME(testz)
#define bytes_write_ucs1_trailing MAKE_S_NAME(bytes_write_ucs1_trailing)
#define bytes_write_ucs2_trailing MAKE_S_NAME(bytes_write_ucs2_trailing)
#define bytes_write_ucs4_trailing MAKE_S_NAME(bytes_write_ucs4_trailing)
#define fast_skip_spaces_u8 MAKE_S_NAME(fast_skip_spaces_u8)
#define fast_skip_spaces_u16 MAKE_S_NAME(fast_skip_spaces_u16)
#define fast_skip_spaces_u32 MAKE_S_NAME(fast_skip_spaces_u32)
#endif // PYYJSON_COMPILE_CONTEXT_S
