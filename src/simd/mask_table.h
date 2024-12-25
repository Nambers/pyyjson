#ifndef SIMD_MASK_TABLE_H
#define SIMD_MASK_TABLE_H


#include "pyyjson.h"


extern pyyjson_align(64) const u8 _MaskTable_8[64][64];


force_inline const void *read_tail_mask_table_8(Py_ssize_t row) {
    return (void *) &_MaskTable_8[row][0];
}

force_inline const void *read_tail_mask_table_16(Py_ssize_t row) {
    return (void *) &_MaskTable_8[2 * row][0];
}

force_inline const void *read_tail_mask_table_32(Py_ssize_t row) {
    return (void *) &_MaskTable_8[4 * row][0];
}


#endif // SIMD_MASK_TABLE_H
