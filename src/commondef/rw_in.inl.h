#include "r_in.inl.h"
#include "w_in.inl.h"

#if COMPILE_READ_UCS_LEVEL == 1 && COMPILE_WRITE_UCS_LEVEL == 4
#    define SIMD_SUB_TYPE SIMD_128
#    define WR_DIV 4
#elif COMPILE_READ_UCS_LEVEL == COMPILE_WRITE_UCS_LEVEL
#    define SIMD_SUB_TYPE SIMD_TYPE
#    define WR_DIV 1
#else
#    define SIMD_SUB_TYPE SIMD_HALF_TYPE
#    define WR_DIV 2
#endif

//
#define VECTOR_ELEVATE4 PYYJSON_CONCAT3(vector_elevate4, COMPILE_READ_UCS_LEVEL, COMPILE_WRITE_UCS_LEVEL)
#if WR_DIV == 2
force_inline _WVECx4_A_ VECTOR_ELEVATE4(_VECx2_A_ src_vec);
#elif WR_DIV == 4
force_inline _WVECx4_A_ VECTOR_ELEVATE4(_VEC_A_ src_vec);
#endif
