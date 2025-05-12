#ifndef PYYJSON_ENCODE_BYTES_WRITERS_UCS1_H
#define PYYJSON_ENCODE_BYTES_WRITERS_UCS1_H

#ifndef PYYJSON_CLANGD_DUMMY
#    include "pyyjson.h"
#    include "simd/simd_detect.h"
#    include "simd/sse2/checker.h"
#    include "simd/sse2/common.h"
#    include "simd/vector_types.h"
#endif

// forward declaration
force_inline void encode_one_special_ucs1(u8 **writer_addr, u8 unicode);

#define COMPILE_READ_UCS_LEVEL 1
//


#if SUPPORT_SIMD_256BITS
#    define COMPILE_SIMD_BITS 256
#    include "compile_context/sr_in.inl.h"

force_inline void bytes_write_ucs1_trailing_256(u8 **dst_addr, const u8 *src, usize len) {
    // TODO
}

#    undef COMPILE_SIMD_BITS
#    include "compile_context/sr_out.inl.h"
#endif

#if SUPPORT_SIMD_512BITS
#    define COMPILE_SIMD_BITS 512
#    include "compile_context/sr_in.inl.h"

force_inline void bytes_write_ucs1_trailing_512(u8 **dst_addr, const u8 *src, usize len) {
    // TODO
}

#    undef COMPILE_SIMD_BITS
#    include "compile_context/sr_out.inl.h"
#endif
//
#undef COMPILE_READ_UCS_LEVEL
#endif // PYYJSON_ENCODE_BYTES_WRITERS_UCS1_H
