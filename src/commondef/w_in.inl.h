#include "w_out.inl.h"
/*
 * Macros IN
 */
#if COMPILE_WRITE_UCS_LEVEL == 4
#    define _WRITER U32_WRITER
#    define _TARGET_TYPE u32
#    define WRITE_BIT_SIZE 32
#elif COMPILE_WRITE_UCS_LEVEL == 2
#    define _WRITER U16_WRITER
#    define _TARGET_TYPE u16
#    define WRITE_BIT_SIZE 16
#elif COMPILE_WRITE_UCS_LEVEL == 1
#    define _WRITER U8_WRITER
#    define _TARGET_TYPE u8
#    define WRITE_BIT_SIZE 8
#else
#    error "COMPILE_WRITE_UCS_LEVEL must be 1, 2 or 4"
#endif
