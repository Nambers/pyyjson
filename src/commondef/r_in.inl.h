#include "r_out.inl.h"
/*
 * Macros IN
 */
#if COMPILE_READ_UCS_LEVEL == 4
#define _FROM_TYPE u32
#define READ_BIT_SIZE 32
#elif COMPILE_READ_UCS_LEVEL == 2
#define _FROM_TYPE u16
#define READ_BIT_SIZE 16
#elif COMPILE_READ_UCS_LEVEL == 1
#define _FROM_TYPE u8
#define READ_BIT_SIZE 8
#else
#error "COMPILE_READ_UCS_LEVEL must be 1, 2 or 4"
#endif

#define CHECK_COUNT_MAX (SIMD_BIT_SIZE / 8 / sizeof(_FROM_TYPE))
