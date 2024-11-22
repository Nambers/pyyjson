#include "encode_shared.h"
#include "simd_detect.h"
#include "simd_impl.h"
#include <threads.h>

#define PYYJSON_CONCAT2_EX(a, b) a##_##b
#define PYYJSON_CONCAT2(a, b) PYYJSON_CONCAT2_EX(a, b)

#define PYYJSON_CONCAT3_EX(a, b, c) a##_##b##_##c
#define PYYJSON_CONCAT3(a, b, c) PYYJSON_CONCAT3_EX(a, b, c)

#define PYYJSON_CONCAT4_EX(a, b, c, d) a##_##b##_##c##_##d
#define PYYJSON_CONCAT4(a, b, c, d) PYYJSON_CONCAT4_EX(a, b, c, d)


typedef struct UnicodeVecHead {
    union {
        u8 *write_u8;
        u16 *write_u16;
        u32 *write_u32;
    };
    void *write_end;
} UnicodeVecHead;

static_assert(sizeof(UnicodeVecHead) <= sizeof(PyASCIIObject), "sizeof(UnicodeVecHead) <= sizeof(PyASCIIObject)");


typedef struct VecASCIIRep {
    union {
        PyASCIIObject ascii_obj;
    };
    u8 ascii_data[1];
} VecASCIIRep;


typedef struct CompactUnicodeRep {
    union {
        VecASCIIRep ascii_rep;
        PyCompactUnicodeObject compact_obj;
    };
    union {
        u8 latin1_data[1];
        u16 u16_data[1];
        u32 u32_data[1];
    };
} CompactUnicodeRep;

typedef struct UnicodeVector {
    union {
        UnicodeVecHead head;
        CompactUnicodeRep unicode_rep;
    };
} UnicodeVector;

#define GET_VEC_ASCII_START(vec) ((void *) &(vec)->unicode_rep.ascii_rep.ascii_data[0])
#define GET_VEC_COMPACT_START(vec) ((void *) &(vec)->unicode_rep.latin1_data[0])

typedef enum EncodeValJumpFlag {
    JumpFlag_Default,
    JumpFlag_ArrValBegin,
    JumpFlag_DictPairBegin,
    JumpFlag_TupleValBegin,
    JumpFlag_Elevate1_ArrVal,
    JumpFlag_Elevate1_ObjVal,
    JumpFlag_Elevate1_Key,
    JumpFlag_Elevate2_ArrVal,
    JumpFlag_Elevate2_ObjVal,
    JumpFlag_Elevate2_Key,
    JumpFlag_Elevate4_ArrVal,
    JumpFlag_Elevate4_ObjVal,
    JumpFlag_Elevate4_Key,
    JumpFlag_Fail,
} EncodeValJumpFlag;


typedef enum PyFastTypes {
    T_Unicode,
    T_Long,
    T_False,
    T_True,
    T_None,
    T_Float,
    T_List,
    T_Dict,
    T_Tuple,
    Unknown,
} PyFastTypes;

typedef struct UnicodeInfo {
    Py_ssize_t ascii_size;
    Py_ssize_t u8_size;
    Py_ssize_t u16_size;
    Py_ssize_t u32_size;
    int cur_ucs_type;
} UnicodeInfo;

typedef struct CtnType {
    PyObject *ctn;
    Py_ssize_t index;
} CtnType;

typedef struct StackVars {
    // cache
    UnicodeVector *vec;
    PyObject *key, *val;
    PyObject *cur_obj;           // = in_obj;
    Py_ssize_t cur_pos;          // = 0;
    Py_ssize_t cur_nested_depth; //= 0;
    Py_ssize_t cur_list_size;
    // alias thread local buffer
    CtnType *ctn_stack; //= obj_viewer->ctn_stack;
    UnicodeInfo unicode_info;
} StackVars;

#define GET_VEC(stack_vars) ((stack_vars)->vec)


force_inline void memorize_ascii_to_ucs4(UnicodeVector *vec, UnicodeInfo *unicode_info) {
    Py_ssize_t len = vec->head.write_u8 - (u8 *) GET_VEC_ASCII_START(vec);
    unicode_info->ascii_size = len;
    u32 *new_write_ptr = ((u32 *) GET_VEC_COMPACT_START(vec)) + len;
    vec->head.write_u32 = new_write_ptr;
    assert(unicode_info->cur_ucs_type == 0);
    unicode_info->cur_ucs_type = 4;
}

force_inline void memorize_ascii_to_ucs2(UnicodeVector *vec, UnicodeInfo *unicode_info) {
    Py_ssize_t len = vec->head.write_u8 - (u8 *) GET_VEC_ASCII_START(vec);
    unicode_info->ascii_size = len;
    u16 *new_write_ptr = ((u16 *) GET_VEC_COMPACT_START(vec)) + len;
    vec->head.write_u16 = new_write_ptr;
    assert(unicode_info->cur_ucs_type == 0);
    unicode_info->cur_ucs_type = 2;
}

