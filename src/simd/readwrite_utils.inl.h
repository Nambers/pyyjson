// requires: READ, WRITE

#include "simd_impl.h"

#define WRITE_SIMD_IMPL PYYJSON_CONCAT3(write_simd_impl, COMPILE_READ_UCS_LEVEL, COMPILE_WRITE_UCS_LEVEL)


force_inline void WRITE_SIMD_IMPL(_TARGET_TYPE *dst, SIMD_TYPE SIMD_VAR) {
#if COMPILE_READ_UCS_LEVEL == COMPILE_WRITE_UCS_LEVEL
    // write as it is
    write_simd((void *)dst, SIMD_VAR);
#else
#    if COMPILE_READ_UCS_LEVEL == 1 && COMPILE_WRITE_UCS_LEVEL == 4
#        define EXTRACTOR PYYJSON_CONCAT3(extract, SIMD_BIT_SIZE, four_parts)
#        define ELEVATOR PYYJSON_CONCAT5(elevate, COMPILE_READ_UCS_LEVEL, COMPILE_WRITE_UCS_LEVEL, to, SIMD_BIT_SIZE)
    SIMD_128 x1, x2, x3, x4;
    EXTRACTOR(SIMD_VAR, &x1, &x2, &x3, &x4);
    // 0
    write_simd((void *)dst, ELEVATOR(x1));
    dst += CHECK_COUNT_MAX / 4;
    // 1
    write_simd((void *)dst, ELEVATOR(x2));
    dst += CHECK_COUNT_MAX / 4;
    // 2
    write_simd((void *)dst, ELEVATOR(x3));
    dst += CHECK_COUNT_MAX / 4;
    // 3
    write_simd((void *)dst, ELEVATOR(x4));
    dst += CHECK_COUNT_MAX / 4;
#        undef ELEVATOR
#        undef EXTRACTOR
#    else
#        define EXTRACTOR PYYJSON_CONCAT3(extract, SIMD_BIT_SIZE, two_parts)
#        define ELEVATOR PYYJSON_CONCAT5(elevate, COMPILE_READ_UCS_LEVEL, COMPILE_WRITE_UCS_LEVEL, to, SIMD_BIT_SIZE)
    SIMD_HALF_TYPE v1, v2;
    EXTRACTOR(SIMD_VAR, &v1, &v2);
    // 0
    write_simd((void *)dst, ELEVATOR(v1));
    dst += CHECK_COUNT_MAX / 2;
    // 1
    write_simd((void *)dst, ELEVATOR(v2));
    dst += CHECK_COUNT_MAX / 2;
#        undef ELEVATOR
#        undef EXTRACTOR
#    endif // r->w == 1->4
#endif
}

#undef WRITE_SIMD_IMPL
