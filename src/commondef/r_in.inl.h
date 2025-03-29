#include "pyyjson.h"
#include "r_out.inl.h"
/*
 * Macros IN
 */
#if COMPILE_READ_UCS_LEVEL == 4
#    define _FROM_TYPE u32
#    define READ_BIT_SIZE 32
#    define READ_BIT_SIZEx2 64
#    define READ_BIT_SIZEx4 128
#    define READ_BIT_SIZEx8 256
#    define READ_512_MASK_TYPE u16
#elif COMPILE_READ_UCS_LEVEL == 2
#    define _FROM_TYPE u16
#    define READ_BIT_SIZE 16
#    define READ_BIT_SIZEx2 32
#    define READ_BIT_SIZEx4 64
#    define READ_BIT_SIZEx8 128
#    define READ_512_MASK_TYPE u32
#elif COMPILE_READ_UCS_LEVEL == 1
#    define _FROM_TYPE u8
#    define READ_BIT_SIZE 8
#    define READ_BIT_SIZEx2 16
#    define READ_BIT_SIZEx4 32
#    define READ_BIT_SIZEx8 64
#    define READ_512_MASK_TYPE u64
#else
#    error "COMPILE_READ_UCS_LEVEL must be 1, 2 or 4"
#endif

#define CHECK_COUNT_MAX (SIMD_BIT_SIZE / 8 / sizeof(_FROM_TYPE))
#define READ_UNSIGNED_BIT_NAME PYYJSON_SIMPLE_CONCAT2(U, READ_BIT_SIZE)
#define VECTOR_TYPE PYYJSON_CONCAT4(VECTOR, READ_UNSIGNED_BIT_NAME, SIMD_BIT_SIZE, A)
#define VECTOR_TYPE_U PYYJSON_CONCAT4(VECTOR, READ_UNSIGNED_BIT_NAME, SIMD_BIT_SIZE, U)
