#ifdef PYYJSON_CLANGD_DUMMY
#    ifndef COMPILE_READ_UCS_LEVEL
#        define COMPILE_READ_UCS_LEVEL 1
#    endif
#endif
//
#include "simd/avx2/common.h"
//
force_inline const void *read_tail_mask_table_8(Py_ssize_t row);
force_inline const void *read_head_mask_table_8(Py_ssize_t row);

#define COMPILE_SIMD_BITS 256
#include "compile_context/sr_in.inl.h"

force_inline vector_a get_escape_mask(vector_a x) {
    vector_a t1 = broadcast(_Slash);
    vector_a t2 = broadcast(_Quote);
    vector_a t3 = broadcast(ControlMax);
    vector_a x1 = x == t1;
    vector_a x2 = x == t2;
#if CHECK_ESCAPE_LT512_USE_SIGNED_SATURATED_MINUS
    vector_a x3 = unsigned_saturate_minus(t3, x);
#else
    vector_a x3_1 = signed_cmpgt(x, broadcast(-1));
    vector_a x3_2 = signed_cmpgt(t3, x);
    vector_a x3 = x3_1 & x3_2;
#endif
    return x1 | x2 | x3;
}

force_inline u32 escape_mask_to_bitmask(vector_a mask) {
#if CHECK_ESCAPE_LT512_USE_SIGNED_SATURATED_MINUS
    mask = cmpeq(mask, setzero());
    u32 bitmask = to_bitmask(mask);
    bitmask = ~bitmask;
#else
    u32 bitmask = to_bitmask(mask);
#endif
    return bitmask;
}

force_inline usize escape_mask_to_done_count(vector_a mask) {
    return u32_tz_bits(escape_mask_to_bitmask(mask)) / COMPILE_READ_UCS_LEVEL;
}

force_inline usize joined4_escape_mask_to_done_count(vector_a mask1,
                                                     vector_a mask2,
                                                     vector_a mask3,
                                                     vector_a mask4) {
    u64 bitmask1, bitmask2, bitmask3, bitmask4;
    u64 bitmask[2];
    bitmask1 = 0xffffffff & escape_mask_to_bitmask(mask1);
    bitmask2 = escape_mask_to_bitmask(mask2);
    bitmask3 = 0xffffffff & escape_mask_to_bitmask(mask3);
    bitmask4 = escape_mask_to_bitmask(mask4);
    bitmask[0] = bitmask1 | (bitmask2 << 32);
    bitmask[1] = bitmask3 | (bitmask4 << 32);
    assert(bitmask[0] | bitmask[1]);
    if (bitmask[0]) return u64_tz_bits(bitmask[0]) / COMPILE_READ_UCS_LEVEL;
    return 64 / COMPILE_READ_UCS_LEVEL + u64_tz_bits(bitmask[1]) / COMPILE_READ_UCS_LEVEL;
}

force_inline vector_a get_high_mask(u8 count) {
    const vector_a *mask_ptr = read_tail_mask_table_8(32 - count * sizeof(_src_t));
    return *mask_ptr;
}

force_inline vector_a high_mask(vector_a x, u8 count) {
    // const vector_a *mask_ptr = read_tail_mask_table_8(32 - count * sizeof(_src_t));
    return x & get_high_mask(count);
}

force_inline vector_a get_low_mask(u8 count) {
    const vector_a *mask_ptr = read_head_mask_table_8(count * sizeof(_src_t));
    return *mask_ptr;
}

force_inline vector_a low_mask(vector_a x, u8 count) {
    // const vector_a *mask_ptr = read_head_mask_table_8(count * sizeof(_src_t));
    return x & get_low_mask(count);
}

#include "compile_context/sr_out.inl.h"
#undef COMPILE_SIMD_BITS
