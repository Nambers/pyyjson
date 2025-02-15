// requires: WRITE

#include "simd_impl.h"
#include "mask_table.h"

#define WRITE_PARTIAL_HEAD PYYJSON_CONCAT2(write_partial_head, COMPILE_WRITE_UCS_LEVEL)
#define WRITE_PARTIAL_TAIL PYYJSON_CONCAT2(write_partial_tail, COMPILE_WRITE_UCS_LEVEL)

// WRITE_PARTIAL_HEAD defines for 512 and 128
#if SIMD_BIT_SIZE == 512 || !PYYJSON_HAS_BLENDV
force_inline void WRITE_PARTIAL_HEAD(void *restrict dst, SIMD_TYPE SIMD_VAR, Py_ssize_t head_cnt) {
#    if SIMD_BIT_SIZE == 512
#        define MASK_WRITER PYYJSON_SIMPLE_CONCAT2(_mm512_mask_storeu_epi, WRITE_BIT_SIZE)
    MASK_WRITER(dst, ((READ_512_MASK_TYPE)1 << (usize)head_cnt) - 1, SIMD_VAR);
#        undef MASK_WRITER
#    else
    static_assert(SIMD_BIT_SIZE == 128, "SIMD_BIT_SIZE == 128");
    if (head_cnt) write_simd(dst, SIMD_VAR);
#    endif
}
#endif

// WRITE_PARTIAL_TAIL defines for 256 and 128
#if SIMD_BIT_SIZE != 512 && PYYJSON_HAS_BLENDV
force_inline void WRITE_PARTIAL_TAIL(void *restrict dst, SIMD_TYPE SIMD_VAR, Py_ssize_t tail_cnt) {
#    if WRITE_SUPPORT_MASK_WRITE
    static_assert(SIMD_BIT_SIZE == 256 && COMPILE_WRITE_UCS_LEVEL == 4, "");
    _mm256_maskstore_epi32((i32 *)dst, load_simd_aligned(read_tail_mask_table_32(tail_cnt)), SIMD_VAR);
#    else
    static_assert(PYYJSON_HAS_BLENDV, "PYYJSON_HAS_BLENDV");
#        define BLENDV_WRITER PYYJSON_CONCAT2(blendv_writetail, SIMD_BIT_SIZE)
#        define MASK_TABLE_READER PYYJSON_CONCAT2(read_tail_mask_table, WRITE_BIT_SIZE)
    if (tail_cnt) {
        BLENDV_WRITER(SIMD_VAR, dst, load_simd_aligned(MASK_TABLE_READER(tail_cnt)));
    }
#        undef MASK_TABLE_READER
#        undef BLENDV_WRITER
#    endif
}
#endif

#undef WRITE_PARTIAL_HEAD
#undef WRITE_PARTIAL_TAIL
