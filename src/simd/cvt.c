#include "long_cvt.h"
//
#include "compile_feature_check.h"
//
#include "compile_context/s_in.inl.h"

void SIMD_NAME_MODIFIER(long_back_cvt_noinline_u8_u16)(u16 *restrict write_start, u8 *restrict read_start, Py_ssize_t _len) {
    usize len = _len;
    u8 *read_end = read_start + len;
    u16 *write_end = write_start + len;
    MAKE_S_NAME(long_back_cvt_u8_u16)(write_end, read_end, len);
}

void SIMD_NAME_MODIFIER(long_back_cvt_noinline_u8_u32)(u32 *restrict write_start, u8 *restrict read_start, Py_ssize_t _len) {
    usize len = _len;
    u8 *read_end = read_start + len;
    u32 *write_end = write_start + len;
    MAKE_S_NAME(long_back_cvt_u8_u32)(write_end, read_end, len);
}

void SIMD_NAME_MODIFIER(long_back_cvt_noinline_u16_u32)(u32 *restrict write_start, u16 *restrict read_start, Py_ssize_t _len) {
    usize len = _len;
    u16 *read_end = read_start + len;
    u32 *write_end = write_start + len;
    MAKE_S_NAME(long_back_cvt_u16_u32)(write_end, read_end, len);
}
