#include "unicode/unicode_buffer.h"

/*
 * Macros IN
 */
#if COMPILE_WRITE_UCS_LEVEL == 4
#    define _WRITER U32_WRITER
#    define _dst_t u32
#    define WRITE_BIT_SIZE 32
// #    define AVX512_BITMASK_TYPE u16
#elif COMPILE_WRITE_UCS_LEVEL == 2
#    define _WRITER U16_WRITER
#    define _dst_t u16
#    define WRITE_BIT_SIZE 16
// #    define AVX512_BITMASK_TYPE u32
#elif COMPILE_WRITE_UCS_LEVEL == 1
#    define _WRITER U8_WRITER
#    define _dst_t u8
#    define WRITE_BIT_SIZE 8
// #    define AVX512_BITMASK_TYPE u64
#else
#    error "COMPILE_WRITE_UCS_LEVEL must be 1, 2 or 4"
#endif

#define WRITE_BATCH_COUNT (COMPILE_SIMD_BITS / 8 / sizeof(_dst_t))
#define WRITE_UNSIGNED_BIT_NAME PYYJSON_SIMPLE_CONCAT2(u, WRITE_BIT_SIZE)
// #define _WVEC_A_ PYYJSON_CONCAT4(VECTOR, WRITE_UNSIGNED_BIT_NAME, COMPILE_SIMD_BITS, A)
// #define _WVEC_U_ PYYJSON_CONCAT4(VECTOR, WRITE_UNSIGNED_BIT_NAME, COMPILE_SIMD_BITS, U)

/*
 * Reserve space for the unicode buffer.
 */
#define UNICODE_BUFFER_RESERVE PYYJSON_CONCAT2(unicode_buffer_reserve, COMPILE_WRITE_UCS_LEVEL)

force_inline bool UNICODE_BUFFER_RESERVE(EncodeUnicodeBufferInfo *unicode_buffer_info, Py_ssize_t size);

#define WRITE_UNICODE_U64 PYYJSON_CONCAT2(write_unicode_u64, COMPILE_WRITE_UCS_LEVEL)
#define WRITE_UNICODE_F64 PYYJSON_CONCAT2(write_unicode_f64, COMPILE_WRITE_UCS_LEVEL)
//
#define _CONTROL_SEQ_TABLE PYYJSON_CONCAT2(_ControlSeqTable, COMPILE_WRITE_UCS_LEVEL)
