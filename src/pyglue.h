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
#define PYYJSON_STRING_FLAG_OBJ_KEY (1 << (PYYJSON_OP_HIGHER_START + 3))
#define PYYJSON_STRING_FLAG_UCS_TYPE_MASK (PYYJSON_STRING_FLAG_ASCII | PYYJSON_STRING_FLAG_LATIN1 | PYYJSON_STRING_FLAG_UCS2 | PYYJSON_STRING_FLAG_UCS4)
#define PYYJSON_STRING_FLAG_MASK (PYYJSON_STRING_FLAG_ASCII | PYYJSON_STRING_FLAG_LATIN1 | PYYJSON_STRING_FLAG_UCS2 | PYYJSON_STRING_FLAG_UCS4 | PYYJSON_STRING_FLAG_OBJ_KEY)
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
#define PYYJSON_NAN_INF_FLAG_SIGNED (1 << (PYYJSON_OP_HIGHER_START + 1))
#define PYYJSON_NAN_INF_FLAG_MASK_WITHOUT_SIGNED (PYYJSON_NAN_INF_FLAG_NAN | PYYJSON_NAN_INF_FLAG_INF)
#define PYYJSON_NAN_INF_FLAG_MASK (PYYJSON_NAN_INF_FLAG_NAN | PYYJSON_NAN_INF_FLAG_INF | PYYJSON_NAN_INF_FLAG_SIGNED)
// end flags
typedef uint32_t op_type;
#define PYYJSON_OP_HEAD pyyjson_op_base op_base;
#define PYYJSON_READ_OP(_op) (((pyyjson_op_base *) (_op))->op)
#define PYYJSON_WRITE_OP(_ptr, _code)               \
    do {                                            \
        ((pyyjson_op_base *) (_ptr))->op = (_code); \
    } while (0)

#if INTPTR_MAX == INT64_MAX
#define PYYJSON_64BIT
#elif INTPTR_MAX == INT32_MAX
#define PYYJSON_32BIT
#else
#error "Unsupported platform"
#endif

#ifdef PYYJSON_64BIT
#define PYYJSON_OP_PADDING char pad[4];
#else
#define PYYJSON_OP_PADDING
#endif

// size = 4. This should not be used directly
typedef struct pyyjson_op_base {
    op_type op;
} pyyjson_op_base;

// size = 4 / 8
typedef struct pyyjson_op {
    PYYJSON_OP_HEAD
    PYYJSON_OP_PADDING
} pyyjson_op;

// size = 12 / 16
typedef struct pyyjson_number_op {
    PYYJSON_OP_HEAD
    union {
        int64_t i;
        uint64_t u;
        double f;
    } data;
} pyyjson_number_op;

// size = 12 / 24
typedef struct pyyjson_string_op {
    PYYJSON_OP_HEAD
    char *data;
    Py_ssize_t len;
} pyyjson_string_op;

// size = 8 / 16
typedef struct pyyjson_container_op {
    PYYJSON_OP_HEAD
    Py_ssize_t len;
} pyyjson_container_op;

PyObject *pyyjson_op_loads(pyyjson_op *op_sequence);

extern PyObject *JSONDecodeError;

#define PYYJSON_STRING_BUFFER_SIZE (32 * 1024 * 1024)
#define PYYJSON_KEY_CACHE_SIZE (1 << 11)
typedef PyObject *pyyjson_cache_type;
extern pyyjson_cache_type AssociativeKeyCache[PYYJSON_KEY_CACHE_SIZE];

// static assertions
static_assert((sizeof(pyyjson_op) % sizeof(pyyjson_op)) == 0);
static_assert((sizeof(pyyjson_number_op) % sizeof(pyyjson_op)) == 0);
static_assert((sizeof(pyyjson_string_op) % sizeof(pyyjson_op)) == 0);
static_assert((sizeof(pyyjson_container_op) % sizeof(pyyjson_op)) == 0);

#endif