force_inline void memorize_ascii_to_ucs1(UnicodeVector *vec, UnicodeInfo *unicode_info) {
    Py_ssize_t len = vec->head.write_u8 - (u8 *) GET_VEC_ASCII_START(vec);
    unicode_info->ascii_size = len;
    u8 *new_write_ptr = ((u8 *) GET_VEC_COMPACT_START(vec)) + len;
    vec->head.write_u8 = new_write_ptr;
    assert(unicode_info->cur_ucs_type == 0);
    unicode_info->cur_ucs_type = 1;
}

force_inline void memorize_ucs1_to_ucs2(UnicodeVector *vec, UnicodeInfo *unicode_info) {
    Py_ssize_t diff = vec->head.write_u8 - (u8 *) GET_VEC_COMPACT_START(vec);
    Py_ssize_t len = diff - unicode_info->ascii_size;
    assert(len >= 0);
    unicode_info->u8_size = len;
    u16 *new_write_ptr = ((u16 *) GET_VEC_COMPACT_START(vec)) + diff;
    vec->head.write_u16 = new_write_ptr;
    assert(unicode_info->cur_ucs_type == 1);
    unicode_info->cur_ucs_type = 2;
}

force_inline void memorize_ucs1_to_ucs4(UnicodeVector *vec, UnicodeInfo *unicode_info) {
    Py_ssize_t diff = vec->head.write_u8 - (u8 *) GET_VEC_COMPACT_START(vec);
    Py_ssize_t len = diff - unicode_info->ascii_size;
    assert(len >= 0);
    unicode_info->u8_size = len;
    u32 *new_write_ptr = ((u32 *) GET_VEC_COMPACT_START(vec)) + diff;
    vec->head.write_u32 = new_write_ptr;
    assert(unicode_info->cur_ucs_type == 1);
    unicode_info->cur_ucs_type = 4;
}

force_inline void memorize_ucs2_to_ucs4(UnicodeVector *vec, UnicodeInfo *unicode_info) {
    Py_ssize_t diff = vec->head.write_u16 - (u16 *) GET_VEC_COMPACT_START(vec);
    Py_ssize_t len = diff - unicode_info->ascii_size - unicode_info->u8_size;
    assert(len >= 0);
    unicode_info->u16_size = len;
    u32 *new_write_ptr = ((u32 *) GET_VEC_COMPACT_START(vec)) + diff;
    vec->head.write_u32 = new_write_ptr;
    assert(unicode_info->cur_ucs_type == 2);
    unicode_info->cur_ucs_type = 4;
}

#if SIMD_BIT_SIZE > 128
force_inline void _long_back_elevate_1_2_loop_impl(u8 *restrict read_start, u8 *restrict read_end, u16 *restrict write_end, Py_ssize_t read_once_count, SIMD_HALF_TYPE (*load_interface)(const void *), void (*write_interface)(void *, SIMD_TYPE)) {
    read_end -= read_once_count;
    write_end -= read_once_count;
    while (read_end >= read_start) {
        SIMD_HALF_TYPE half;
        SIMD_TYPE full;
        half = load_interface(read_end);
        full = PYYJSON_CONCAT2(elevate_1_2_to, SIMD_BIT_SIZE)(half);
        write_interface(write_end, full);
        read_end -= read_once_count;
        write_end -= read_once_count;
    }
}
#else
force_inline void _long_back_elevate_1_2_small_tail_1(u8 **read_end_addr, u16 **write_end_addr) {
    SIMD_128 x_read, elevated;
    *read_end_addr -= 8;
    *write_end_addr -= 8;
    x_read = broadcast_64_128(*(i64 *) *read_end_addr);
    elevated = elevate_1_2_to_128(x_read);
    write_128((void *) *write_end_addr, elevated);
}

force_inline void _long_back_elevate_2_4_small_tail_1(u16 **read_end_addr, u32 **write_end_addr) {
    SIMD_128 x_read, elevated;
    *read_end_addr -= 4;
    *write_end_addr -= 4;
    x_read = broadcast_64_128(*(i64 *) *read_end_addr);
    elevated = elevate_2_4_to_128(x_read);
    write_128((void *) *write_end_addr, elevated);
}
#endif // SIMD_BIT_SIZE

