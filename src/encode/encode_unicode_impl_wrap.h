#ifndef PYYJSON_ENCODE_UNICODE_IMPL_WRAP_H
#define PYYJSON_ENCODE_UNICODE_IMPL_WRAP_H

#include "encode/indent_writer.h"
#include "encode_shared.h"
#include "simd/simd_detect.h"
#include "simd/simd_impl.h"
#include "unicode/unicode.h"
//
#include "simd/compile_feature_check.h"
#define COMPILE_INDENT_LEVEL 0

#define COMPILE_READ_UCS_LEVEL 1
#define COMPILE_WRITE_UCS_LEVEL 1
#include "_encode_unicode_impl.inl.h"
#undef COMPILE_WRITE_UCS_LEVEL
#undef COMPILE_READ_UCS_LEVEL

#define COMPILE_READ_UCS_LEVEL 1
#define COMPILE_WRITE_UCS_LEVEL 2
#include "_encode_unicode_impl.inl.h"
#undef COMPILE_WRITE_UCS_LEVEL
#undef COMPILE_READ_UCS_LEVEL

#define COMPILE_READ_UCS_LEVEL 1
#define COMPILE_WRITE_UCS_LEVEL 4
#include "_encode_unicode_impl.inl.h"
#undef COMPILE_WRITE_UCS_LEVEL
#undef COMPILE_READ_UCS_LEVEL

#define COMPILE_READ_UCS_LEVEL 2
#define COMPILE_WRITE_UCS_LEVEL 2
#include "_encode_unicode_impl.inl.h"
#undef COMPILE_WRITE_UCS_LEVEL
#undef COMPILE_READ_UCS_LEVEL

#define COMPILE_READ_UCS_LEVEL 2
#define COMPILE_WRITE_UCS_LEVEL 4
#include "_encode_unicode_impl.inl.h"
#undef COMPILE_WRITE_UCS_LEVEL
#undef COMPILE_READ_UCS_LEVEL

#define COMPILE_READ_UCS_LEVEL 4
#define COMPILE_WRITE_UCS_LEVEL 4
#include "_encode_unicode_impl.inl.h"
#undef COMPILE_WRITE_UCS_LEVEL
#undef COMPILE_READ_UCS_LEVEL

#undef COMPILE_INDENT_LEVEL


#define COMPILE_INDENT_LEVEL 2

#define COMPILE_READ_UCS_LEVEL 1
#define COMPILE_WRITE_UCS_LEVEL 1
#include "_encode_unicode_impl.inl.h"
#undef COMPILE_WRITE_UCS_LEVEL
#undef COMPILE_READ_UCS_LEVEL

#define COMPILE_READ_UCS_LEVEL 1
#define COMPILE_WRITE_UCS_LEVEL 2
#include "_encode_unicode_impl.inl.h"
#undef COMPILE_WRITE_UCS_LEVEL
#undef COMPILE_READ_UCS_LEVEL

#define COMPILE_READ_UCS_LEVEL 1
#define COMPILE_WRITE_UCS_LEVEL 4
#include "_encode_unicode_impl.inl.h"
#undef COMPILE_WRITE_UCS_LEVEL
#undef COMPILE_READ_UCS_LEVEL

#define COMPILE_READ_UCS_LEVEL 2
#define COMPILE_WRITE_UCS_LEVEL 2
#include "_encode_unicode_impl.inl.h"
#undef COMPILE_WRITE_UCS_LEVEL
#undef COMPILE_READ_UCS_LEVEL

#define COMPILE_READ_UCS_LEVEL 2
#define COMPILE_WRITE_UCS_LEVEL 4
#include "_encode_unicode_impl.inl.h"
#undef COMPILE_WRITE_UCS_LEVEL
#undef COMPILE_READ_UCS_LEVEL

#define COMPILE_READ_UCS_LEVEL 4
#define COMPILE_WRITE_UCS_LEVEL 4
#include "_encode_unicode_impl.inl.h"
#undef COMPILE_WRITE_UCS_LEVEL
#undef COMPILE_READ_UCS_LEVEL

#undef COMPILE_INDENT_LEVEL

#define COMPILE_INDENT_LEVEL 4

#define COMPILE_READ_UCS_LEVEL 1
#define COMPILE_WRITE_UCS_LEVEL 1
#include "_encode_unicode_impl.inl.h"
#undef COMPILE_WRITE_UCS_LEVEL
#undef COMPILE_READ_UCS_LEVEL

#define COMPILE_READ_UCS_LEVEL 1
#define COMPILE_WRITE_UCS_LEVEL 2
#include "_encode_unicode_impl.inl.h"
#undef COMPILE_WRITE_UCS_LEVEL
#undef COMPILE_READ_UCS_LEVEL

#define COMPILE_READ_UCS_LEVEL 1
#define COMPILE_WRITE_UCS_LEVEL 4
#include "_encode_unicode_impl.inl.h"
#undef COMPILE_WRITE_UCS_LEVEL
#undef COMPILE_READ_UCS_LEVEL

#define COMPILE_READ_UCS_LEVEL 2
#define COMPILE_WRITE_UCS_LEVEL 2
#include "_encode_unicode_impl.inl.h"
#undef COMPILE_WRITE_UCS_LEVEL
#undef COMPILE_READ_UCS_LEVEL

#define COMPILE_READ_UCS_LEVEL 2
#define COMPILE_WRITE_UCS_LEVEL 4
#include "_encode_unicode_impl.inl.h"
#undef COMPILE_WRITE_UCS_LEVEL
#undef COMPILE_READ_UCS_LEVEL

#define COMPILE_READ_UCS_LEVEL 4
#define COMPILE_WRITE_UCS_LEVEL 4
#include "_encode_unicode_impl.inl.h"
#undef COMPILE_WRITE_UCS_LEVEL
#undef COMPILE_READ_UCS_LEVEL

#undef COMPILE_INDENT_LEVEL

#undef COMPILE_SIMD_BITS

#endif // PYYJSON_ENCODE_UNICODE_IMPL_WRAP_H
