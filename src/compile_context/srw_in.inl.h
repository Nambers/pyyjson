#include "rw_in.inl.h"
//
#include "r_out.inl.h"
//
#include "sr_in.inl.h"
//
#include "s_out.inl.h"
#include "w_out.inl.h"
//
#include "sw_in.inl.h"


#define MAKE_SRW_NAME(_x_) PYYJSON_CONCAT4(_x_, READ_UNSIGNED_BIT_NAME, WRITE_UNSIGNED_BIT_NAME, COMPILE_SIMD_BITS)
#define trailing_copy_with_cvt MAKE_SRW_NAME(trailing_copy_with_cvt)
#define encode_trailing_copy_with_cvt MAKE_SRW_NAME(encode_trailing_copy_with_cvt)
#define cvt_to_dst MAKE_SRW_NAME(cvt_to_dst)
#define encode_unicode_loop MAKE_SRW_NAME(encode_unicode_loop)
#define encode_unicode_loop4 MAKE_SRW_NAME(encode_unicode_loop4)
#define encode_unicode_impl MAKE_SRW_NAME(encode_unicode_impl)
#define long_cvt MAKE_SRW_NAME(long_cvt)
#define long_back_cvt MAKE_SRW_NAME(long_back_cvt)
