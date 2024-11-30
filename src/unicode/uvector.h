
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


force_inline bool vec_in_boundary(UnicodeVector *vec) {
    return vec->head.write_u8 <= (u8 *) vec->head.write_end && vec->head.write_u8 >= (u8 *) GET_VEC_ASCII_START(vec);
}


#endif
