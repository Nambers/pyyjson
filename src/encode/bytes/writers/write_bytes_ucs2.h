#ifndef PYYJSON_ENCODE_BYTES_WRITERS_UCS2_H
#define PYYJSON_ENCODE_BYTES_WRITERS_UCS2_H

#ifndef PYYJSON_CLANGD_DUMMY
#    include "pyyjson.h"
#    include "simd/simd_detect.h"
#    include "simd/sse2/checker.h"
#    include "simd/sse2/common.h"
#    include "simd/vector_types.h"
#endif

// forward declaration
force_inline void encode_one_special_ucs1(u8 **writer_addr, u8 unicode);
force_inline int ucs2_get_type(u16 unicode, bool *is_escaped);
force_inline void ucs2_encode_3bytes_utf8_ssse3(vector_a_u16_128 x, u8 *writer);
force_inline bool encode_one_ucs2(u8 **writer_addr, u16 unicode);
force_inline void ucs2_encode_2bytes_utf8_sse2(vector_a_u16_128 x, u8 *writer);
extern PyObject *JSONEncodeError;

#define COMPILE_READ_UCS_LEVEL 2
//
#define COMPILE_SIMD_BITS 128
#include "compile_context/sr_in.inl.h"



#include "compile_context/sr_out.inl.h"
#undef COMPILE_SIMD_BITS

#if SUPPORT_SIMD_256BITS
#    define COMPILE_SIMD_BITS 256
#    include "compile_context/sr_in.inl.h"

force_inline bool bytes_write_ucs2_trailing_256(u8 **writer_addr, const u16 *src, usize len) {
    return false;
}

#    include "compile_context/sr_out.inl.h"
#    undef COMPILE_SIMD_BITS
#endif

#if SUPPORT_SIMD_512BITS
#    define COMPILE_SIMD_BITS 512
#    include "compile_context/sr_in.inl.h"

force_inline bool bytes_write_ucs2_trailing_512(u8 **writer_addr, const u16 *src, usize len) {
    return false;
}

#    include "compile_context/sr_out.inl.h"
#    undef COMPILE_SIMD_BITS
#endif

#undef COMPILE_READ_UCS_LEVEL

#endif // PYYJSON_ENCODE_BYTES_WRITERS_UCS2_H
