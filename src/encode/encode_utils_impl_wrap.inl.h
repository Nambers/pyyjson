/* 
 * Some utility functions only related to *write*, like vector reserve, writing number
 * need macro: COMPILE_WRITE_UCS_LEVEL, value: 1, 2, or 4.
 */

#define COMPILE_WRITE_UCS_LEVEL 1
#include "encode_utils_impl.inl.h"
#undef COMPILE_WRITE_UCS_LEVEL

#define COMPILE_WRITE_UCS_LEVEL 2
#include "encode_utils_impl.inl.h"
#undef COMPILE_WRITE_UCS_LEVEL

#define COMPILE_WRITE_UCS_LEVEL 4
#include "encode_utils_impl.inl.h"
#undef COMPILE_WRITE_UCS_LEVEL