force_inline void long_back_elevate_1_2(u16 *restrict write_start, u8 *restrict read_start, Py_ssize_t len) {
    // only 128 -> 256 and 256 -> 512 should consider aligness.
    // 64(128) -> 128 cannot be aligned anyway.
    u8 *read_end = read_start + len;
    u16 *write_end = write_start + len;
#if SIMD_BIT_SIZE > 128
    const Py_ssize_t read_once_count = SIMD_BIT_SIZE / 2 / 8;
    Py_ssize_t tail_len = len & (read_once_count - 1);
    if (tail_len) {
#if SIMD_BIT_SIZE == 256
        // read and write with blendv.
        SIMD_128 x;
        SIMD_256 y, mask, blend;
        u16 *const write_tail_start = write_end - read_once_count;
        x = load_128((const void *) (read_end - read_once_count));
        y = elevate_1_2_to_256(x);
        mask = load_256_aligned((const void *) &_MaskTable_16[read_once_count - tail_len][0]);
        blend = load_256((const void *) write_tail_start);
        y = blendv_256(blend, y, mask);
        write_256((void *) write_tail_start, y);
#else
        // 512, use mask_storeu.
        SIMD_256 y;
        SIMD_512 z;
        y = load_256((const void *) (read_end - tail_len));
        z = elevate_1_2_to_512(y);
        _mm512_mask_storeu_epi16((void *) (write_end - tail_len), (1 << (usize) tail_len) - 1, z);
#endif // SIMD_BIT_SIZE
        len &= ~(read_once_count - 1);
        read_end = read_start + len;
        write_end = write_start + len;
    }
    if (0 == (((Py_ssize_t) (read_start)) & (read_once_count - 1))) {
        if (0 == (((Py_ssize_t) (write_start)) & (read_once_count * 2 - 1))) {
            goto elevate_both_aligned;
        } else {
            goto elevate_src_aligned;
        }
    } else {
        if (0 == (((Py_ssize_t) (write_start)) & (read_once_count * 2 - 1))) {
            goto elevate_dst_aligned;
        } else {
            goto elevate_both_not_aligned;
        }
    }
elevate_both_aligned:;
    _long_back_elevate_1_2_loop_impl(read_start, read_end, write_end, read_once_count, load_aligned_half, write_aligned);
    return;
elevate_src_aligned:;
    _long_back_elevate_1_2_loop_impl(read_start, read_end, write_end, read_once_count, load_aligned_half, write_simd);
    return;
elevate_dst_aligned:;
    _long_back_elevate_1_2_loop_impl(read_start, read_end, write_end, read_once_count, load_half, write_aligned);
    return;
elevate_both_not_aligned:;
    _long_back_elevate_1_2_loop_impl(read_start, read_end, write_end, read_once_count, load_half, write_simd);
    return;
#else // SIMD_BIT_SIZE == 128
    SIMD_128 x_read;
    SIMD_128 x;
    // 16 bytes as a block.
    Py_ssize_t tail_len = len & (Py_ssize_t) 15;
    if (tail_len) {
        usize tail_parts = (usize) tail_len >> 3;
        usize small_tail_len = (usize) tail_len & 7;
        for (usize i = 0; i < small_tail_len; ++i) {
            *--write_end = *--read_end;
        }
        if (tail_parts) {
            // 64 -> 128.
            _long_back_elevate_1_2_small_tail_1(&read_end, &write_end);
        }
        len &= ~(Py_ssize_t) 15;
    }
    read_end -= 16;
    write_end -= 16;
    while (read_end >= read_start) {
        x_read = load_128((const void *) read_end);
        x = elevate_1_2_to_128(x_read);
        write_128((void *) write_end, x);
        x_read = unpack_hi_64_128(x_read, x_read);
        x = elevate_1_2_to_128(x_read);
        write_128((void *) (write_end + 8), x);
        read_end -= 16;
        write_end -= 16;
    }
#endif
}

// long_back_elevate_1_4 tool
#if SIMD_BIT_SIZE == 512
force_inline void _long_back_elevate_1_4_loop_impl(u8 *restrict read_start, u8 *restrict read_end, u32 *restrict write_end, Py_ssize_t read_once_count, SIMD_128 (*load_interface)(const void *), void (*write_interface)(void *, SIMD_TYPE)) {
    SIMD_128 small;
    SIMD_512 full;
    read_end -= read_once_count;
    write_end -= read_once_count;
    while (read_end >= read_start) {
        small = load_interface((const void *) read_end);
        full = elevate_1_4_to_512(small);
        write_interface((void *) write_end, full);
        read_end -= read_once_count;
        write_end -= read_once_count;
    }
}
#endif

force_inline void _long_back_elevate_1_4_small_tail_1(u8 **restrict read_end_addr, u32 **restrict write_end_addr) {
    SIMD_128 x_read, elevated;
    *read_end_addr -= 4;
    *write_end_addr -= 4;
    x_read = broadcast_32_128(*(i32 *) *read_end_addr);
    elevated = elevate_1_4_to_128(x_read);
    write_128((void *) *write_end_addr, elevated);
}

force_inline void _long_back_elevate_1_4_small_tail_2(u8 **restrict read_end_addr, u32 **restrict write_end_addr) {
#if SIMD_BIT_SIZE == 256
    // 64 -> 256.
    SIMD_128 x_read;
    SIMD_256 elevated;
    *read_end_addr -= 8;
    *write_end_addr -= 8;
    x_read = broadcast_64_128(*(i64 *) *read_end_addr);
    elevated = elevate_1_4_to_256(x_read);
    write_256((void *) *write_end_addr, elevated);
#else
    SIMD_128 x_read, elevated;
    *read_end_addr -= 8;
    *write_end_addr -= 8;
    i32 *read_i32_ptr = (i32 *) *read_end_addr;
    // TODO check this assembly
    x_read = set_32_128(*read_i32_ptr, 0, *(read_i32_ptr + 1), 0);
    elevated = elevate_1_4_to_128(x_read);
    write_128((void *) *write_end_addr, elevated);
    x_read = unpack_hi_64_128(x_read, x_read);
    elevated = elevate_1_4_to_128(x_read);
    write_128((void *) ((*write_end_addr) + 4), elevated);
#endif
}

