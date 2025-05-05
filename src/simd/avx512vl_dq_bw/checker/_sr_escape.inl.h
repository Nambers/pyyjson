#ifdef PYYJSON_CLANGD_DUMMY
#    ifndef COMPILE_READ_UCS_LEVEL
#        define COMPILE_READ_UCS_LEVEL 1
#    endif
#endif
//
#include "simd/avx512vl_dq_bw/common.h"
//
#define COMPILE_SIMD_BITS 512
#include "commondef/sr_in.inl.h"

force_inline AVX512_BITMASK_TYPE get_escape_bitmask(vector_a x) {
    AVX512_BITMASK_TYPE bitmask_1 = cmpeq_bitmask(x, broadcast(_Slash));
    AVX512_BITMASK_TYPE bitmask_2 = cmpeq_bitmask(x, broadcast(_Quote));
    AVX512_BITMASK_TYPE bitmask_3 = unsigned_cmplt_bitmask(x, broadcast(ControlMax));
    return bitmask_1 | bitmask_2 | bitmask_3;
}

force_inline usize escape_bitmask_to_done_count(AVX512_BITMASK_TYPE bitmask) {
    if (sizeof(AVX512_BITMASK_TYPE) == 8) {
        return u64_tz_bits((u64)bitmask);
    }
    assert(sizeof(AVX512_BITMASK_TYPE) < 8);
    return u32_tz_bits((u32)bitmask);
}

force_inline usize joined4_escape_bitmask_to_done_count(AVX512_BITMASK_TYPE bitmask1,
                                                        AVX512_BITMASK_TYPE bitmask2,
                                                        AVX512_BITMASK_TYPE bitmask3,
                                                        AVX512_BITMASK_TYPE bitmask4) {
#if COMPILE_READ_UCS_LEVEL == 1
#    define TZBITS u64_tz_bits
#else
#    define TZBITS u32_tz_bits
#endif
#define TOTALBITCOUNT (64 / COMPILE_READ_UCS_LEVEL)
    assert(bitmask1 | bitmask2 | bitmask3 | bitmask4);
    usize d1 = TZBITS(bitmask1);
    usize d2 = TZBITS(bitmask2);
    usize d3 = TZBITS(bitmask3);
    usize d4 = TZBITS(bitmask4);
    if (bitmask1) return TOTALBITCOUNT * 0 + d1;
    if (bitmask2) return TOTALBITCOUNT * 1 + d2;
    if (bitmask3) return TOTALBITCOUNT * 2 + d3;
    return TOTALBITCOUNT * 3 + d4;
#undef TOTALBITCOUNT
#undef TZBITS
}

#include "commondef/sr_out.inl.h"
#undef COMPILE_SIMD_BITS
