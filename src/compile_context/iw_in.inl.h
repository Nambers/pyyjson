#ifndef PYYJSON_COMPILE_CONTEXT_IW
#define PYYJSON_COMPILE_CONTEXT_IW

#include "w_in.inl.h"

// fake include and definition to deceive clangd
#ifdef PYYJSON_CLANGD_DUMMY
#    include "pyyjson.h"
#    ifndef COMPILE_INDENT_LEVEL
#        define COMPILE_INDENT_LEVEL 2
#    endif
#endif

/*
 * Basic definitions.
 */
#if COMPILE_INDENT_LEVEL == 4
#elif COMPILE_INDENT_LEVEL == 2
#elif COMPILE_INDENT_LEVEL == 0
#else
#    error "COMPILE_INDENT_LEVEL must be 0, 2 or 4"
#endif

#define __IDENT_NAME PYYJSON_SIMPLE_CONCAT2(indent, COMPILE_INDENT_LEVEL)

#define MAKE_IW_NAME(_x_) PYYJSON_CONCAT3(_x_, __IDENT_NAME, _dst_t)

/*
 * Write indents to unicode buffer. Need to reserve space before calling this function.
 */
#define write_unicode_indent MAKE_IW_NAME(write_unicode_indent)

/*
 * Write indents to unicode buffer. Will reserve space if needed.
 */
#define unicode_indent_writer MAKE_IW_NAME(unicode_indent_writer)

#endif // PYYJSON_COMPILE_CONTEXT_IW
