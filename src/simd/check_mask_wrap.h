#ifndef PYYJSON_CHECK_MASK_WRAP_H
#define PYYJSON_CHECK_MASK_WRAP_H


#define COMPILE_READ_UCS_LEVEL 1
#include "_check_mask.inl.h"
#undef COMPILE_READ_UCS_LEVEL

#define COMPILE_READ_UCS_LEVEL 2
#include "_check_mask.inl.h"
#undef COMPILE_READ_UCS_LEVEL

#define COMPILE_READ_UCS_LEVEL 4
#include "_check_mask.inl.h"
#undef COMPILE_READ_UCS_LEVEL


#endif // PYYJSON_CHECK_MASK_WRAP_H