force_inline void long_back_elevate_1_4(u32 *restrict write_start, u8 *restrict read_start, Py_ssize_t len) {
    // only 128 -> 512 should consider aligness.
    // 32/64(128) -> 128/256 cannot be aligned anyway.
    u8 *read_end = read_start + len;
    u32 *write_end = write_start + len;
#if SIMD_BIT_SIZE == 512
    const Py_ssize_t read_once_count = SIMD_BIT_SIZE / 4 / 8;
    Py_ssize_t tail_len = len & (read_once_count - 1);
    if (tail_len) {
        SIMD_128 x;
        SIMD_512 z;
        x = load_128((const void *) (read_end - tail_len));
        z = elevate_1_4_to_512(x);
        _mm512_mask_storeu_epi32((void *) (write_end - tail_len), ((__mmask16) 1 << (usize) tail_len) - 1, z);
        len &= ~(read_once_count - 1);
        read_end = read_start + len;
        write_end = write_start + len;
    }
    if (0 == (((Py_ssize_t) (read_start)) & (128 / 8 - 1))) {
        if (0 == (((Py_ssize_t) (write_start)) & (512 / 8 - 1))) {
            goto elevate_both_aligned;
        } else {
            goto elevate_src_aligned;
        }
    } else {
        if (0 == (((Py_ssize_t) (write_start)) & (512 / 8 - 1))) {
            goto elevate_dst_aligned;
        } else {
            goto elevate_both_not_aligned;
        }
    }
elevate_both_aligned:;
    _long_back_elevate_1_4_loop_impl(read_start, read_end, write_end, read_once_count, load_128_aligned, write_aligned);
    return;
elevate_src_aligned:;
    _long_back_elevate_1_4_loop_impl(read_start, read_end, write_end, read_once_count, load_128_aligned, write_simd);
    return;
elevate_dst_aligned:;
    _long_back_elevate_1_4_loop_impl(read_start, read_end, write_end, read_once_count, load_128, write_aligned);
    return;
elevate_both_not_aligned:;
    _long_back_elevate_1_4_loop_impl(read_start, read_end, write_end, read_once_count, load_128, write_simd);
    return;
#else
    SIMD_128 x_read;
    SIMD_TYPE SIMD_VAR;
    // 16 bytes as a block.
    Py_ssize_t tail_len = len & (Py_ssize_t) 15;
    if (tail_len) {
        usize tail_parts = (usize) tail_len >> 2;
        usize small_tail_len = (usize) tail_len & 3;
        for (usize i = 0; i < small_tail_len; ++i) {
            *--write_end = *--read_end;
        }
        switch (tail_parts) {
            case 0: {
                // nothing.
                break;
            }
            case 1: {
                // 32 -> 128.
                _long_back_elevate_1_4_small_tail_1(&read_end, &write_end);
                break;
            }
            case 2: {
                _long_back_elevate_1_4_small_tail_2(&read_end, &write_end);
                break;
            }
            case 3: {
                // 3 = 1 + 2, so...
                _long_back_elevate_1_4_small_tail_1(&read_end, &write_end);
                _long_back_elevate_1_4_small_tail_2(&read_end, &write_end);
                break;
            }
            default: {
                Py_UNREACHABLE();
                assert(false);
                break;
            }
        }

        len &= ~(Py_ssize_t) 15;
    }
    read_end -= 16;
    write_end -= 16;
    while (read_end >= read_start) {
        x_read = load_128((const void *) read_end);
#if SIMD_BIT_SIZE == 256
        SIMD_VAR = elevate_1_4_to_256(x_read);
        write_256((void *) write_end, SIMD_VAR);
        x_read = unpack_hi_64_128(x_read, x_read);
        SIMD_VAR = elevate_1_4_to_256(x_read);
        write_256((void *) (write_end + 8), SIMD_VAR);
#else
        SIMD_VAR = elevate_1_4_to_128(x_read);
        write_128((void *) write_end, SIMD_VAR);
        RIGHT_SHIFT_128BITS(x_read, 32, &x_read);
        SIMD_VAR = elevate_1_4_to_128(x_read);
        write_128((void *) (write_end + 4), SIMD_VAR);
        RIGHT_SHIFT_128BITS(x_read, 32, &x_read);
        SIMD_VAR = elevate_1_4_to_128(x_read);
        write_128((void *) (write_end + 8), SIMD_VAR);
        RIGHT_SHIFT_128BITS(x_read, 32, &x_read);
        SIMD_VAR = elevate_1_4_to_128(x_read);
        write_128((void *) (write_end + 12), SIMD_VAR);
#endif
        read_end -= 16;
        write_end -= 16;
    }
#endif
}

