#ifndef INDENT_WRAP_H
#define INDENT_WRAP_H


#undef COMPILE_WRITE_UCS_LEVEL
#undef COMPILE_INDENT_LEVEL

#define COMPILE_INDENT_LEVEL 0
#define COMPILE_WRITE_UCS_LEVEL 1
#include "unicode/_indent.inl.h"
#undef COMPILE_WRITE_UCS_LEVEL
#undef COMPILE_INDENT_LEVEL

#define COMPILE_INDENT_LEVEL 0
#define COMPILE_WRITE_UCS_LEVEL 2
#include "unicode/_indent.inl.h"
#undef COMPILE_WRITE_UCS_LEVEL
#undef COMPILE_INDENT_LEVEL

#define COMPILE_INDENT_LEVEL 0
#define COMPILE_WRITE_UCS_LEVEL 4
#include "unicode/_indent.inl.h"
#undef COMPILE_WRITE_UCS_LEVEL
#undef COMPILE_INDENT_LEVEL

#define COMPILE_INDENT_LEVEL 2
#define COMPILE_WRITE_UCS_LEVEL 1
#include "unicode/_indent.inl.h"
#undef COMPILE_WRITE_UCS_LEVEL
#undef COMPILE_INDENT_LEVEL

#define COMPILE_INDENT_LEVEL 2
#define COMPILE_WRITE_UCS_LEVEL 2
#include "unicode/_indent.inl.h"
#undef COMPILE_WRITE_UCS_LEVEL
#undef COMPILE_INDENT_LEVEL

#define COMPILE_INDENT_LEVEL 2
#define COMPILE_WRITE_UCS_LEVEL 4
#include "unicode/_indent.inl.h"
#undef COMPILE_WRITE_UCS_LEVEL
#undef COMPILE_INDENT_LEVEL

#define COMPILE_INDENT_LEVEL 4
#define COMPILE_WRITE_UCS_LEVEL 1
#include "unicode/_indent.inl.h"
#undef COMPILE_WRITE_UCS_LEVEL
#undef COMPILE_INDENT_LEVEL

#define COMPILE_INDENT_LEVEL 4
#define COMPILE_WRITE_UCS_LEVEL 2
#include "unicode/_indent.inl.h"
#undef COMPILE_WRITE_UCS_LEVEL
#undef COMPILE_INDENT_LEVEL

#define COMPILE_INDENT_LEVEL 4
#define COMPILE_WRITE_UCS_LEVEL 4
#include "unicode/_indent.inl.h"
#undef COMPILE_WRITE_UCS_LEVEL
#undef COMPILE_INDENT_LEVEL


#endif // INDENT_WRAP_H
