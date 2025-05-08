#include "pyyjson.h"
//
#include "r_in.inl.h"
#include "s_in.inl.h"

#define MAKE_SR_NAME(_x_) PYYJSON_CONCAT3(_x_, READ_UNSIGNED_BIT_NAME, COMPILE_SIMD_BITS)
#define READ_BATCH_COUNT (COMPILE_SIMD_BITS / 8 / sizeof(_src_t))

#ifdef COMPILE_UCS_LEVEL
#    if COMPILE_UCS_LEVEL == 0
#        define __UCS_NAME ascii
#    else
#        define __UCS_NAME PYYJSON_SIMPLE_CONCAT2(ucs, COMPILE_UCS_LEVEL)
#    endif
#    define MAKE_S_UCS_NAME(_x_) PYYJSON_CONCAT3(_x_, __UCS_NAME, COMPILE_SIMD_BITS)
#endif

#define vector_a MAKE_SR_NAME(vector_a)
#define vector_u MAKE_SR_NAME(vector_u)

// #define SET_ALL PYYJSON_CONCAT3(broadcast, READ_BIT_SIZE, COMPILE_SIMD_BITS)
// #define LOAD_A(_x) PYYJSON_CONCAT3(load, COMPILE_SIMD_BITS, aligned)((const vector_a *)(_x))
// #define LOAD_U(_x) PYYJSON_CONCAT2(load, COMPILE_SIMD_BITS)((const vector_u *)(_x))
#if PYYJSON_X86 && COMPILE_SIMD_BITS == 512
#    define VECTOR_MASK_TYPE AVX512_BITMASK_TYPE
#else
#    define VECTOR_MASK_TYPE vector_a
#endif

#if COMPILE_SIMD_BITS == 512
#    define _VECx2_A_ PYYJSON_CONCAT4(vector, a, READ_UNSIGNED_BIT_NAME, 1024)
#    define _VECx4_A_ PYYJSON_CONCAT4(vector, a, READ_UNSIGNED_BIT_NAME, 2048)
#    define _VECx2_U_ PYYJSON_CONCAT4(vector, u, READ_UNSIGNED_BIT_NAME, 1024)
#    define _VECx4_U_ PYYJSON_CONCAT4(vector, u, READ_UNSIGNED_BIT_NAME, 2048)
#    define _VEC_half_A_ PYYJSON_CONCAT4(vector, a, READ_UNSIGNED_BIT_NAME, 256)
#    define _VEC_half_U_ PYYJSON_CONCAT4(vector, u, READ_UNSIGNED_BIT_NAME, 256)
#    define _VEC_quad_A_ PYYJSON_CONCAT4(vector, a, READ_UNSIGNED_BIT_NAME, 128)
#    define _VEC_quad_U_ PYYJSON_CONCAT4(vector, u, READ_UNSIGNED_BIT_NAME, 128)
#elif COMPILE_SIMD_BITS == 256
#    define _VECx2_A_ PYYJSON_CONCAT4(vector, a, READ_UNSIGNED_BIT_NAME, 512)
#    define _VECx4_A_ PYYJSON_CONCAT4(vector, a, READ_UNSIGNED_BIT_NAME, 1024)
#    define _VECx2_U_ PYYJSON_CONCAT4(vector, u, READ_UNSIGNED_BIT_NAME, 512)
#    define _VECx4_U_ PYYJSON_CONCAT4(vector, u, READ_UNSIGNED_BIT_NAME, 1024)
#    define _VEC_half_A_ PYYJSON_CONCAT4(vector, a, READ_UNSIGNED_BIT_NAME, 128)
#    define _VEC_half_U_ PYYJSON_CONCAT4(vector, u, READ_UNSIGNED_BIT_NAME, 128)
#    define _VEC_quad_A_ PYYJSON_CONCAT4(vector, a, READ_UNSIGNED_BIT_NAME, 64)
#    define _VEC_quad_U_ PYYJSON_CONCAT4(vector, u, READ_UNSIGNED_BIT_NAME, 64)
#else
#    define _VECx2_A_ PYYJSON_CONCAT4(vector, a, READ_UNSIGNED_BIT_NAME, 256)
#    define _VECx4_A_ PYYJSON_CONCAT4(vector, a, READ_UNSIGNED_BIT_NAME, 512)
#    define _VECx2_U_ PYYJSON_CONCAT4(vector, u, READ_UNSIGNED_BIT_NAME, 256)
#    define _VECx4_U_ PYYJSON_CONCAT4(vector, u, READ_UNSIGNED_BIT_NAME, 512)
#    define _VEC_half_A_ PYYJSON_CONCAT4(vector, a, READ_UNSIGNED_BIT_NAME, 64)
#    define _VEC_half_U_ PYYJSON_CONCAT4(vector, u, READ_UNSIGNED_BIT_NAME, 64)
#    define _VEC_quad_A_ PYYJSON_CONCAT4(vector, a, READ_UNSIGNED_BIT_NAME, 32)
#    define _VEC_quad_U_ PYYJSON_CONCAT4(vector, u, READ_UNSIGNED_BIT_NAME, 32)
#endif