#if SIMD_BIT_SIZE > 128
force_inline void _long_back_elevate_2_4_loop_impl(u16 *restrict read_start, u16 *restrict read_end, u32 *restrict write_end, Py_ssize_t read_once_count, SIMD_HALF_TYPE (*load_interface)(const void *), void (*write_interface)(void *, SIMD_TYPE)) {
    read_end -= read_once_count;
    write_end -= read_once_count;
    while (read_end >= read_start) {
        SIMD_HALF_TYPE half;
        SIMD_TYPE full;
        half = load_interface(read_end);
        full = PYYJSON_CONCAT2(elevate_2_4_to, SIMD_BIT_SIZE)(half);
        write_interface(write_end, full);
        read_end -= read_once_count;
        write_end -= read_once_count;
    }
}
#endif // SIMD_BIT_SIZE > 128

force_inline void long_back_elevate_2_4(u32 *restrict write_start, u16 *restrict read_start, Py_ssize_t len) {
    // TODO
    // only 128 -> 256 and 256 -> 512 should consider aligness.
    // 64(128) -> 128 cannot be aligned anyway.
    u16 *read_end = read_start + len;
    u32 *write_end = write_start + len;
#if SIMD_BIT_SIZE > 128
    const Py_ssize_t read_once_count = SIMD_BIT_SIZE / 4 / 8;
    Py_ssize_t tail_len = len & (read_once_count - 1);
    if (tail_len) {
#if SIMD_BIT_SIZE == 256
        // read and write with blendv.
        SIMD_128 x;
        SIMD_256 y, mask, blend;
        u32 *const write_tail_start = write_end - read_once_count;
        x = load_128((const void *) (read_end - read_once_count));
        y = elevate_2_4_to_256(x);
        mask = load_256_aligned((const void *) &_MaskTable_32[read_once_count - tail_len][0]);
        blend = load_256((const void *) write_tail_start);
        y = blendv_256(blend, y, mask);
        write_256((void *) write_tail_start, y);
#else
        // 512, use mask_storeu.
        SIMD_256 y;
        SIMD_512 z;
        y = load_256((const void *) (read_end - tail_len));
        z = elevate_2_4_to_512(y);
        _mm512_mask_storeu_epi32((void *) (write_end - tail_len), (1 << (usize) tail_len) - 1, z);
#endif // SIMD_BIT_SIZE
        len &= ~(read_once_count - 1);
        read_end = read_start + len;
        write_end = write_start + len;
    }
    if (0 == (((Py_ssize_t) (read_start)) & (read_once_count * 2 - 1))) {
        if (0 == (((Py_ssize_t) (write_start)) & (read_once_count * 4 - 1))) {
            goto elevate_both_aligned;
        } else {
            goto elevate_src_aligned;
        }
    } else {
        if (0 == (((Py_ssize_t) (write_start)) & (read_once_count * 4 - 1))) {
            goto elevate_dst_aligned;
        } else {
            goto elevate_both_not_aligned;
        }
    }
elevate_both_aligned:;
    _long_back_elevate_2_4_loop_impl(read_start, read_end, write_end, read_once_count, load_aligned_half, write_aligned);
    return;
elevate_src_aligned:;
    _long_back_elevate_2_4_loop_impl(read_start, read_end, write_end, read_once_count, load_aligned_half, write_simd);
    return;
elevate_dst_aligned:;
    _long_back_elevate_2_4_loop_impl(read_start, read_end, write_end, read_once_count, load_half, write_aligned);
    return;
elevate_both_not_aligned:;
    _long_back_elevate_2_4_loop_impl(read_start, read_end, write_end, read_once_count, load_half, write_simd);
    return;
#else // SIMD_BIT_SIZE == 128
    SIMD_128 x_read;
    SIMD_128 x;
    // 16 bytes as a block. i.e. 8 WORDs
    Py_ssize_t tail_len = len & (Py_ssize_t) 7;
    if (tail_len) {
        usize tail_parts = (usize) tail_len >> 2;
        usize small_tail_len = (usize) tail_len & 3;
        for (usize i = 0; i < small_tail_len; ++i) {
            *--write_end = *--read_end;
        }
        if (tail_parts) {
            // 64 -> 128.
            _long_back_elevate_2_4_small_tail_1(&read_end, &write_end);
        }
        len &= ~(Py_ssize_t) 7;
    }
    read_end -= 8;
    write_end -= 8;
    while (read_end >= read_start) {
        x_read = load_128((const void *) read_end);
        x = elevate_2_4_to_128(x_read);
        write_128((void *) write_end, x);
        x_read = unpack_hi_64_128(x_read, x_read);
        x = elevate_2_4_to_128(x_read);
        write_128((void *) (write_end + 4), x);
        read_end -= 8;
        write_end -= 8;
    }
#endif
}

