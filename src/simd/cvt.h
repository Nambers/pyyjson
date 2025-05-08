#ifndef SIMD_CVT_H
#define SIMD_CVT_H

#include "simd/mask_table.h"
#include "simd_detect.h"
#include "unicode/unicode_buffer.h"


void SIMD_NAME_MODIFIER(long_back_cvt_noinline_u8_u16)(u16 *restrict write_start, u8 *restrict read_start, Py_ssize_t len);


void SIMD_NAME_MODIFIER(long_back_cvt_noinline_u8_u32)(u32 *restrict write_start, u8 *restrict read_start, Py_ssize_t len);


void SIMD_NAME_MODIFIER(long_back_cvt_noinline_u16_u32)(u32 *restrict write_start, u16 *restrict read_start, Py_ssize_t len);


#endif // SIMD_CVT_H
