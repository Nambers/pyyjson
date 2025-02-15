#define COMPILE_READ_UCS_LEVEL 4
#define COMPILE_WRITE_UCS_LEVEL 2
#include "commondef/rw_in.inl.h"
//
#include "downgrade.inl.h"
//
#include "commondef/rw_out.inl.h"
#undef COMPILE_WRITE_UCS_LEVEL
#undef COMPILE_READ_UCS_LEVEL

#define COMPILE_READ_UCS_LEVEL 4
#define COMPILE_WRITE_UCS_LEVEL 1
#include "commondef/rw_in.inl.h"
//
#include "downgrade.inl.h"
//
#include "commondef/rw_out.inl.h"
#undef COMPILE_WRITE_UCS_LEVEL
#undef COMPILE_READ_UCS_LEVEL

#define COMPILE_READ_UCS_LEVEL 2
#define COMPILE_WRITE_UCS_LEVEL 1
#include "commondef/rw_in.inl.h"
//
#include "downgrade.inl.h"
//
#include "commondef/rw_out.inl.h"
#undef COMPILE_WRITE_UCS_LEVEL
#undef COMPILE_READ_UCS_LEVEL