force_inline void ascii_elevate2(UnicodeVector *vec, UnicodeInfo *unicode_info) {
    u8 *start = ((u8 *) GET_VEC_ASCII_START(vec));
    u16 *write_start = ((u16 *) GET_VEC_COMPACT_START(vec));
    long_back_elevate_1_2(write_start, start, unicode_info->ascii_size);
}

force_inline void ascii_elevate4(UnicodeVector *vec, UnicodeInfo *unicode_info) {
    u8 *start = ((u8 *) GET_VEC_ASCII_START(vec));
    u32 *write_start = ((u32 *) GET_VEC_COMPACT_START(vec));
    long_back_elevate_1_4(write_start, start, unicode_info->ascii_size);
}

force_inline void ucs1_elevate2(UnicodeVector *vec, UnicodeInfo *unicode_info) {
    Py_ssize_t offset = unicode_info->ascii_size;
    u8 *start = ((u8 *) GET_VEC_COMPACT_START(vec)) + offset;
    u16 *write_start = ((u16 *) GET_VEC_COMPACT_START(vec)) + offset;
    long_back_elevate_1_2(write_start, start, unicode_info->u8_size);
}

force_inline void ucs1_elevate4(UnicodeVector *vec, UnicodeInfo *unicode_info) {
    Py_ssize_t offset = unicode_info->ascii_size;
    u8 *start = ((u8 *) GET_VEC_COMPACT_START(vec)) + offset;
    u32 *write_start = ((u32 *) GET_VEC_COMPACT_START(vec)) + offset;
    long_back_elevate_1_4(write_start, start, unicode_info->u8_size);
}

force_inline void ucs2_elevate4(UnicodeVector *vec, UnicodeInfo *unicode_info) {
    Py_ssize_t offset = unicode_info->ascii_size + unicode_info->u8_size;
    u16 *start = ((u16 *) GET_VEC_COMPACT_START(vec)) + offset;
    u32 *write_start = ((u32 *) GET_VEC_COMPACT_START(vec)) + offset;
    long_back_elevate_2_4(write_start, start, unicode_info->u16_size);
}

force_inline void ascii_elevate1(UnicodeVector *vec, UnicodeInfo *unicode_info) {
    memmove(GET_VEC_COMPACT_START(vec), GET_VEC_ASCII_START(vec), unicode_info->ascii_size);
}

thread_local CtnType __tls_ctn_stack[PYYJSON_ENCODE_MAX_RECURSION];

force_inline bool init_stack_vars(StackVars *stack_vars, PyObject *in_obj) {
    stack_vars->cur_obj = in_obj;
    stack_vars->cur_pos = 0;
    stack_vars->cur_nested_depth = 0;
    stack_vars->ctn_stack = __tls_ctn_stack;
    if (unlikely(!stack_vars->ctn_stack)) {
        stack_vars->vec = NULL; // avoid freeing a wild pointer
        PyErr_NoMemory();
        return false;
    }
    memset(&stack_vars->unicode_info, 0, sizeof(UnicodeInfo));
    GET_VEC(stack_vars) = PyObject_Malloc(PYYJSON_ENCODE_DST_BUFFER_INIT_SIZE);
#ifndef NDEBUG
    memset(GET_VEC(stack_vars), 0, PYYJSON_ENCODE_DST_BUFFER_INIT_SIZE);
#endif
    if (likely(GET_VEC(stack_vars))) {
        GET_VEC(stack_vars)->head.write_u8 = (u8 *) (((PyASCIIObject *) GET_VEC(stack_vars)) + 1);
        GET_VEC(stack_vars)->head.write_end = (void *) ((u8 *) (GET_VEC(stack_vars)) + PYYJSON_ENCODE_DST_BUFFER_INIT_SIZE);
        return true;
    }
    PyErr_NoMemory();
    return false;
}

force_inline bool vector_resize_to_fit(UnicodeVector **restrict vec_addr, Py_ssize_t len, int ucs_type) {
    Py_ssize_t char_size = ucs_type ? ucs_type : 1;
    Py_ssize_t struct_size = ucs_type ? sizeof(PyCompactUnicodeObject) : sizeof(PyASCIIObject);
    assert(len <= ((PY_SSIZE_T_MAX - struct_size) / char_size - 1));
    // Resizes to a smaller size. It *should* always success
    UnicodeVector *new_vec = (UnicodeVector *) PyObject_Realloc(*vec_addr, struct_size + (len + 1) * char_size);
    if (likely(new_vec)) {
        *vec_addr = new_vec;
        return true;
    }
    return false;
}

// _PyUnicode_CheckConsistency is hidden in Python 3.13
#if PY_MINOR_VERSION >= 13
extern int _PyUnicode_CheckConsistency(PyObject *op, int check_content);
#endif

