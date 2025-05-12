#ifndef PYYJSON_COMPILE_CONTEXT_SR
#define PYYJSON_COMPILE_CONTEXT_SR

// Include sub contexts.
#include "r_in.inl.h"
#include "s_in.inl.h"

/* The count of unicode that can be read in one SIMD register. */
#define READ_BATCH_COUNT (COMPILE_SIMD_BITS / 8 / sizeof(_src_t))

// Name creation macros.
#define MAKE_SR_NAME(_x_) PYYJSON_CONCAT3(_x_, _src_t, COMPILE_SIMD_BITS)
#ifdef COMPILE_UCS_LEVEL
#    if COMPILE_UCS_LEVEL == 0
#        define __UCS_NAME ascii
#    else
#        define __UCS_NAME PYYJSON_SIMPLE_CONCAT2(ucs, COMPILE_UCS_LEVEL)
#    endif
#    define MAKE_S_UCS_NAME(_x_) PYYJSON_CONCAT3(_x_, __UCS_NAME, COMPILE_SIMD_BITS)
#endif

/*
 * The very basic vector type (aligned/unaligned), 
 * that can be presented by a SIMD register.
 */
#define vector_a MAKE_SR_NAME(vector_a)
#define vector_u MAKE_SR_NAME(vector_u)


#if !PYYJSON_X86 || COMPILE_READ_UCS_LEVEL == 4
// x86: for bit size < 512, we don't have cmp_epu8,
// the mask is calculated by subs_epu8.
// so we have to cmpeq with zero to get the real bit mask.
// x86 doesn't have the signed saturate minus for 32-bit integers.
#    define CHECK_ESCAPE_LT512_USE_SIGNED_SATURATED_MINUS 0
#else
#    define CHECK_ESCAPE_LT512_USE_SIGNED_SATURATED_MINUS 1
#endif

/*
 * Names using SR context.
 */
#define unionvector_a_x4 PYYJSON_CONCAT2(MAKE_SR_NAME(unionvector_a), x4)
#define unionvector_u_x4 PYYJSON_CONCAT2(MAKE_SR_NAME(unionvector_u), x4)
//
#define get_bitmask_from MAKE_SR_NAME(get_bitmask_from)
#define get_escape_mask MAKE_SR_NAME(get_escape_mask)
#define escape_mask_to_bitmask MAKE_SR_NAME(escape_mask_to_bitmask)
#define escape_mask_to_done_count MAKE_SR_NAME(escape_mask_to_done_count)
#define escape_mask_to_done_count_no_eq0 MAKE_SR_NAME(escape_mask_to_done_count_no_eq0)
#define joined4_escape_mask_to_done_count MAKE_SR_NAME(joined4_escape_mask_to_done_count)
#define broadcast MAKE_SR_NAME(broadcast)
#define unsigned_saturate_minus MAKE_SR_NAME(unsigned_saturate_minus)
// signed_cmplt availability: SSE2
#define signed_cmplt MAKE_SR_NAME(signed_cmplt)
// signed_cmpgt availability: SSE2, AVX2
#define signed_cmpgt MAKE_SR_NAME(signed_cmpgt)
#define cmpeq MAKE_SR_NAME(cmpeq)
#define get_escape_bitmask MAKE_SR_NAME(get_escape_bitmask)
#define escape_bitmask_to_done_count MAKE_SR_NAME(escape_bitmask_to_done_count)
#define joined4_escape_bitmask_to_done_count MAKE_SR_NAME(joined4_escape_bitmask_to_done_count)
#define cmpeq_bitmask MAKE_SR_NAME(cmpeq_bitmask)
#define cmpneq_bitmask MAKE_SR_NAME(cmpneq_bitmask)
#define unsigned_cmple_bitmask MAKE_SR_NAME(unsigned_cmple_bitmask)
#define unsigned_cmplt_bitmask MAKE_SR_NAME(unsigned_cmplt_bitmask)
#define unsigned_cmpge_bitmask MAKE_SR_NAME(unsigned_cmpge_bitmask)
#define get_high_mask MAKE_SR_NAME(get_high_mask)
#define high_mask MAKE_SR_NAME(high_mask)
#define get_low_mask MAKE_SR_NAME(get_low_mask)
#define low_mask MAKE_SR_NAME(low_mask)
#define maskz_loadu MAKE_SR_NAME(maskz_loadu)
#define fast_skip_spaces MAKE_SR_NAME(fast_skip_spaces)
#define checkmax MAKE_SR_NAME(checkmax)

//
#if COMPILE_SIMD_BITS == 512
#    define testz_escape_mask(_x_) ((_x_) == 0)
#    define escape_anymask_to_done_count escape_bitmask_to_done_count
#else
#    define testz_escape_mask testz
#    define escape_anymask_to_done_count escape_mask_to_done_count
#endif

#ifdef COMPILE_UCS_LEVEL
#    define __check_vector_max_char_internal MAKE_S_UCS_NAME(__check_vector_max_char_internal)
#    define check_vector_max_char MAKE_S_UCS_NAME(check_vector_max_char)
#endif

#endif // PYYJSON_COMPILE_CONTEXT_SR
