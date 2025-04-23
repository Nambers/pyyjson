#ifndef PYYJSON_UNION_VECTOR_H
#define PYYJSON_UNION_VECTOR_H
#include "simd_impl.h"

typedef union {
    VECTOR_U8_128_A x[2];
    VECTOR_U8_256_A y;
} UnionVectorA_U8_128_x2;

typedef union {
    VECTOR_U16_128_A x[2];
    VECTOR_U16_256_A y;
} UnionVectorA_U16_128_x2;

typedef union {
    VECTOR_U32_128_A x[2];
    VECTOR_U32_256_A y;
} UnionVectorA_U32_128_x2;

typedef union {
    VECTOR_U8_128_A x[4];
    VECTOR_U8_256_A y[2];
    VECTOR_U8_512_A z;
} UnionVectorA_U8_128_x4;

typedef union {
    VECTOR_U16_128_A x[4];
    VECTOR_U16_256_A y[2];
    VECTOR_U16_512_A z;
} UnionVectorA_U16_128_x4;

typedef union {
    VECTOR_U32_128_A x[4];
    VECTOR_U32_256_A y[2];
    VECTOR_U32_512_A z;
} UnionVectorA_U32_128_x4;

typedef union {
    VECTOR_U8_256_A x[2];
    VECTOR_U8_512_A y;
} UnionVectorA_U8_256_x2;

typedef union {
    VECTOR_U16_256_A x[2];
    VECTOR_U16_512_A y;
} UnionVectorA_U16_256_x2;

typedef union {
    VECTOR_U32_256_A x[2];
    VECTOR_U32_512_A y;
} UnionVectorA_U32_256_x2;

typedef union {
    VECTOR_U8_256_A x[4];
    VECTOR_U8_512_A y[2];
    VECTOR_U8_1024_A z;
} UnionVectorA_U8_256_x4;

typedef union {
    VECTOR_U16_256_A x[4];
    VECTOR_U16_512_A y[2];
    VECTOR_U16_1024_A z;
} UnionVectorA_U16_256_x4;

typedef union {
    VECTOR_U32_256_A x[4];
    VECTOR_U32_512_A y[2];
    VECTOR_U32_1024_A z;
} UnionVectorA_U32_256_x4;

typedef union {
    VECTOR_U8_512_A x[2];
    VECTOR_U8_1024_A y;
} UnionVectorA_U8_512_x2;

typedef union {
    VECTOR_U16_512_A x[2];
    VECTOR_U16_1024_A y;
} UnionVectorA_U16_512_x2;

typedef union {
    VECTOR_U32_512_A x[2];
    VECTOR_U32_1024_A y;
} UnionVectorA_U32_512_x2;

typedef union {
    VECTOR_U8_512_A x[4];
    VECTOR_U8_1024_A y[2];
    VECTOR_U8_2048_A z;
} UnionVectorA_U8_512_x4;

typedef union {
    VECTOR_U16_512_A x[4];
    VECTOR_U16_1024_A y[2];
    VECTOR_U16_2048_A z;
} UnionVectorA_U16_512_x4;

typedef union {
    VECTOR_U32_512_A x[4];
    VECTOR_U32_1024_A y[2];
    VECTOR_U32_2048_A z;
} UnionVectorA_U32_512_x4;

#endif // PYYJSON_UNION_VECTOR_H
