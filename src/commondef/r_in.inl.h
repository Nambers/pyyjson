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
#define SET_ALL PYYJSON_CONCAT3(broadcast, READ_BIT_SIZE, SIMD_BIT_SIZE)
#define LOAD_A(_x) PYYJSON_CONCAT3(load, SIMD_BIT_SIZE, aligned)((const VECTOR_TYPE *)(_x))
#define LOAD_U(_x) PYYJSON_CONCAT2(load, SIMD_BIT_SIZE)((const VECTOR_TYPE_U *)(_x))
#if PYYJSON_X86 && SIMD_BIT_SIZE == 512
#    define VECTOR_MASK_TYPE READ_512_MASK_TYPE
#else
#    define VECTOR_MASK_TYPE VECTOR_TYPE
#endif

//
#define CHECK_ESCAPE_IMPL_GET_MASK PYYJSON_CONCAT2(check_escape_impl_get_mask, COMPILE_READ_UCS_LEVEL)
force_inline VECTOR_MASK_TYPE CHECK_ESCAPE_IMPL_GET_MASK(const _FROM_TYPE *restrict src, VECTOR_TYPE *restrict _out_vec);
