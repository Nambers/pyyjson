#ifndef SIMD_MASK_TABLE_H
#define SIMD_MASK_TABLE_H

#include "pyyjson.h"


extern pyyjson_align(64) const u8 _TailmaskTable_8[64][64];
extern pyyjson_align(64) const u8 _HeadmaskTable_8[64][64];

/*==============================================================================
 * Read mask from tail mask table.
 * `read_tail_mask_table_x` gives `row` zeroes in the front of the mask.
 *============================================================================*/

/* Read tail mask of u8. */
force_inline const void *read_tail_mask_table_8(Py_ssize_t row) {
    return (void *)&_TailmaskTable_8[row][0];
}

/* Read tail mask of u16. */
force_inline const void *read_tail_mask_table_16(Py_ssize_t row) {
    return (void *)&_TailmaskTable_8[2 * row][0];
}

/* Read tail mask of u32. */
force_inline const void *read_tail_mask_table_32(Py_ssize_t row) {
    return (void *)&_TailmaskTable_8[4 * row][0];
}

/* Read head mask of u8. */
force_inline const void *read_head_mask_table_8(Py_ssize_t row) {
    return (void *)&_HeadmaskTable_8[row][0];
}

/* Read head mask of u16. */
force_inline const void *read_head_mask_table_16(Py_ssize_t row) {
    return (void *)&_HeadmaskTable_8[2 * row][0];
}

/* Read head mask of u32. */
force_inline const void *read_head_mask_table_32(Py_ssize_t row) {
    return (void *)&_HeadmaskTable_8[4 * row][0];
}

#endif // SIMD_MASK_TABLE_H
