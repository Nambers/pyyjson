#ifndef SIMD_CVT_H
#define SIMD_CVT_H


#include "simd/mask_table.h"
#include "unicode/uvector.h"


void long_back_elevate_1_2(u16 *restrict write_start, u8 *restrict read_start, Py_ssize_t len);


void long_back_elevate_1_4(u32 *restrict write_start, u8 *restrict read_start, Py_ssize_t len);


void long_back_elevate_2_4(u32 *restrict write_start, u16 *restrict read_start, Py_ssize_t len);


#endif // SIMD_CVT_H