force_inline void init_py_unicode(UnicodeVector **restrict vec_addr, Py_ssize_t size, int kind) {
    UnicodeVector *vec = *vec_addr;
    PyCompactUnicodeObject *unicode = &vec->unicode_rep.compact_obj;
    PyASCIIObject *ascii = &vec->unicode_rep.ascii_rep.ascii_obj;
    PyObject_Init((PyObject *) unicode, &PyUnicode_Type);
    void *data = kind ? GET_VEC_COMPACT_START(vec) : GET_VEC_ASCII_START(vec);
    //
    ascii->length = size;
    ascii->hash = -1;
    ascii->state.interned = 0;
    ascii->state.kind = kind ? kind : 1;
    ascii->state.compact = 1;
    ascii->state.ascii = kind ? 0 : 1;

#if PY_MINOR_VERSION >= 12
    // statically_allocated appears in 3.12
    ascii->state.statically_allocated = 0;
#else
    bool is_sharing = false;
    // ready is dropped in 3.12
    ascii->state.ready = 1;
#endif

    if (kind <= 1) {
        ((u8 *) data)[size] = 0;
    } else if (kind == 2) {
        ((u16 *) data)[size] = 0;
#if PY_MINOR_VERSION < 12
        is_sharing = sizeof(wchar_t) == 2;
#endif
    } else if (kind == 4) {
        ((u32 *) data)[size] = 0;
#if PY_MINOR_VERSION < 12
        is_sharing = sizeof(wchar_t) == 4;
#endif
    } else {
        assert(false);
        Py_UNREACHABLE();
    }
    if (kind) {
        unicode->utf8 = NULL;
        unicode->utf8_length = 0;
    }
#if PY_MINOR_VERSION < 12
    if (kind > 1) {
        if (is_sharing) {
            unicode->wstr_length = size;
            ascii->wstr = (wchar_t *) data;
        } else {
            unicode->wstr_length = 0;
            ascii->wstr = NULL;
        }
    } else {
        ascii->wstr = NULL;
        if (kind) unicode->wstr_length = 0;
    }
#endif
    assert(_PyUnicode_CheckConsistency((PyObject *) unicode, 0));
    assert(ascii->ob_base.ob_refcnt == 1);
}

#define _CTN_STACK(stack_vars) ((void) 0, stack_vars->ctn_stack)

#if PY_MINOR_VERSION >= 13
// _PyNone_Type is hidden in Python 3.13
static PyTypeObject *PyNone_Type = NULL;
void _init_PyNone_Type(PyTypeObject *none_type) {
    PyNone_Type = none_type;
}
#else
#define PyNone_Type &_PyNone_Type
#endif

force_inline PyFastTypes fast_type_check(PyObject *val) {
    PyTypeObject *type = Py_TYPE(val);
    if (type == &PyUnicode_Type) {
        return T_Unicode;
    } else if (type == &PyLong_Type) {
        return T_Long;
    } else if (type == &PyBool_Type) {
        if (val == Py_False) {
            return T_False;
        } else {
            assert(val == Py_True);
            return T_True;
        }
    } else if (type == PyNone_Type) {
        return T_None;
    } else if (type == &PyFloat_Type) {
        return T_Float;
    } else if (type == &PyList_Type) {
        return T_List;
    } else if (type == &PyDict_Type) {
        return T_Dict;
    } else if (type == &PyTuple_Type) {
        return T_Tuple;
    } else {
        return Unknown;
    }
}

#define RETURN_ON_UNLIKELY_ERR(x) \
    do {                          \
        if (unlikely((x))) {      \
            return false;         \
        }                         \
    } while (0)

#define U32_WRITER(vec) (vec->head.write_u32)
#define U16_WRITER(vec) (vec->head.write_u16)
#define U8_WRITER(vec) (vec->head.write_u8)


#define VEC_END(vec) (vec->head.write_end)
#define VEC_MEM_U8_DIFF(vec, ptr) (Py_ssize_t) ptr - (Py_ssize_t) vec

#define TAIL_PADDING (512 / 8)

force_inline Py_ssize_t get_indent_char_count(Py_ssize_t cur_nested_depth, Py_ssize_t indent_level) {
    return indent_level ? (indent_level * cur_nested_depth + 1) : 0;
}

force_inline void vec_set_rwptr_and_size(UnicodeVector *vec, Py_ssize_t rw_diff, Py_ssize_t target_size) {
    vec->head.write_u8 = (u8 *) (((Py_ssize_t) vec) + rw_diff);
    vec->head.write_end = (void *) (((Py_ssize_t) vec) + target_size);
}

force_inline bool vec_in_boundary(UnicodeVector *vec) {
    return vec->head.write_u8 <= (u8 *) vec->head.write_end && vec->head.write_u8 >= (u8 *) GET_VEC_ASCII_START(vec);
}

/* 
 * Some utility functions only related to *write*, like vector reserve, writing number
 * need macro: COMPILE_WRITE_UCS_LEVEL, value: 1, 2, or 4
 */
#include "encode_utils_impl_wrap.inl"

#include "encode_simd_utils_wrap.inl"

#include "encode_unicode_impl_wrap.inl"

#include "encode_impl_wrap.inl"


