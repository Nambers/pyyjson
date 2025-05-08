#ifdef PYYJSON_CLANGD_DUMMY
#    ifndef COMPILE_READ_UCS_LEVEL
#        define COMPILE_READ_UCS_LEVEL 1
#    endif
#endif
//
#include "simd/avx512vl_dq_bw/common.h"
//
#define COMPILE_SIMD_BITS 512
#include "compile_context/sr_in.inl.h"

force_inline avx512_bitmask_t get_escape_bitmask(vector_a x) {
    avx512_bitmask_t bitmask_1 = cmpeq_bitmask(x, broadcast(_Slash));
    avx512_bitmask_t bitmask_2 = cmpeq_bitmask(x, broadcast(_Quote));
    avx512_bitmask_t bitmask_3 = unsigned_cmplt_bitmask(x, broadcast(ControlMax));
    return bitmask_1 | bitmask_2 | bitmask_3;
}

force_inline usize escape_bitmask_to_done_count(avx512_bitmask_t bitmask) {
    if (sizeof(avx512_bitmask_t) == 8) {
        return u64_tz_bits((u64)bitmask);
    }
    assert(sizeof(avx512_bitmask_t) < 8);
    return u32_tz_bits((u32)bitmask);
}

force_inline usize joined4_escape_bitmask_to_done_count(avx512_bitmask_t bitmask1,
                                                        avx512_bitmask_t bitmask2,
                                                        avx512_bitmask_t bitmask3,
                                                        avx512_bitmask_t bitmask4) {
#if COMPILE_READ_UCS_LEVEL == 1
#    define TZBITS u64_tz_bits
#else
#    define TZBITS u32_tz_bits
#endif
#define TOTALBITCOUNT (64 / COMPILE_READ_UCS_LEVEL)
    assert(bitmask1 | bitmask2 | bitmask3 | bitmask4);
    if (bitmask1) return TOTALBITCOUNT * 0 + TZBITS(bitmask1);
    if (bitmask2) return TOTALBITCOUNT * 1 + TZBITS(bitmask2);
    if (bitmask3) return TOTALBITCOUNT * 2 + TZBITS(bitmask3);
    return TOTALBITCOUNT * 3 + TZBITS(bitmask4);
#undef TOTALBITCOUNT
#undef TZBITS
}

#include "compile_context/sr_out.inl.h"
#undef COMPILE_SIMD_BITS
