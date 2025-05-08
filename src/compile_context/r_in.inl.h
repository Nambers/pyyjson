#ifndef PYYJSON_COMPILE_CONTEXT_R
#define PYYJSON_COMPILE_CONTEXT_R

// fake include and definition to deceive clangd
#ifdef PYYJSON_CLANGD_DUMMY
#    include "pyyjson.h"
#    ifndef COMPILE_READ_UCS_LEVEL
#        define COMPILE_READ_UCS_LEVEL 1
#    endif
#endif

/*
 * Basic definitions.
 */
#if COMPILE_READ_UCS_LEVEL == 4
#    define READ_BIT_SIZE 32
#    define READ_BIT_SIZEx2 64
#    define READ_BIT_SIZEx4 128
#    define READ_BIT_SIZEx8 256
#    define AVX512BITMASK_SIZE 16
#elif COMPILE_READ_UCS_LEVEL == 2
#    define READ_BIT_SIZE 16
#    define READ_BIT_SIZEx2 32
#    define READ_BIT_SIZEx4 64
#    define READ_BIT_SIZEx8 128
#    define AVX512BITMASK_SIZE 32
#elif COMPILE_READ_UCS_LEVEL == 1
#    define READ_BIT_SIZE 8
#    define READ_BIT_SIZEx2 16
#    define READ_BIT_SIZEx4 32
#    define READ_BIT_SIZEx8 64
#    define AVX512BITMASK_SIZE 64
#else
#    error "COMPILE_READ_UCS_LEVEL must be 1, 2 or 4"
#endif

// The source type.
#define _src_t PYYJSON_SIMPLE_CONCAT2(u, READ_BIT_SIZE)

// Other type definitions.
#define avx512_bitmask_t PYYJSON_SIMPLE_CONCAT2(u, AVX512BITMASK_SIZE)

// Name creation macro.
#define MAKE_R_NAME(_x_) PYYJSON_CONCAT2(_x_, _src_t)

/*
 * Names using R context.
 */
#define cmpeq_2chars MAKE_R_NAME(cmpeq_2chars)
#define DecodeSrcInfo MAKE_R_NAME(DecodeSrcInfo)
#define verify_escape_hex MAKE_R_NAME(verify_escape_hex)
#define read_to_hex MAKE_R_NAME(read_to_hex)
#define _read_true MAKE_R_NAME(_read_true)
#define _read_false MAKE_R_NAME(_read_false)
#define _read_null MAKE_R_NAME(_read_null)
#define _read_inf MAKE_R_NAME(_read_inf)
#define _read_nan MAKE_R_NAME(_read_nan)
#define read_inf_or_nan MAKE_R_NAME(read_inf_or_nan)

#endif // PYYJSON_COMPILE_CONTEXT_R
