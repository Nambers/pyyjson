
#ifndef UVECTOR_H
#define UVECTOR_H


#include "pyyjson.h"


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


#define U8_WRITER(vec) (vec->head.write_u8)
#define U16_WRITER(vec) (vec->head.write_u16)
#define U32_WRITER(vec) (vec->head.write_u32)

#define GET_VEC_ASCII_START(vec) ((void *) &(vec)->unicode_rep.ascii_rep.ascii_data[0])
#define GET_VEC_COMPACT_START(vec) ((void *) &(vec)->unicode_rep.latin1_data[0])

#define VEC_END(vec) (vec->head.write_end)


force_noinline UnicodeVector *unicode_vec_reserve(UnicodeVector *vec, void *target_ptr);


force_noinline void init_py_unicode(UnicodeVector *restrict vec_addr, Py_ssize_t size, int kind);


force_inline bool vec_in_boundary(UnicodeVector *vec) {
    return vec->head.write_u8 <= (u8 *) vec->head.write_end && vec->head.write_u8 >= (u8 *) GET_VEC_ASCII_START(vec);
}

/* Resize the vector pointed by `vec_addr`.
 * If resize succeed, the vector will be updated to the new address and return true.
 * Otherwise, `vec_addr` left unchanged and returns false.
 * Args:
 *     vec_addr: The address of the vector.
 *     len: Count of valid unicode points in the vector.
 *     ucs_type: The unicode type of the vector (0 stands for ascii).
 */
force_noinline bool vector_resize_to_fit(UnicodeVector **restrict vec_addr, Py_ssize_t len, int ucs_type);


#endif
