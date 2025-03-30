// requires: READ, WRITE

#include "commondef/rw_in.inl.h"
#include "pyyjson.h"
#include "simd_impl.h"

#define DOWNGRADE_STRING PYYJSON_CONCAT3(downgrade_string, COMPILE_READ_UCS_LEVEL, COMPILE_WRITE_UCS_LEVEL)

#if PYYJSON_X86
force_inline void DOWNGRADE_STRING(const _FROM_TYPE *src_start, Py_ssize_t copy_count, _TARGET_TYPE *write_buffer_head) {
#    if COMPILE_READ_UCS_LEVEL == 2
// 2 -> 1
#        define ZIPPER zip_simd_16_to_8
#        define ZIP_WRITER write_real_half
#        define ZIPTYPE SIMD_REAL_HALF_TYPE
#    elif COMPILE_WRITE_UCS_LEVEL == 2
// 4 -> 2
#        define ZIPPER zip_simd_32_to_16
#        define ZIP_WRITER write_real_half
#        define ZIPTYPE SIMD_REAL_HALF_TYPE
#    else
// 4 -> 1
#        define ZIPPER zip_simd_32_to_8
#        define ZIP_WRITER write_real_quarter
#        define ZIPTYPE SIMD_REAL_QUARTER_TYPE
#    endif
    const _FROM_TYPE *src = (const _FROM_TYPE *)src_start;
    _TARGET_TYPE *dst = (_TARGET_TYPE *)write_buffer_head;
    SIMD_TYPE SIMD_VAR;
    usize _copy_count = (usize)copy_count;
    usize _loop = _copy_count / CHECK_COUNT_MAX;
    usize _rest = _copy_count % CHECK_COUNT_MAX;
#    if SIMD_BIT_SIZE == 512
    if (_copy_count * sizeof(_FROM_TYPE) < 32) {
        // we cannot load 64 bytes directly, use avx2 instead
#        if COMPILE_READ_UCS_LEVEL == 2
// 2 -> 1
#            define SMALL_ZIPPER zip_256_16_to_8
#            define SMALL_ZIP_WRITER write_real_quarter
#            define SMALL_ZIPTYPE SIMD_128
#        elif COMPILE_WRITE_UCS_LEVEL == 2
// 4 -> 2
#            define SMALL_ZIPPER zip_256_32_to_16
#            define SMALL_ZIP_WRITER write_real_quarter
#            define SMALL_ZIPTYPE SIMD_128
#        else
// 4 -> 1
#            define SMALL_ZIPPER zip_256_32_to_8
#            define SMALL_ZIP_WRITER(d, v) *(u64 *)(d) = (v)
#            define SMALL_ZIPTYPE u64
#        endif
        SIMD_256 y = load_256((const void *)(src + copy_count - CHECK_COUNT_MAX / 2));
        SMALL_ZIPTYPE half_val = SMALL_ZIPPER(y);
        SMALL_ZIP_WRITER(dst + copy_count - CHECK_COUNT_MAX / 2, half_val);
#        undef SMALL_ZIPTYPE
#        undef SMALL_ZIP_WRITER
#        undef SMALL_ZIPPER
        return;
    }
#    endif
    SIMD_TYPE _last_stride;
    _last_stride = load_simd((const void *)(src + copy_count - CHECK_COUNT_MAX));

    for (; _loop; _loop--) {
        SIMD_TYPE SIMD_VAR = load_simd((const void *)src);
        ZIPTYPE half_val = ZIPPER(SIMD_VAR);
        ZIP_WRITER(dst, half_val);
        // copy_count -= CHECK_COUNT_MAX;
        dst += CHECK_COUNT_MAX;
        src += CHECK_COUNT_MAX;
    }
    if (_rest) {
        ZIPTYPE half_val = ZIPPER(_last_stride);
        ZIP_WRITER(write_buffer_head + copy_count - CHECK_COUNT_MAX, half_val);
    }
#    undef ZIPTYPE
#    undef ZIP_WRITER
#    undef ZIPPER
}
#elif PYYJSON_AARCH
force_inline void DOWNGRADE_STRING(const _FROM_TYPE *src_start, Py_ssize_t copy_count, _TARGET_TYPE *write_buffer_head) {
    assert(false);
}
#endif
#undef DOWNGRADE_STRING
#include "commondef/rw_out.inl.h"
