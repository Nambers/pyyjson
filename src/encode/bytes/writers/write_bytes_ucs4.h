#ifndef PYYJSON_ENCODE_BYTES_WRITERS_UCS4_H
#define PYYJSON_ENCODE_BYTES_WRITERS_UCS4_H
//
#define COMPILE_SIMD_BITS 128
#define COMPILE_READ_UCS_LEVEL 4
#include "compile_context/sr_in.inl.h"


#include "compile_context/sr_out.inl.h"
#undef COMPILE_READ_UCS_LEVEL
#undef COMPILE_SIMD_BITS

#endif // PYYJSON_ENCODE_BYTES_WRITERS_UCS4_H
