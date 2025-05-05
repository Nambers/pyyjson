#ifdef PYYJSON_CLANGD_DUMMY
#    include "pyyjson.h"
#    ifndef COMPILE_READ_UCS_LEVEL
#        define COMPILE_READ_UCS_LEVEL 1
#    endif
#endif

/*
 * Macros IN
 */
#if COMPILE_READ_UCS_LEVEL == 4
#    define _src_t u32
#    define READ_BIT_SIZE 32
#    define READ_BIT_SIZEx2 64
#    define READ_BIT_SIZEx4 128
#    define READ_BIT_SIZEx8 256
#    define AVX512_BITMASK_TYPE u16
#elif COMPILE_READ_UCS_LEVEL == 2
#    define _src_t u16
#    define READ_BIT_SIZE 16
#    define READ_BIT_SIZEx2 32
#    define READ_BIT_SIZEx4 64
#    define READ_BIT_SIZEx8 128
#    define AVX512_BITMASK_TYPE u32
#elif COMPILE_READ_UCS_LEVEL == 1
#    define _src_t u8
#    define READ_BIT_SIZE 8
#    define READ_BIT_SIZEx2 16
#    define READ_BIT_SIZEx4 32
#    define READ_BIT_SIZEx8 64
#    define AVX512_BITMASK_TYPE u64
#else
#    error "COMPILE_READ_UCS_LEVEL must be 1, 2 or 4"
#endif

#define READ_UNSIGNED_BIT_NAME PYYJSON_SIMPLE_CONCAT2(u, READ_BIT_SIZE)
#define READ_UNSIGNED_BIT_NAME_UPPER PYYJSON_SIMPLE_CONCAT2(U, READ_BIT_SIZE)

#define cmpeq_2chars PYYJSON_CONCAT2(cmpeq_2chars, READ_UNSIGNED_BIT_NAME)

#define DecodeSrcInfo PYYJSON_CONCAT2(DecodeSrcInfo, READ_UNSIGNED_BIT_NAME)
#define verify_escape_hex PYYJSON_CONCAT2(verify_escape_hex, READ_UNSIGNED_BIT_NAME)
#define read_to_hex PYYJSON_CONCAT2(read_to_hex, READ_UNSIGNED_BIT_NAME)
#define _read_true PYYJSON_CONCAT2(_read_true, READ_UNSIGNED_BIT_NAME)
#define _read_false PYYJSON_CONCAT2(_read_false, READ_UNSIGNED_BIT_NAME)
#define _read_null PYYJSON_CONCAT2(_read_null, READ_UNSIGNED_BIT_NAME)
#define _read_inf PYYJSON_CONCAT2(_read_inf, READ_UNSIGNED_BIT_NAME)
#define _read_nan PYYJSON_CONCAT2(_read_nan, READ_UNSIGNED_BIT_NAME)
#define read_inf_or_nan PYYJSON_CONCAT2(read_inf_or_nan, READ_UNSIGNED_BIT_NAME)