#if !PYYJSON_X86 || COMPILE_READ_UCS_LEVEL == 4
// x86: for bit size < 512, we don't have cmp_epu8,
// the mask is calculated by subs_epu8.
// so we have to cmpeq with zero to get the real bit mask.
// x86 doesn't have the signed saturate minus for 32-bit integers.
#    define CHECK_ESCAPE_LT512_USE_SIGNED_SATURATED_MINUS 0
#else
#    define CHECK_ESCAPE_LT512_USE_SIGNED_SATURATED_MINUS 1
#endif

#define UNIONVECx2 PYYJSON_CONCAT4(UnionVectorA, READ_UNSIGNED_BIT_NAME, COMPILE_SIMD_BITS, x2)
#define UNIONVECx4 PYYJSON_CONCAT4(UnionVectorA, READ_UNSIGNED_BIT_NAME, COMPILE_SIMD_BITS, x4)
//
#define CHECK_ESCAPE_IMPL_GET_MASK PYYJSON_CONCAT2(check_escape_impl_get_mask, COMPILE_READ_UCS_LEVEL)
// force_inline VECTOR_MASK_TYPE CHECK_ESCAPE_IMPL_GET_MASK(const _src_t *restrict src, vector_a *restrict _out_vec);
//
#define CHECK_MASK_AND_GET_DONE_COUNT PYYJSON_CONCAT2(check_mask_and_get_done_count, COMPILE_READ_UCS_LEVEL)
// force_inline void CHECK_MASK_AND_GET_DONE_COUNT(vector_a vec, bool *out_checked, usize *out_done_count);
//
#define CHECK_MASK_AND_GET_DONE_COUNTx2 PYYJSON_CONCAT2(check_mask_and_get_done_countx2, COMPILE_READ_UCS_LEVEL)
// force_inline void CHECK_MASK_AND_GET_DONE_COUNTx2(_VECx2_A_ vec, bool *out_checked, usize *out_done_count);
//
#define CHECK_MASK_AND_GET_DONE_COUNTx4 PYYJSON_CONCAT2(check_mask_and_get_done_countx4, COMPILE_READ_UCS_LEVEL)
// force_inline void CHECK_MASK_AND_GET_DONE_COUNTx4(_VECx4_A_ vec, bool *out_checked, usize *out_done_count);
//
#define CHECK_MASK_128_SRC_T PYYJSON_CONCAT4(vector, a, READ_UNSIGNED_BIT_NAME, 128)
#if COMPILE_SIMD_BITS == 512
#    define CHECK_MASK_128_SRC_MASK_T __mmask16
#else
#    define CHECK_MASK_128_SRC_MASK_T CHECK_MASK_128_SRC_T
#endif
#define CHECK_MASK_AND_GET_DONE_COUNT_128_WITH_MASK PYYJSON_CONCAT2(check_mask_and_get_done_count_128, COMPILE_READ_UCS_LEVEL)
// force_inline void CHECK_MASK_AND_GET_DONE_COUNT_128_WITH_MASK(CHECK_MASK_128_SRC_MASK_T vec, bool *out_checked, usize *out_done_count, CHECK_MASK_128_SRC_MASK_T *optional_mask);
//
#define get_escape_mask MAKE_SR_NAME(get_escape_mask)
#define escape_mask_to_bitmask MAKE_SR_NAME(escape_mask_to_bitmask)
#define escape_mask_to_done_count MAKE_SR_NAME(escape_mask_to_done_count)
#define joined4_escape_mask_to_done_count MAKE_SR_NAME(joined4_escape_mask_to_done_count)
//
#define broadcast MAKE_SR_NAME(broadcast)
//
#define unsigned_saturate_minus MAKE_SR_NAME(unsigned_saturate_minus)
//
#define signed_cmplt MAKE_SR_NAME(signed_cmplt)
//
#define signed_cmpgt MAKE_SR_NAME(signed_cmpgt)
//
#define cmpeq MAKE_SR_NAME(cmpeq)
//
#define get_escape_bitmask MAKE_SR_NAME(get_escape_bitmask)
//
#define escape_bitmask_to_done_count MAKE_SR_NAME(escape_bitmask_to_done_count)
//
#define joined4_escape_bitmask_to_done_count MAKE_SR_NAME(joined4_escape_bitmask_to_done_count)
//
#define cmpeq_bitmask MAKE_SR_NAME(cmpeq_bitmask)
#define cmpneq_bitmask MAKE_SR_NAME(cmpneq_bitmask)
//
#define unsigned_cmple_bitmask MAKE_SR_NAME(unsigned_cmple_bitmask)
//
#define unsigned_cmplt_bitmask MAKE_SR_NAME(unsigned_cmplt_bitmask)
//
#define high_mask MAKE_SR_NAME(high_mask)
//
#define low_mask MAKE_SR_NAME(low_mask)
//
#define maskz_loadu MAKE_SR_NAME(maskz_loadu)
//
#define fast_skip_spaces MAKE_SR_NAME(fast_skip_spaces)
//
#define checkmax MAKE_SR_NAME(checkmax)
//
#ifdef COMPILE_UCS_LEVEL
#    define __check_vector_max_char_internal MAKE_S_UCS_NAME(__check_vector_max_char_internal)
#    define check_vector_max_char MAKE_S_UCS_NAME(check_vector_max_char)
#endif