force_inline PyObject *pyyjson_dumps_single_unicode(PyObject *unicode) {
    UnicodeVector *vec = PyObject_Malloc(PYYJSON_ENCODE_DST_BUFFER_INIT_SIZE);
    if (unlikely(!vec)) {
        return PyErr_NoMemory();
    }
    Py_ssize_t len = PyUnicode_GET_LENGTH(unicode);
    int unicode_kind = PyUnicode_KIND(unicode);
    bool is_ascii = PyUnicode_IS_ASCII(unicode);
    if (is_ascii) {
        U8_WRITER(vec) = (u8 *) (((PyASCIIObject *) vec) + 1);
    } else {
        U8_WRITER(vec) = (u8 *) (((PyCompactUnicodeObject *) vec) + 1);
    }
    void *write_start = (void *) U8_WRITER(vec);
    vec->head.write_end = (void *) ((u8 *) vec + PYYJSON_ENCODE_DST_BUFFER_INIT_SIZE);
    bool success;
    switch (unicode_kind) {
        // pass is_in_obj = true to avoid unwanted indent check
        case 1: {
            success = vec_write_str_0_1_1(unicode, len, &vec, true, 0);
            if (success) vec_back1_1(vec);
            break;
        }
        case 2: {
            success = vec_write_str_0_2_2(unicode, len, &vec, true, 0);
            if (success) vec_back1_2(vec);
            break;
        }
        case 4: {
            success = vec_write_str_0_4_4(unicode, len, &vec, true, 0);
            if (success) vec_back1_4(vec);
            break;
        }
        default: {
            Py_UNREACHABLE();
            assert(false);
        }
    }
    if (unlikely(!success)) {
        PyObject_Free(vec);
        return NULL;
    }
    Py_ssize_t written_len = (Py_ssize_t) U8_WRITER(vec) - (Py_ssize_t) write_start;
    written_len /= unicode_kind;
    assert(written_len >= 2);
    success = vector_resize_to_fit(&vec, written_len, is_ascii ? 0 : unicode_kind);
    if (unlikely(!success)) {
        PyObject_Free(vec);
        return NULL;
    }
    init_py_unicode(&vec, written_len, is_ascii ? 0 : unicode_kind);
    return (PyObject *) vec;
}

force_inline PyObject *pyyjson_dumps_single_float(PyObject *val) {
    u8 buffer[32];
    double v = PyFloat_AS_DOUBLE(val);
    u64 *raw = (u64 *) &v;
    u8 *buffer_end = write_f64_raw(buffer, *raw);
    size_t size = buffer_end - buffer;
    PyObject *unicode = PyUnicode_New(size, 127);
    if (unlikely(!unicode)) return NULL;
    // assert(unicode);
    char *write_pos = (char *) (((PyASCIIObject *) unicode) + 1);
    memcpy((void *) write_pos, buffer, size);
    write_pos[size] = 0;
    return unicode;
}

force_noinline PyObject *pyyjson_Encode(PyObject *self, PyObject *args, PyObject *kwargs) {
    PyObject *obj;
    int option_digit = 0;
    usize indent = 0;
    PyObject *ret;
    static const char *kwlist[] = {"obj", "options", NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O|i", (char **) kwlist, &obj, &option_digit)) {
        PyErr_SetString(PyExc_TypeError, "Invalid argument");
        goto fail;
    }

    assert(obj);

    PyFastTypes fast_type = fast_type_check(obj);

    switch (fast_type) {
        case T_List:
        case T_Dict:
        case T_Tuple: {
            goto dumps_container;
        }
        case T_Unicode: {
            goto dumps_unicode;
        }
        case T_Long: {
            goto dumps_long;
        }
        case T_False:
        case T_True:
        case T_None: {
            goto dumps_constant;
        }
        case T_Float: {
            goto dumps_float;
        }
        default: {
            PyErr_SetString(JSONEncodeError, "Unsupported type to encode");
            goto fail;
        }
    }


dumps_container:;
    if (option_digit & 1) {
        if (option_digit & 2) {
            PyErr_SetString(PyExc_ValueError, "Cannot mix indent options");
            goto fail;
        }
        indent = 2;
    } else if (option_digit & 2) {
        indent = 4;
    }

    switch (indent) {
        case 0: {
            ret = pyyjson_dumps_obj_0_0(obj);
            break;
        }
        case 2: {
            ret = pyyjson_dumps_obj_2_0(obj);
            break;
        }
        case 4: {
            ret = pyyjson_dumps_obj_4_0(obj);
            break;
        }
        default: {
            Py_UNREACHABLE();
            assert(false);
        }
    }

    if (unlikely(!ret)) {
        if (!PyErr_Occurred()) {
            PyErr_SetString(JSONEncodeError, "Failed to decode JSON: unknown error");
        }
    }

    assert(!ret || ret->ob_refcnt == 1);

    goto success;

dumps_unicode:;
    return pyyjson_dumps_single_unicode(obj);
dumps_long:;
dumps_constant:;
    goto fail; // TODO
dumps_float:;
    return pyyjson_dumps_single_float(obj);
success:;
    return ret;
fail:;
    return NULL;
}
