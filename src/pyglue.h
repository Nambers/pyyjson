#ifndef PYYJSON_GLUE_H
#define PYYJSON_GLUE_H
#ifdef _DEBUG
#undef _DEBUG
#include <Python.h>
#define _DEBUG
#else
#include <Python.h>
#endif
#include <assert.h>
#include <stdalign.h>
#include <stdbool.h>
#include <stdint.h>
// hot spot:
// string -> float,int->array,dict->null,false,true. least: uint, nan, inf
#define PYYJSON_NO_OP (0)
#define PYYJSON_OP_STRING (1 << 0)
#define PYYJSON_OP_NUMBER (1 << 1)
#define PYYJSON_OP_CONTAINER (1 << 2)
#define PYYJSON_OP_CONSTANTS (1 << 3)
#define PYYJSON_OP_NAN_INF (1 << 4)
#define PYYJSON_OP_COUNT_MAX 5
#define PYYJSON_OP_MASK ((1 << PYYJSON_OP_COUNT_MAX) - 1)
//
#define PYYJSON_OP_HIGHER_START PYYJSON_OP_COUNT_MAX
//string flags
#define PYYJSON_STRING_FLAG_ASCII 0
#define PYYJSON_STRING_FLAG_LATIN1 (1 << (PYYJSON_OP_HIGHER_START))
#define PYYJSON_STRING_FLAG_UCS2 (1 << (PYYJSON_OP_HIGHER_START + 1))
#define PYYJSON_STRING_FLAG_UCS4 (1 << (PYYJSON_OP_HIGHER_START + 2))
#define PYYJSON_STRING_FLAG_MASK (PYYJSON_STRING_FLAG_ASCII | PYYJSON_STRING_FLAG_LATIN1 | PYYJSON_STRING_FLAG_UCS2 | PYYJSON_STRING_FLAG_UCS4)
// num flags
#define PYYJSON_NUM_FLAG_FLOAT 0
#define PYYJSON_NUM_FLAG_INT (1 << (PYYJSON_OP_HIGHER_START))
#define PYYJSON_NUM_FLAG_UINT (1 << (PYYJSON_OP_HIGHER_START + 1))
#define PYYJSON_NUM_FLAG_MASK (PYYJSON_NUM_FLAG_FLOAT | PYYJSON_NUM_FLAG_INT | PYYJSON_NUM_FLAG_UINT)
// container flags
#define PYYJSON_CONTAINER_FLAG_ARRAY 0
#define PYYJSON_CONTAINER_FLAG_DICT (1 << (PYYJSON_OP_HIGHER_START))
#define PYYJSON_CONTAINER_FLAG_MASK (PYYJSON_CONTAINER_FLAG_ARRAY | PYYJSON_CONTAINER_FLAG_DICT)
// constants flags
#define PYYJSON_CONSTANTS_FLAG_NULL 0
#define PYYJSON_CONSTANTS_FLAG_FALSE (1 << (PYYJSON_OP_HIGHER_START))
#define PYYJSON_CONSTANTS_FLAG_TRUE (1 << (PYYJSON_OP_HIGHER_START + 1))
#define PYYJSON_CONSTANTS_FLAG_MASK (PYYJSON_CONSTANTS_FLAG_NULL | PYYJSON_CONSTANTS_FLAG_FALSE | PYYJSON_CONSTANTS_FLAG_TRUE)
// nan inf flags
#define PYYJSON_NAN_INF_FLAG_NAN 0
#define PYYJSON_NAN_INF_FLAG_INF (1 << (PYYJSON_OP_HIGHER_START))
#define PYYJSON_NAN_INF_FLAG_MASK (PYYJSON_NAN_INF_FLAG_NAN | PYYJSON_NAN_INF_FLAG_INF)
// end flags
typedef uint32_t op_type;
#define PYYJSON_OP_HEAD pyyjson_op_base op_base;
#define PYYJSON_READ_OP(_op) (((pyyjson_op_base *) (_op))->op)
#define PYYJSON_WRITE_OP(_ptr, _code)               \
    do {                                            \
        ((pyyjson_op_base *) (_ptr))->op = (_code); \
    } while (0)

// size = 4
typedef struct pyyjson_op_base {
    op_type op;
} pyyjson_op_base;

// size = 8
typedef struct pyyjson_op {
    PYYJSON_OP_HEAD
    char pad[4];
} pyyjson_op;

// size = 16
typedef struct pyyjson_number_op {
    PYYJSON_OP_HEAD
    char pad[4];
    union {
        int64_t i;
        uint64_t u;
        double f;
    } data;
} pyyjson_number_op;

// size = 16 / 24
typedef struct pyyjson_string_op {
    pyyjson_op op_base; // here we force the size of pyyjson_string_op to be 16 for 32bit platforms
    char *data;
    Py_ssize_t len;
} pyyjson_string_op;

// size = 8 / 16
typedef struct pyyjson_container_op {
    PYYJSON_OP_HEAD
    Py_ssize_t len;
} pyyjson_container_op;

// size = 8
typedef struct pyyjson_nan_inf_op {
    PYYJSON_OP_HEAD
    bool sign;
} pyyjson_nan_inf_op;

PyObject *pyyjson_op_loads(pyyjson_op *op_sequence);

extern PyObject *JSONDecodeError;

static_assert((sizeof(pyyjson_op) % sizeof(pyyjson_op)) == 0);
static_assert((sizeof(pyyjson_number_op) % sizeof(pyyjson_op)) == 0);
static_assert((sizeof(pyyjson_string_op) % sizeof(pyyjson_op)) == 0);
static_assert((sizeof(pyyjson_container_op) % sizeof(pyyjson_op)) == 0);
static_assert((sizeof(pyyjson_nan_inf_op) % sizeof(pyyjson_op)) == 0);

#endif
